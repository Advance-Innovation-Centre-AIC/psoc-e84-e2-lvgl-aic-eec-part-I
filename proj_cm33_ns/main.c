/*******************************************************************************
* File Name        : main.c
*
* Description      : CM33 non-secure application with FreeRTOS
*                    Runs IMU, IPC, and WiFi tasks concurrently.
*                    Migrated from bare-metal SysTick to FreeRTOS scheduler.
*
* Related Document : See README.md
*
********************************************************************************
 * (c) 2025, Infineon Technologies AG
 * Part of BiiL Course: Embedded C for IoT
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "cy_scb_i2c.h"
#include "cy_syslib.h"
#include "mtb_hal.h"
#include "mtb_bmi270.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* Retarget-IO for printf (now available with FreeRTOS) */
#include "retarget_io_init.h"
#include <stdio.h>

/* ADC (Autonomous Analog Controller) for Potentiometer */
#include "cy_autanalog.h"

/* Shared memory for CM33-CM55 IPC */
#include "../shared/imu_shared.h"

/* WiFi Task */
#include "source/wifi_task.h"

/* Bluetooth Task */
#include "source/bt_task.h"

/* CAPSENSE Module (read via I2C, send via IPC) */
#include "source/capsense_task.h"

/*******************************************************************************
 * IPC Communication (CM33-NS <-> CM55)
 ******************************************************************************/
#define IPC_ENABLED     (1U)

#if IPC_ENABLED
#include "ipc/cm33_ipc_pipe.h"
#endif


/*******************************************************************************
* Macros
*******************************************************************************/
#define CM55_BOOT_WAIT_TIME_USEC    (10U)
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

#define IMU_POLL_INTERVAL_MS        (100U)

/* Task stack sizes and priorities */
#define IMU_TASK_STACK_SIZE         (512U)
#define IMU_TASK_PRIORITY           (2U)

#define IPC_TASK_STACK_SIZE         (512U)
#define IPC_TASK_PRIORITY           (3U)

/*******************************************************************************
* BMI270 Configuration
*******************************************************************************/
#define GRAVITY_EARTH               (9.80665f)
#define DEG_TO_RAD                  (0.01745f)
#define GYR_RANGE_DPS               (2000.0f)
#define ACC_RANGE_2G                (2.0f)


/*******************************************************************************
* Global Variables
*******************************************************************************/
static cy_stc_scb_i2c_context_t i2c_context;
static mtb_hal_i2c_t i2c_hal_obj;
static mtb_bmi270_t bmi270_dev;
static mtb_bmi270_data_t bmi270_data;
static bool bmi270_initialized = false;

/* Last known good IMU values (for sanity check) */
static float last_ax = 0.0f, last_ay = 0.0f, last_az = 9.8f;
static float last_gx = 0.0f, last_gy = 0.0f, last_gz = 0.0f;
static bool first_read = true;

/* Sanity check limits */
#define ACCEL_MAX_VALUE     (20.0f)
#define ACCEL_MAX_DELTA     (15.0f)


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static bool imu_init(void);
static void imu_read_and_update(void);
static void imu_task(void *pvParameters);
static void ipc_processing_task(void *pvParameters);


/*******************************************************************************
* IMU Initialization
*******************************************************************************/
static bool imu_init(void)
{
    cy_rslt_t result;

    /* Initialize I2C */
    if (CY_SCB_I2C_SUCCESS != Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
                                               &CYBSP_I2C_CONTROLLER_config,
                                               &i2c_context)) {
        return false;
    }
    Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

    /* Setup HAL I2C */
    result = mtb_hal_i2c_setup(&i2c_hal_obj,
                                &CYBSP_I2C_CONTROLLER_hal_config,
                                &i2c_context, NULL);
    if (CY_RSLT_SUCCESS != result) {
        return false;
    }

    /* Initialize BMI270 */
    result = mtb_bmi270_init_i2c(&bmi270_dev, &i2c_hal_obj,
                                  MTB_BMI270_ADDRESS_DEFAULT);
    if (CY_RSLT_SUCCESS != result) {
        return false;
    }

    /* Configure BMI270 */
    result = mtb_bmi270_config_default(&bmi270_dev);
    if (CY_RSLT_SUCCESS != result) {
        return false;
    }

    return true;
}


