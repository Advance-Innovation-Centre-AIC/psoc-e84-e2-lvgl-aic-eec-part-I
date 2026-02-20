/*******************************************************************************
 * File Name:   sensors.c
 *
 * Description: AIC-EEC Sensor Hardware Abstraction Layer Implementation
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 * Note: This implementation provides both hardware sensor access and
 *       simulation mode for UI development without hardware.
 *
 ******************************************************************************/

#include "sensors.h"
#include "ma_filter.h"      /* Moving Average Filter for sensor smoothing */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* Include PSoC HAL - always include for CM55 builds */
#include "cybsp.h"
#include "cycfg_peripherals.h"   /* For CYBSP_SAR_ADC_* */

/* Check if hardware ADC is available by checking BSP ADC definitions */
#if defined(CYBSP_SAR_ADC_ENABLED) && (CYBSP_SAR_ADC_ENABLED == 1U)
#include "cy_autanalog.h"    /* Autonomous Analog Controller for ADC */
#define HW_ADC_AVAILABLE 1
#else
#define HW_ADC_AVAILABLE 0
#endif

/* IMU shared memory for CM33-CM55 IPC (BMI270 data from CM33) */
#include "../../shared/imu_shared.h"
#define HW_IMU_AVAILABLE 1  /* IMU data available via shared memory from CM33 */

/*******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* ADC reference voltage in millivolts (PSoC Edge uses 1.8V internal ref) */
#define DEFAULT_VREF_MV         (1800U)

/* ADC hardware configuration (for PSoC Edge with AAC) */
#define SAR_ADC_INDEX           (0U)
#define SAR_ADC_SEQUENCER       (0U)

/* ADC maximum values by resolution */
#define ADC_MAX_8BIT            (255U)
#define ADC_MAX_10BIT           (1023U)
#define ADC_MAX_12BIT           (4095U)

/* IMU conversion factors (based on BMI270 configuration) */
#define GRAVITY_EARTH           (9.80665f)  /* m/s^2 */
#define DEG_TO_RAD              (0.01745f)  /* Degrees to radians */
#define GYR_RANGE_DPS           (2000.0f)   /* Gyroscope range in deg/s */
#define ACC_RANGE_2G            (2.0f)      /* Accelerometer range in g */

/* Legacy scales (for raw value conversion) */
#define ACCEL_SCALE_2G          (16384.0f)  /* LSB/g for +/-2g range */
#define GYRO_SCALE_250DPS       (131.0f)    /* LSB/(deg/s) for +/-250 deg/s */

/* Temperature sensor calibration */
#define TEMP_OFFSET             (0.0f)
#define TEMP_SCALE              (1.0f)

/* Orientation detection thresholds */
#define ORIENT_THRESHOLD        (0.7f)      /* g value for orientation */

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Initialization flags */
static bool sensors_initialized = false;
static bool adc_initialized = false;
static bool imu_initialized = false;

/* ADC configuration */
static aic_adc_resolution_t current_resolution = AIC_ADC_RES_12BIT;
static uint16_t current_vref_mv = DEFAULT_VREF_MV;
static uint16_t adc_max_value = ADC_MAX_12BIT;

/* Simulation mode */
static bool simulation_mode = true;  /* Default to simulation for safety */

/* Simulated sensor values */
static uint16_t sim_adc_values[AIC_ADC_CH_COUNT] = {2048, 0, 0, 0, 2048};
static float sim_accel[AIC_AXIS_COUNT] = {0.0f, 0.0f, 1.0f};  /* 1g on Z */
static float sim_gyro[AIC_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};

/* IMU calibration offsets */
static float accel_offset[AIC_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};
static float gyro_offset[AIC_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};

/* Moving Average Filters for IMU data smoothing */
static ma_filter_3axis_t accel_filter;
static ma_filter_3axis_t gyro_filter;
static bool filters_initialized = false;

/* Cached IMU values (used when shared memory read fails due to CM33 writing) */
static float cached_accel[AIC_AXIS_COUNT] = {0.0f, 0.0f, 9.8f};  /* Default 1g on Z */
static float cached_gyro[AIC_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};
static bool imu_cache_valid = false;

/* Channel names */
static const char* adc_channel_names[AIC_ADC_CH_COUNT] = {
    "CH0", "CH1", "CH2", "CH3", "Temp"
};

