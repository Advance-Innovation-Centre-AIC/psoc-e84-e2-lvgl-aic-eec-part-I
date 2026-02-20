/*******************************************************************************
 * File: cm55_ipc_pipe.c
 * Description: CM55 IPC Pipe implementation
 *
 * This module implements IPC communication from CM55 to CM33-NS core
 * using Infineon IPC Pipe library with FreeRTOS integration.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#include "cm55_ipc_pipe.h"
#include "../../shared/include/ipc_communication.h"
#include "cy_ipc_pipe.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define CM55_IPC_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 2)
#define CM55_IPC_TASK_PRIORITY      (3)
#define CM55_IPC_PROCESS_DELAY_MS   (10)

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* Message buffer in shared memory */
CY_SECTION_SHAREDMEM static ipc_msg_t cm55_tx_msg;

/* Receive state */
static volatile bool msg_received = false;
static ipc_msg_t rx_buffer;
static SemaphoreHandle_t rx_mutex = NULL;

/* Initialization state */
static bool ipc_initialized = false;

/* Callback registration */
static cm55_ipc_rx_callback_t rx_callback = NULL;
static void *rx_callback_user_data = NULL;

/* Statistics */
static uint32_t tx_count = 0;
static uint32_t rx_count = 0;
static uint32_t error_count = 0;

/* FreeRTOS task handle */
static TaskHandle_t ipc_task_handle = NULL;

/*******************************************************************************
 * IPC Callback (called from ISR context)
 ******************************************************************************/

static void cm55_ipc_callback(uint32_t *msgData)
{
    ipc_msg_t *msg = (ipc_msg_t *)msgData;

    /* Copy to buffer (ISR-safe) */
    memcpy(&rx_buffer, msg, sizeof(ipc_msg_t));
    msg_received = true;
    rx_count++;
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm55_ipc_init(void)
{
    cy_en_ipc_pipe_status_t status;

    if (ipc_initialized) {
        return CY_IPC_PIPE_SUCCESS;
    }

    /* Create mutex for thread-safe access */
    rx_mutex = xSemaphoreCreateMutex();
    if (rx_mutex == NULL) {
        printf("[CM55 IPC] Failed to create mutex\n");
        return CY_IPC_PIPE_ERROR_NO_INTR;
    }

    /* Initialize IPC Pipe infrastructure (Config + Init) */
    cm55_ipc_communication_setup();

    /* Register callback for incoming messages */
    status = Cy_IPC_Pipe_RegisterCallback(
        CM55_IPC_PIPE_EP_ADDR,
        cm55_ipc_callback,
        CM55_IPC_PIPE_CLIENT_ID
    );

    if (status == CY_IPC_PIPE_SUCCESS) {
        ipc_initialized = true;
        printf("[CM55 IPC] Initialized successfully\n");
    } else {
        printf("[CM55 IPC] Init failed: %d\n", status);
        vSemaphoreDelete(rx_mutex);
        rx_mutex = NULL;
    }

    return status;
}

void cm55_ipc_deinit(void)
{
    if (!ipc_initialized) {
        return;
    }

    cm55_ipc_delete_task();

    if (rx_mutex != NULL) {
        vSemaphoreDelete(rx_mutex);
        rx_mutex = NULL;
    }

    ipc_initialized = false;
    printf("[CM55 IPC] Deinitialized\n");
}

bool cm55_ipc_is_init(void)
{
    return ipc_initialized;
}

/*******************************************************************************
 * Send Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm55_ipc_send(const ipc_msg_t *msg)
{
    if (!ipc_initialized) {
        error_count++;
        return CY_IPC_PIPE_ERROR_NO_INTR;
    }

    /* Copy message to shared memory buffer */
    memcpy(&cm55_tx_msg, msg, sizeof(ipc_msg_t));
    cm55_tx_msg.client_id = CM33_IPC_PIPE_CLIENT_ID;
    cm55_tx_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP2;

    cy_en_ipc_pipe_status_t status = Cy_IPC_Pipe_SendMessage(
        CM33_IPC_PIPE_EP_ADDR,
        CM55_IPC_PIPE_EP_ADDR,
        (void *)&cm55_tx_msg,
        NULL
    );

    if (status == CY_IPC_PIPE_SUCCESS) {
        tx_count++;
    } else {
        error_count++;
    }

    return status;
}

cy_en_ipc_pipe_status_t cm55_ipc_send_retry(const ipc_msg_t *msg, uint32_t max_retries)
{
    cy_en_ipc_pipe_status_t status;
    uint32_t retries = 0;
    uint32_t retry_limit = (max_retries > 0) ? max_retries : IPC_SEND_MAX_RETRIES;

    do {
        status = cm55_ipc_send(msg);
        if (status == CY_IPC_PIPE_ERROR_SEND_BUSY) {
            vTaskDelay(pdMS_TO_TICKS(IPC_SEND_RETRY_DELAY_MS));
            retries++;
        }
    } while (status == CY_IPC_PIPE_ERROR_SEND_BUSY && retries < retry_limit);

    return status;
}

cy_en_ipc_pipe_status_t cm55_ipc_send_cmd(ipc_cmd_t cmd, uint32_t value)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, cmd);
    msg.value = value;

    return cm55_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm55_ipc_send_data(ipc_cmd_t cmd, const char *data)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, cmd);

    if (data != NULL) {
        strncpy(msg.data, data, IPC_DATA_MAX_LEN - 1);
        msg.data[IPC_DATA_MAX_LEN - 1] = '\0';
    }

    return cm55_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * Receive Functions
 ******************************************************************************/