/*******************************************************************************
* Helper: Convert raw accel to m/s^2
*******************************************************************************/
static float lsb_to_mps2(int16_t val, int8_t g_range, uint8_t bit_width)
{
    float half_scale = (float)(1u << (bit_width - 1u));
    return (GRAVITY_EARTH * val * g_range) / half_scale;
}


/*******************************************************************************
* Helper: Convert raw gyro to rad/s
*******************************************************************************/
static float lsb_to_rps(int16_t val, float dps, uint8_t bit_width)
{
    float half_scale = (float)(1u << (bit_width - 1u));
    return (DEG_TO_RAD * dps / half_scale) * val;
}


/*******************************************************************************
* Helper: Check if value is within reasonable range
*******************************************************************************/
static inline float fabsf_simple(float x)
{
    return (x < 0.0f) ? -x : x;
}

static inline bool accel_is_valid(float ax, float ay, float az)
{
    if (fabsf_simple(ax) > ACCEL_MAX_VALUE ||
        fabsf_simple(ay) > ACCEL_MAX_VALUE ||
        fabsf_simple(az) > ACCEL_MAX_VALUE) {
        return false;
    }

    if (!first_read) {
        if (fabsf_simple(ax - last_ax) > ACCEL_MAX_DELTA ||
            fabsf_simple(ay - last_ay) > ACCEL_MAX_DELTA ||
            fabsf_simple(az - last_az) > ACCEL_MAX_DELTA) {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
* Read IMU and update shared memory
*******************************************************************************/
static void imu_read_and_update(void)
{
    if (!bmi270_initialized) return;

    cy_rslt_t result = mtb_bmi270_read(&bmi270_dev, &bmi270_data);

    if (CY_RSLT_SUCCESS != result) {
        imu_shared_error();
        return;
    }

    /* Convert to physical units */
    float ax = lsb_to_mps2(bmi270_data.sensor_data.acc.x, ACC_RANGE_2G,
                           bmi270_dev.sensor.resolution);
    float ay = lsb_to_mps2(bmi270_data.sensor_data.acc.y, ACC_RANGE_2G,
                           bmi270_dev.sensor.resolution);
    float az = lsb_to_mps2(bmi270_data.sensor_data.acc.z, ACC_RANGE_2G,
                           bmi270_dev.sensor.resolution);

    float gx = lsb_to_rps(bmi270_data.sensor_data.gyr.x, GYR_RANGE_DPS,
                          bmi270_dev.sensor.resolution);
    float gy = lsb_to_rps(bmi270_data.sensor_data.gyr.y, GYR_RANGE_DPS,
                          bmi270_dev.sensor.resolution);
    float gz = lsb_to_rps(bmi270_data.sensor_data.gyr.z, GYR_RANGE_DPS,
                          bmi270_dev.sensor.resolution);

    /* Sanity check */
    if (!accel_is_valid(ax, ay, az)) {
        ax = last_ax;
        ay = last_ay;
        az = last_az;
        gx = last_gx;
        gy = last_gy;
        gz = last_gz;
        imu_shared_error();
    } else {
        last_ax = ax;
        last_ay = ay;
        last_az = az;
        last_gx = gx;
        last_gy = gy;
        last_gz = gz;
        first_read = false;
    }

    /* Update shared memory (use FreeRTOS tick as timestamp) */
    imu_shared_update(ax, ay, az, gx, gy, gz,
                      (uint32_t)xTaskGetTickCount());
}


/*******************************************************************************
* IMU Task - Periodic IMU reading via FreeRTOS
*******************************************************************************/
static void imu_task(void *pvParameters)
{
    (void)pvParameters;

    /* Initialize IMU (I2C + BMI270) inside task context */
    bmi270_initialized = imu_init();

    if (bmi270_initialized) {
        printf("[CM33] IMU initialized (BMI270)\r\n");

        /* Initialize CAPSENSE module (shares I2C bus with IMU) */
        capsense_module_init(CYBSP_I2C_CONTROLLER_HW, &i2c_context);
        printf("[CM33] CAPSENSE module initialized (I2C 0x%02X)\r\n",
               CAPSENSE_I2C_SLAVE_ADDR);
    } else {
        printf("[CM33] IMU init failed - sensor data unavailable\r\n");
    }

    for (;;)
    {
        imu_read_and_update();
        capsense_module_poll();
        vTaskDelay(pdMS_TO_TICKS(IMU_POLL_INTERVAL_MS));
    }
}


/*******************************************************************************
* IPC Processing Task - Handles incoming IPC messages
*******************************************************************************/
static void ipc_processing_task(void *pvParameters)
{
    (void)pvParameters;

    printf("[CM33] IPC processing task started\r\n");

    for (;;)
    {
#if IPC_ENABLED
        cm33_ipc_process();
#endif
        vTaskDelay(pdMS_TO_TICKS(10));  /* 10ms polling interval */
    }
}


/*******************************************************************************
* FreeRTOS Hook Functions (required by FreeRTOSConfig.h settings)
*******************************************************************************/

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[CM33] FATAL: Stack overflow in task '%s'\r\n", pcTaskName);
    CY_ASSERT(0);
}

void vApplicationMallocFailedHook(void)
{
    printf("[CM33] FATAL: Malloc failed\r\n");
    CY_ASSERT(0);
}


/*******************************************************************************
* Main Function
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize board */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result) {
        __disable_irq();
        CY_ASSERT(0);
        while(true);
    }

    /* Initialize retarget-io for debug UART (printf) */
    init_retarget_io();

    printf("\r\n========================================\r\n");
    printf("  CM33-NS FreeRTOS Application\r\n");
    printf("  BiiL Course: Embedded C for IoT\r\n");
    printf("========================================\r\n\r\n");

    /* Initialize shared memory for IMU data */
    imu_shared_init();

    /***************************************************************************
     * Initialize ADC (Autonomous Analog Controller) for Potentiometer
     * This runs autonomously - no RTOS dependency needed
     ***************************************************************************/
    result = Cy_AutAnalog_Init(&autonomous_analog_init);
    if (CY_AUTANALOG_SUCCESS == result) {
        Cy_AutAnalog_SetInterruptMask(CY_AUTANALOG_INT_SAR0_RESULT);
        Cy_AutAnalog_StartAutonomousControl();
        printf("[CM33] ADC initialized (autonomous mode)\r\n");
    }

    /* Enable interrupts BEFORE IPC init (reference project pattern) */
    __enable_irq();

#if IPC_ENABLED
    /* Initialize IPC Pipe infrastructure BEFORE enabling CM55 */
    cm33_ipc_init();
    printf("[CM33] IPC Pipe initialized\r\n");
#endif

    /* Enable CM55 (must be AFTER IPC init to avoid race condition) */
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
    printf("[CM33] CM55 enabled at 0x%08X\r\n", (unsigned int)CM55_APP_BOOT_ADDR);

    /***************************************************************************
     * Create FreeRTOS Tasks
     ***************************************************************************/

    /* IMU Task: Periodic sensor reading (I2C + BMI270) */
    xTaskCreate(imu_task, "IMU Task",
                IMU_TASK_STACK_SIZE, NULL,
                IMU_TASK_PRIORITY, NULL);

    /* IPC Task: Process incoming IPC messages from CM55 */
    xTaskCreate(ipc_processing_task, "IPC Task",
                IPC_TASK_STACK_SIZE, NULL,
                IPC_TASK_PRIORITY, NULL);

    /* WiFi Task: WiFi scanning and connection management */
    xTaskCreate(wifi_task, "WiFi Task",
                WIFI_TASK_STACK_SIZE, NULL,
                WIFI_TASK_PRIORITY, NULL);

    /* Bluetooth Task: BLE scanning and connection management */
    xTaskCreate(bt_task, "BT Task",
                BT_TASK_STACK_SIZE, NULL,
                BT_TASK_PRIORITY, NULL);

    printf("[CM33] Starting FreeRTOS scheduler...\r\n\r\n");

    /* Start FreeRTOS scheduler - does not return */
    vTaskStartScheduler();

    /* Should never reach here */
    printf("[CM33] ERROR: Scheduler returned!\r\n");
    CY_ASSERT(0);

    return 0;
}


/* [] END OF FILE */