/* Orientation names */
static const char* orientation_names[] = {
    "Unknown",
    "Flat Up",
    "Flat Down",
    "Portrait",
    "Portrait Inverted",
    "Landscape",
    "Landscape Inverted"
};

/* Note: IMU code is in Week 4 Part II sensors.c */

/*******************************************************************************
 * Sensor Initialization
 ******************************************************************************/

bool aic_sensors_init(void)
{
    if (sensors_initialized) {
        return true;
    }

    printf("[Sensors] Initializing sensor subsystem...\r\n");

    /* Initialize ADC with default 12-bit resolution */
    if (!aic_adc_init(AIC_ADC_RES_12BIT)) {
        printf("[Sensors] Warning: ADC init failed\r\n");
    }

    /* Initialize IMU */
    if (!aic_imu_init()) {
        printf("[Sensors] Warning: IMU init failed\r\n");
    }

    sensors_initialized = true;
    printf("[Sensors] Sensor subsystem initialized\r\n");
    return true;
}

void aic_sensors_deinit(void)
{
    adc_initialized = false;
    imu_initialized = false;
    sensors_initialized = false;
    printf("[Sensors] Sensor subsystem deinitialized\r\n");
}

/*******************************************************************************
 * ADC Functions
 ******************************************************************************/

bool aic_adc_init(aic_adc_resolution_t resolution)
{
    current_resolution = resolution;

    /* Set max value based on resolution */
    switch (resolution) {
        case AIC_ADC_RES_8BIT:
            adc_max_value = ADC_MAX_8BIT;
            break;
        case AIC_ADC_RES_10BIT:
            adc_max_value = ADC_MAX_10BIT;
            break;
        case AIC_ADC_RES_12BIT:
        default:
            adc_max_value = ADC_MAX_12BIT;
            current_resolution = AIC_ADC_RES_12BIT;
            break;
    }

#if HW_ADC_AVAILABLE
    /* Hardware ADC initialization */
    /* Note: SAR ADC configuration is typically done in Device Configurator */
    /* Example:
     * cy_rslt_t result = Cy_SAR_Init(SAR0, &sar_config);
     * if (result == CY_RSLT_SUCCESS) {
     *     Cy_SAR_Enable(SAR0);
     *     adc_initialized = true;
     * }
     */
    adc_initialized = true;
    simulation_mode = false;
    printf("[ADC] Hardware initialized (%d-bit)\r\n", resolution);
#else
    /* Software simulation mode */
    adc_initialized = true;
    simulation_mode = true;
    printf("[ADC] Simulation mode (%d-bit)\r\n", resolution);
#endif

    return adc_initialized;
}

uint16_t aic_adc_read(aic_adc_channel_t channel)
{
    if (channel >= AIC_ADC_CH_COUNT) {
        return 0;
    }

    if (simulation_mode || !adc_initialized) {
        return sim_adc_values[channel];
    }

#if HW_ADC_AVAILABLE
    /*
     * Hardware ADC read using Autonomous Analog Controller (AAC)
     * Note: AAC must be initialized and started from CM33 core first.
     * The Cy_AutAnalog_SAR_ReadResult() function reads from the SAR ADC.
     *
     * Reference: PSOC_Edge_ADC project main.c
     *   Cy_AutAnalog_Init(&autonomous_analog_init);
     *   Cy_AutAnalog_StartAutonomousControl();
     *   sar_adc_count = Cy_AutAnalog_SAR_ReadResult(SAR_ADC_INDEX,
     *                                              CY_AUTANALOG_SAR_INPUT_GPIO,
     *                                              channel);
     */
    int32_t adc_count = Cy_AutAnalog_SAR_ReadResult(SAR_ADC_INDEX,
                                                   CY_AUTANALOG_SAR_INPUT_GPIO,
                                                   (uint32_t)channel);

    /* If hardware read returns valid data, use it */
    if (adc_count >= 0 && adc_count <= (int32_t)adc_max_value) {
        return (uint16_t)adc_count;
    }

    /* Fallback to simulation if hardware read fails */
    return sim_adc_values[channel];
#else
    return sim_adc_values[channel];
#endif
}

float aic_adc_read_voltage(aic_adc_channel_t channel)
{
    uint16_t raw = aic_adc_read(channel);
    return aic_adc_raw_to_voltage(raw, current_resolution, current_vref_mv);
}