bool cm55_ipc_msg_pending(void)
{
    return msg_received;
}

bool cm55_ipc_get_msg(ipc_msg_t *msg)
{
    if (!msg_received || msg == NULL) {
        return false;
    }

    if (xSemaphoreTake(rx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        memcpy(msg, &rx_buffer, sizeof(ipc_msg_t));
        msg_received = false;
        xSemaphoreGive(rx_mutex);
        return true;
    }

    return false;
}

void cm55_ipc_register_callback(cm55_ipc_rx_callback_t callback, void *user_data)
{
    rx_callback = callback;
    rx_callback_user_data = user_data;
}

void cm55_ipc_process(void)
{
    ipc_msg_t msg;

    if (cm55_ipc_get_msg(&msg)) {
        /* Call registered callback first */
        if (rx_callback != NULL) {
            rx_callback(&msg, rx_callback_user_data);
        }

        /* Handle built-in commands */
        switch (msg.cmd) {
            case IPC_CMD_PING:
                /* Respond with PONG */
                {
                    ipc_msg_t pong;
                    IPC_MSG_INIT(&pong, IPC_CMD_PONG);
                    pong.value = msg.value;
                    cm55_ipc_send_retry(&pong, 0);
                }
                break;

            case IPC_CMD_LOG:
                /* Print log from CM33 */
                printf("[CM33] %s", msg.data);
                break;

            case IPC_CMD_SENSOR_DATA:
                /* Handle sensor data - forward to callback */
                break;

            case IPC_CMD_BUTTON:
            case IPC_CMD_BUTTON_EVENT:
                /* Handle button event - forward to callback */
                break;

            default:
                /* Unknown commands are handled by callback only */
                break;
        }
    }
}

/*******************************************************************************
 * Logging Functions
 ******************************************************************************/

void cm55_ipc_log(const char *fmt, ...)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_LOG);

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.data, IPC_DATA_MAX_LEN, fmt, args);
    va_end(args);

    cm55_ipc_send_retry(&msg, 0);
}

void cm55_ipc_log_level(ipc_cmd_t level, const char *fmt, ...)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, level);

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.data, IPC_DATA_MAX_LEN, fmt, args);
    va_end(args);

    cm55_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * Sensor Data Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm55_ipc_send_imu(const ipc_imu_data_t *data)
{
    if (data == NULL) {
        return CY_IPC_PIPE_ERROR_BAD_HANDLE;
    }

    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_IMU_DATA);
    memcpy(msg.data, data, sizeof(ipc_imu_data_t));

    return cm55_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm55_ipc_send_adc(const ipc_adc_data_t *data)
{
    if (data == NULL) {
        return CY_IPC_PIPE_ERROR_BAD_HANDLE;
    }

    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_ADC_DATA);
    memcpy(msg.data, data, sizeof(ipc_adc_data_t));

    return cm55_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm55_ipc_request_sensor(ipc_cmd_t sensor_type)
{
    return cm55_ipc_send_cmd(IPC_CMD_SENSOR_REQ, (uint32_t)sensor_type);
}

/*******************************************************************************
 * GPIO Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm55_ipc_set_led(uint8_t led_id, bool state)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_LED_SET);

    ipc_led_data_t led_data = {
        .led_id = led_id,
        .state = state ? 1 : 0,
        .brightness = 100,
        .reserved = 0
    };
    memcpy(msg.data, &led_data, sizeof(ipc_led_data_t));

    return cm55_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm55_ipc_set_led_brightness(uint8_t led_id, uint8_t brightness)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_LED_BRIGHTNESS);

    ipc_led_data_t led_data = {
        .led_id = led_id,
        .state = 1,
        .brightness = brightness,
        .reserved = 0
    };
    memcpy(msg.data, &led_data, sizeof(ipc_led_data_t));

    return cm55_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm55_ipc_request_button(uint8_t button_id)
{
    return cm55_ipc_send_cmd(IPC_CMD_GPIO_GET, button_id);
}

/*******************************************************************************
 * FreeRTOS Task
 ******************************************************************************/

static void cm55_ipc_task(void *param)
{
    (void)param;

    printf("[CM55 IPC] Task started\n");

    while (1) {
        cm55_ipc_process();
        vTaskDelay(pdMS_TO_TICKS(CM55_IPC_PROCESS_DELAY_MS));
    }
}

void cm55_ipc_create_task(void)
{
    if (ipc_task_handle != NULL) {
        return;  /* Task already created */
    }

    BaseType_t result = xTaskCreate(
        cm55_ipc_task,
        "IPC_RX",
        CM55_IPC_TASK_STACK_SIZE,
        NULL,
        CM55_IPC_TASK_PRIORITY,
        &ipc_task_handle
    );

    if (result != pdPASS) {
        printf("[CM55 IPC] Failed to create task\n");
        ipc_task_handle = NULL;
    }
}

void cm55_ipc_delete_task(void)
{
    if (ipc_task_handle != NULL) {
        vTaskDelete(ipc_task_handle);
        ipc_task_handle = NULL;
        printf("[CM55 IPC] Task deleted\n");
    }
}

/*******************************************************************************
 * Statistics and Debugging
 ******************************************************************************/

void cm55_ipc_get_stats(uint32_t *tx, uint32_t *rx, uint32_t *errors)
{
    if (tx != NULL) {
        *tx = tx_count;
    }
    if (rx != NULL) {
        *rx = rx_count;
    }
    if (errors != NULL) {
        *errors = error_count;
    }
}

void cm55_ipc_reset_stats(void)
{
    tx_count = 0;
    rx_count = 0;
    error_count = 0;
}