uint8_t aic_adc_read_percent(aic_adc_channel_t channel)
{
    uint16_t raw = aic_adc_read(channel);
    return (uint8_t)((raw * 100) / adc_max_value);
}

void aic_adc_set_vref(uint16_t vref_mv)
{
    current_vref_mv = vref_mv;
    printf("[ADC] VREF set to %u mV\r\n", vref_mv);
}

uint16_t aic_adc_get_vref(void)
{
    return current_vref_mv;
}

float aic_adc_read_temperature(void)
{
    if (simulation_mode) {
        /* Return room temperature */
        return 25.0f;
    }

#if HW_ADC_AVAILABLE
    /* Read internal temperature sensor */
    /* Hardware-specific implementation */
    return 25.0f;
#else
    return 25.0f;
#endif
}

/*******************************************************************************
 * IMU Functions
 ******************************************************************************/

bool aic_imu_init(void)
{
    /* Initialize Moving Average Filters for accel and gyro */
    if (!filters_initialized) {
        ma_filter_3axis_init(&accel_filter, MA_FILTER_DEFAULT_SIZE);
        ma_filter_3axis_init(&gyro_filter, MA_FILTER_DEFAULT_SIZE);
        filters_initialized = true;
        printf("[IMU] Moving Average Filter initialized (size=%d)\r\n", MA_FILTER_DEFAULT_SIZE);
    }

#if HW_IMU_AVAILABLE
    /* Check if CM33 has initialized IMU shared memory */
    if (IMU_SHARED_IS_VALID()) {
        imu_initialized = true;
        simulation_mode = false;
        printf("[IMU] Hardware mode (via CM33 shared memory)\r\n");
        return true;
    }
    /* Fallback to simulation if shared memory not ready */
    printf("[IMU] Waiting for CM33 to initialize BMI270...\r\n");
#endif
    /* Simulation mode fallback */
    imu_initialized = true;
    printf("[IMU] Simulation mode\r\n");
    return imu_initialized;
}

bool aic_imu_read_accel(float *ax, float *ay, float *az)
{
    if (!imu_initialized || ax == NULL || ay == NULL || az == NULL) {
        return false;
    }

    float raw_ax, raw_ay, raw_az;
    bool success = false;

#if HW_IMU_AVAILABLE
    /* Try to read from shared memory (CM33 BMI270 data) */
    if (IMU_SHARED_IS_VALID()) {
        if (imu_shared_read_accel(&raw_ax, &raw_ay, &raw_az)) {
            raw_ax -= accel_offset[AIC_AXIS_X];
            raw_ay -= accel_offset[AIC_AXIS_Y];
            raw_az -= accel_offset[AIC_AXIS_Z];
            /* Update cache with new valid data */
            cached_accel[AIC_AXIS_X] = raw_ax;
            cached_accel[AIC_AXIS_Y] = raw_ay;
            cached_accel[AIC_AXIS_Z] = raw_az;
            imu_cache_valid = true;
            success = true;
        } else if (imu_cache_valid) {
            /* CM33 is writing - use cached value instead of jumping to simulation */
            raw_ax = cached_accel[AIC_AXIS_X];
            raw_ay = cached_accel[AIC_AXIS_Y];
            raw_az = cached_accel[AIC_AXIS_Z];
            success = true;
        }
    }
#endif

    /* Fallback to simulation mode only if no hardware and no cache */
    if (!success) {
        raw_ax = sim_accel[AIC_AXIS_X] - accel_offset[AIC_AXIS_X];
        raw_ay = sim_accel[AIC_AXIS_Y] - accel_offset[AIC_AXIS_Y];
        raw_az = sim_accel[AIC_AXIS_Z] - accel_offset[AIC_AXIS_Z];
        success = true;
    }

    /* Apply Moving Average Filter to reduce noise */
    if (filters_initialized) {
        ma_filter_3axis_update(&accel_filter, raw_ax, raw_ay, raw_az, ax, ay, az);
    } else {
        *ax = raw_ax;
        *ay = raw_ay;
        *az = raw_az;
    }

    return success;
}

bool aic_imu_read_gyro(float *gx, float *gy, float *gz)
{
    if (!imu_initialized || gx == NULL || gy == NULL || gz == NULL) {
        return false;
    }

    float raw_gx, raw_gy, raw_gz;
    bool success = false;

#if HW_IMU_AVAILABLE
    /* Try to read from shared memory (CM33 BMI270 data) */
    if (IMU_SHARED_IS_VALID()) {
        if (imu_shared_read_gyro(&raw_gx, &raw_gy, &raw_gz)) {
            raw_gx -= gyro_offset[AIC_AXIS_X];
            raw_gy -= gyro_offset[AIC_AXIS_Y];
            raw_gz -= gyro_offset[AIC_AXIS_Z];
            /* Update cache with new valid data */
            cached_gyro[AIC_AXIS_X] = raw_gx;
            cached_gyro[AIC_AXIS_Y] = raw_gy;
            cached_gyro[AIC_AXIS_Z] = raw_gz;
            success = true;
        } else if (imu_cache_valid) {
            /* CM33 is writing - use cached value instead of jumping to simulation */
            raw_gx = cached_gyro[AIC_AXIS_X];
            raw_gy = cached_gyro[AIC_AXIS_Y];
            raw_gz = cached_gyro[AIC_AXIS_Z];
            success = true;
        }
    }
#endif

    /* Fallback to simulation mode only if no hardware and no cache */
    if (!success) {
        raw_gx = sim_gyro[AIC_AXIS_X] - gyro_offset[AIC_AXIS_X];
        raw_gy = sim_gyro[AIC_AXIS_Y] - gyro_offset[AIC_AXIS_Y];
        raw_gz = sim_gyro[AIC_AXIS_Z] - gyro_offset[AIC_AXIS_Z];
        success = true;
    }

    /* Apply Moving Average Filter to reduce noise */
    if (filters_initialized) {
        ma_filter_3axis_update(&gyro_filter, raw_gx, raw_gy, raw_gz, gx, gy, gz);
    } else {
        *gx = raw_gx;
        *gy = raw_gy;
        *gz = raw_gz;
    }

    return success;
}

bool aic_imu_read_all(aic_imu_data_t *data)
{
    if (!imu_initialized || data == NULL) {
        return false;
    }

    bool accel_ok = aic_imu_read_accel(&data->accel_x, &data->accel_y, &data->accel_z);
    bool gyro_ok = aic_imu_read_gyro(&data->gyro_x, &data->gyro_y, &data->gyro_z);

    /* Convert to raw values */
    data->accel_raw_x = (int32_t)(data->accel_x * ACCEL_SCALE_2G);
    data->accel_raw_y = (int32_t)(data->accel_y * ACCEL_SCALE_2G);
    data->accel_raw_z = (int32_t)(data->accel_z * ACCEL_SCALE_2G);
    data->gyro_raw_x = (int32_t)(data->gyro_x * GYRO_SCALE_250DPS);
    data->gyro_raw_y = (int32_t)(data->gyro_y * GYRO_SCALE_250DPS);
    data->gyro_raw_z = (int32_t)(data->gyro_z * GYRO_SCALE_250DPS);

    return accel_ok && gyro_ok;
}

aic_orientation_t aic_imu_get_orientation(void)
{
    float ax, ay, az;

    if (!aic_imu_read_accel(&ax, &ay, &az)) {
        return AIC_ORIENT_UNKNOWN;
    }

    /* Determine orientation based on gravity vector */
    if (az > ORIENT_THRESHOLD) {
        return AIC_ORIENT_FLAT_UP;
    } else if (az < -ORIENT_THRESHOLD) {
        return AIC_ORIENT_FLAT_DOWN;
    } else if (ay > ORIENT_THRESHOLD) {
        return AIC_ORIENT_PORTRAIT;
    } else if (ay < -ORIENT_THRESHOLD) {
        return AIC_ORIENT_PORTRAIT_INV;
    } else if (ax > ORIENT_THRESHOLD) {
        return AIC_ORIENT_LANDSCAPE;
    } else if (ax < -ORIENT_THRESHOLD) {
        return AIC_ORIENT_LANDSCAPE_INV;
    }

    return AIC_ORIENT_UNKNOWN;
}

const char* aic_imu_orientation_name(aic_orientation_t orient)
{
    if (orient > AIC_ORIENT_LANDSCAPE_INV) {
        return orientation_names[0];  /* Unknown */
    }
    return orientation_names[orient];
}

bool aic_imu_calibrate(void)
{
    if (!imu_initialized) {
        return false;
    }

    printf("[IMU] Calibrating... keep device still\r\n");

    /* Average multiple readings */
    const int num_samples = 100;
    float sum_ax = 0, sum_ay = 0, sum_az = 0;
    float sum_gx = 0, sum_gy = 0, sum_gz = 0;

    for (int i = 0; i < num_samples; i++) {
        float ax, ay, az, gx, gy, gz;

        aic_imu_read_accel(&ax, &ay, &az);
        aic_imu_read_gyro(&gx, &gy, &gz);

        sum_ax += ax;
        sum_ay += ay;
        sum_az += az;
        sum_gx += gx;
        sum_gy += gy;
        sum_gz += gz;
    }

    /* Calculate offsets (assuming device is flat, Z should be 1g) */
    accel_offset[AIC_AXIS_X] = sum_ax / num_samples;
    accel_offset[AIC_AXIS_Y] = sum_ay / num_samples;
    accel_offset[AIC_AXIS_Z] = (sum_az / num_samples) - 1.0f;  /* Remove 1g */
    gyro_offset[AIC_AXIS_X] = sum_gx / num_samples;
    gyro_offset[AIC_AXIS_Y] = sum_gy / num_samples;
    gyro_offset[AIC_AXIS_Z] = sum_gz / num_samples;

    printf("[IMU] Calibration complete\r\n");
    return true;
}

bool aic_imu_is_available(void)
{
    return imu_initialized;
}

/*******************************************************************************
 * Simulation Functions
 ******************************************************************************/

void aic_sensors_set_simulation(bool enable)
{
    simulation_mode = enable;
    printf("[Sensors] Simulation mode: %s\r\n", enable ? "ON" : "OFF");
}

bool aic_sensors_is_simulation(void)
{
    return simulation_mode;
}

void aic_adc_set_simulated(aic_adc_channel_t channel, uint16_t value)
{
    if (channel >= AIC_ADC_CH_COUNT) {
        return;
    }

    /* Clamp to max value */
    if (value > adc_max_value) {
        value = adc_max_value;
    }

    sim_adc_values[channel] = value;
}

void aic_imu_set_simulated_accel(float ax, float ay, float az)
{
    sim_accel[AIC_AXIS_X] = ax;
    sim_accel[AIC_AXIS_Y] = ay;
    sim_accel[AIC_AXIS_Z] = az;
}

void aic_imu_set_simulated_gyro(float gx, float gy, float gz)
{
    sim_gyro[AIC_AXIS_X] = gx;
    sim_gyro[AIC_AXIS_Y] = gy;
    sim_gyro[AIC_AXIS_Z] = gz;
}

/*******************************************************************************
 * CAPSENSE I2C Functions
 *
 * IMPORTANT: I2C for CAPSENSE (SCB0) communication
 *
 * The PSoC 4000T CAPSENSE chip communicates via I2C:
 *   - Address: 0x08
 *   - Data format: 3 bytes [BTN0_status, BTN1_status, Slider_pos]
 *   - BTN0/BTN1 are ASCII '0' or '1' (subtract 0x30 to get numeric)
 *   - Slider is raw value 0-100 (0 = no touch)
 *
 * NOTE: CAPSENSE shares I2C bus (SCB0) with display touch controller.
 *       I2C is already initialized by main.c for touch screen.
 *       We use the shared context: disp_touch_i2c_controller_context
 *
 * Reference: PSOC_Edge_Bluetooth_LE_with_CAPSENSE_Button_and_Slider
 *            proj_cm33_ns/source/i2c_capsense/i2c_capsense.c
 *
 ******************************************************************************/

/* Check if I2C controller is available */
#if defined(CYBSP_I2C_CONTROLLER_ENABLED) && (CYBSP_I2C_CONTROLLER_ENABLED == 1U)
#define HW_CAPSENSE_AVAILABLE 1
#include "cy_scb_i2c.h"
#include "display_i2c_config.h"   /* For DISPLAY_I2C_CONTROLLER_HW defines */
#else
#define HW_CAPSENSE_AVAILABLE 0
#endif

/* CAPSENSE I2C configuration - match BLE example exactly */
#define CAPSENSE_I2C_SLAVE_ADDR     (0x08U)  /* PSoC 4000T I2C address */
#define CAPSENSE_I2C_TIMEOUT_MS     (0U)     /* 0 = blocking (required for reliable communication) */
#define CAPSENSE_READ_SIZE          (3U)
#define CAPSENSE_ASCII_OFFSET       (30U)    /* Match BLE example: INTERGER_ASCII_DIFFERENCE */

/* Button status constants (from PSoC 4000T)
 * Both buttons: 0 = not pressed, 1 = pressed */
#define CAPSENSE_BTN0_NOT_PRESSED   (0U)
#define CAPSENSE_BTN1_NOT_PRESSED   (0U)
#define CAPSENSE_SLIDER_NO_TOUCH    (0U)

/* Private variables for CAPSENSE */
static bool capsense_initialized = false;
static bool capsense_hw_detected = false;

/* Create our own I2C context for CAPSENSE (like BLE example)
 * The display touch uses disp_touch_i2c_controller_context.
 * We need a separate context and proper I2C initialization.
 */
static cy_stc_scb_i2c_context_t capsense_i2c_context;

bool aic_capsense_init(void)
{
    if (capsense_initialized) {
        return capsense_hw_detected;
    }

#if HW_CAPSENSE_AVAILABLE
    cy_en_scb_i2c_status_t initStatus;

    printf("[CAPSENSE] Initializing I2C for PSoC 4000T...\r\n");
    printf("[CAPSENSE] I2C HW: SCB0, Address: 0x%02X\r\n", CAPSENSE_I2C_SLAVE_ADDR);

    /* IMPORTANT: I2C was already initialized by touch driver in main.c
     * We need to completely reset it before re-initializing for CAPSENSE.
     * This is necessary because touch driver configured it differently.
     */
    printf("[CAPSENSE] Disabling existing I2C...\r\n");
    Cy_SCB_I2C_Disable(CYBSP_I2C_CONTROLLER_HW, &capsense_i2c_context);
    Cy_SCB_I2C_DeInit(CYBSP_I2C_CONTROLLER_HW);

    /* Now initialize I2C fresh with our own context (like BLE example) */
    initStatus = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
                                  &CYBSP_I2C_CONTROLLER_config,
                                  &capsense_i2c_context);

    if (initStatus != CY_SCB_I2C_SUCCESS) {
        printf("[CAPSENSE] I2C Init failed: %d\r\n", (int)initStatus);
        capsense_initialized = true;
        capsense_hw_detected = false;
        return false;
    }

    /* Enable I2C master hardware */
    Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

    printf("[CAPSENSE] I2C initialized successfully\r\n");

    capsense_initialized = true;
    capsense_hw_detected = true;  /* Assume connected - will detect on read */

    return true;
#else
    printf("[CAPSENSE] I2C not enabled in BSP\r\n");
    capsense_initialized = true;
    capsense_hw_detected = false;
    return false;
#endif
}

bool aic_capsense_read(aic_capsense_data_t *data)
{
    if (data == NULL) {
        return false;
    }

    /* Default values (no touch) */
    data->btn0_pressed = false;
    data->btn1_pressed = false;
    data->slider_pos = 0;
    data->slider_active = false;

    if (!capsense_initialized) {
        aic_capsense_init();
    }

    if (!capsense_hw_detected) {
        return false;
    }

#if HW_CAPSENSE_AVAILABLE
    uint8_t buffer[CAPSENSE_READ_SIZE] = {0};
    uint8_t *pData = &buffer[0];
    uint8_t size = CAPSENSE_READ_SIZE;
    cy_en_scb_i2c_command_t ack = CY_SCB_I2C_ACK;
    cy_en_scb_i2c_status_t status;

    /* Check if I2C bus is idle before starting transaction */
    /* Use our own context (like BLE example), not the touch context */
    status = (capsense_i2c_context.state == CY_SCB_I2C_IDLE) ?
             Cy_SCB_I2C_MasterSendStart(CYBSP_I2C_CONTROLLER_HW,
                                        CAPSENSE_I2C_SLAVE_ADDR,
                                        CY_SCB_I2C_READ_XFER,
                                        CAPSENSE_I2C_TIMEOUT_MS,
                                        &capsense_i2c_context) :
             Cy_SCB_I2C_MasterSendReStart(CYBSP_I2C_CONTROLLER_HW,
                                          CAPSENSE_I2C_SLAVE_ADDR,
                                          CY_SCB_I2C_READ_XFER,
                                          CAPSENSE_I2C_TIMEOUT_MS,
                                          &capsense_i2c_context);

    if (status == CY_SCB_I2C_SUCCESS) {
        /* Read bytes one by one - exactly like BLE example */
        while (size > 0) {
            if (size == 1) {
                ack = CY_SCB_I2C_NAK;  /* NAK on last byte */
            }
            status = Cy_SCB_I2C_MasterReadByte(CYBSP_I2C_CONTROLLER_HW,
                                               ack,
                                               pData,
                                               CAPSENSE_I2C_TIMEOUT_MS,
                                               &capsense_i2c_context);
            if (status != CY_SCB_I2C_SUCCESS) {
                break;
            }
            ++pData;
            --size;
        }
    }

    /* Send stop - always after each transaction (like BLE example) */
    Cy_SCB_I2C_MasterSendStop(CYBSP_I2C_CONTROLLER_HW,
                              CAPSENSE_I2C_TIMEOUT_MS,
                              &capsense_i2c_context);

    if (status != CY_SCB_I2C_SUCCESS) {
        return false;
    }

    /* DEBUG: Print raw I2C buffer (every 50 reads to avoid spam) */
    static uint32_t debug_count = 0;
    if ((debug_count++ % 50) == 0) {
        printf("[I2C RAW] buf[0]=0x%02X buf[1]=0x%02X buf[2]=0x%02X\r\n",
               buffer[0], buffer[1], buffer[2]);
    }

    /* Parse CAPSENSE data - exactly like BLE example
     * Subtract ASCII offset for button values
     */
    buffer[0] -= CAPSENSE_ASCII_OFFSET;  /* BTN0 */
    buffer[1] -= CAPSENSE_ASCII_OFFSET;  /* BTN1 */
    /* buffer[2] is raw slider value, no conversion needed */

    /* Button 0: pressed if value != 0 */
    data->btn0_pressed = (buffer[0] != CAPSENSE_BTN0_NOT_PRESSED);

    /* Button 1: pressed if value != 1 (inverted logic from PSoC 4000T) */
    data->btn1_pressed = (buffer[1] != CAPSENSE_BTN1_NOT_PRESSED);

    /* Slider: position 0-100, 0 means no touch */
    data->slider_pos = buffer[2];
    data->slider_active = (buffer[2] != CAPSENSE_SLIDER_NO_TOUCH);

    return true;
#else
    return false;
#endif
}

bool aic_capsense_is_available(void)
{
    if (!capsense_initialized) {
        aic_capsense_init();
    }
    return capsense_hw_detected;
}

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

const char* aic_adc_channel_name(aic_adc_channel_t channel)
{
    if (channel >= AIC_ADC_CH_COUNT) {
        return "Unknown";
    }
    return adc_channel_names[channel];
}

float aic_adc_raw_to_voltage(uint16_t raw, aic_adc_resolution_t resolution, uint16_t vref_mv)
{
    uint16_t max_val;

    switch (resolution) {
        case AIC_ADC_RES_8BIT:
            max_val = ADC_MAX_8BIT;
            break;
        case AIC_ADC_RES_10BIT:
            max_val = ADC_MAX_10BIT;
            break;
        case AIC_ADC_RES_12BIT:
        default:
            max_val = ADC_MAX_12BIT;
            break;
    }

    return ((float)raw * vref_mv) / (max_val * 1000.0f);
}

void aic_sensors_print_status(void)
{
    printf("\r\n=== Sensor Status ===\r\n");
    printf("Initialized: %s\r\n", sensors_initialized ? "Yes" : "No");
    printf("Mode: %s\r\n", simulation_mode ? "Simulation" : "Hardware");

    printf("\r\nADC:\r\n");
    printf("  Initialized: %s\r\n", adc_initialized ? "Yes" : "No");
    printf("  Resolution: %d-bit\r\n", current_resolution);
    printf("  VREF: %u mV\r\n", current_vref_mv);

    printf("\r\nIMU:\r\n");
    printf("  Initialized: %s\r\n", imu_initialized ? "Yes" : "No");
    printf("  Orientation: %s\r\n", aic_imu_orientation_name(aic_imu_get_orientation()));

    float ax, ay, az, gx, gy, gz;
    if (aic_imu_read_accel(&ax, &ay, &az)) {
        printf("  Accel: X=%.2f, Y=%.2f, Z=%.2f g\r\n", ax, ay, az);
    }
    if (aic_imu_read_gyro(&gx, &gy, &gz)) {
        printf("  Gyro:  X=%.2f, Y=%.2f, Z=%.2f deg/s\r\n", gx, gy, gz);
    }

    printf("======================\r\n");
}

/* [] END OF FILE */
