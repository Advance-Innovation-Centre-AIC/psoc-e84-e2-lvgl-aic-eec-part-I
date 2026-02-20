/*******************************************************************************
 * File: cm33_ipc_pipe.c
 * Description: CM33-NS IPC Pipe implementation
 *
 * This module implements IPC communication from CM33-NS to CM55 core
 * using Infineon IPC Pipe library.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#include "cm33_ipc_pipe.h"
#include "../../shared/include/ipc_communication.h"
#include "cy_ipc_pipe.h"
#include "cy_syslib.h"
#include <string.h>
#include <stdio.h>

/* WiFi task for routing WiFi IPC commands */
#include "../source/wifi_task.h"

/* BT task for routing BT IPC commands */
#include "../source/bt_task.h"

/* CAPSENSE module for routing CAPSENSE IPC commands */
#include "../source/capsense_task.h"

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* Message buffer in shared memory */
CY_SECTION_SHAREDMEM static ipc_msg_t cm33_tx_msg;

/* Receive state */
static volatile bool msg_received = false;
static ipc_msg_t rx_buffer;

/* Initialization state */
static bool ipc_initialized = false;

/* Callback registration */
static cm33_ipc_rx_callback_t rx_callback = NULL;
static void *rx_callback_user_data = NULL;

/* Statistics */
static uint32_t tx_count = 0;
static uint32_t rx_count = 0;
static uint32_t error_count = 0;

/*******************************************************************************
 * IPC Callback (called from ISR context)
 ******************************************************************************/

static void cm33_ipc_callback(uint32_t *msgData)
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

cy_en_ipc_pipe_status_t cm33_ipc_init(void)
{
    cy_en_ipc_pipe_status_t status;

    if (ipc_initialized) {
        return CY_IPC_PIPE_SUCCESS;
    }

    /* Initialize IPC Pipe infrastructure (Semaphores + Config + Init) */
    cm33_ipc_communication_setup();

    /* Delay for IPC hardware stabilization (matches reference project) */
    Cy_SysLib_Delay(50U);

    /* Register callback for incoming messages */
    status = Cy_IPC_Pipe_RegisterCallback(
        CM33_IPC_PIPE_EP_ADDR,
        cm33_ipc_callback,
        CM33_IPC_PIPE_CLIENT_ID
    );

    if (status == CY_IPC_PIPE_SUCCESS) {
        ipc_initialized = true;
    }

    return status;
}

void cm33_ipc_deinit(void)
{
    if (!ipc_initialized) {
        return;
    }

    ipc_initialized = false;
}

bool cm33_ipc_is_init(void)
{
    return ipc_initialized;
}

/*******************************************************************************
 * Send Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm33_ipc_send(const ipc_msg_t *msg)
{
    if (!ipc_initialized) {
        error_count++;
        return CY_IPC_PIPE_ERROR_NO_INTR;
    }

    /* Copy message to shared memory buffer */
    memcpy(&cm33_tx_msg, msg, sizeof(ipc_msg_t));
    cm33_tx_msg.client_id = CM55_IPC_PIPE_CLIENT_ID;
    cm33_tx_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;

    cy_en_ipc_pipe_status_t status = Cy_IPC_Pipe_SendMessage(
        CM55_IPC_PIPE_EP_ADDR,
        CM33_IPC_PIPE_EP_ADDR,
        (void *)&cm33_tx_msg,
        NULL
    );

    if (status == CY_IPC_PIPE_SUCCESS) {
        tx_count++;
    } else {
        error_count++;
    }

    return status;
}

cy_en_ipc_pipe_status_t cm33_ipc_send_retry(const ipc_msg_t *msg, uint32_t max_retries)
{
    cy_en_ipc_pipe_status_t status;
    uint32_t retries = 0;
    uint32_t retry_limit = (max_retries > 0) ? max_retries : IPC_SEND_MAX_RETRIES;

    do {
        status = cm33_ipc_send(msg);
        if (status == CY_IPC_PIPE_ERROR_SEND_BUSY) {
            Cy_SysLib_Delay(IPC_SEND_RETRY_DELAY_MS);
            retries++;
        }
    } while (status == CY_IPC_PIPE_ERROR_SEND_BUSY && retries < retry_limit);

    return status;
}

cy_en_ipc_pipe_status_t cm33_ipc_send_cmd(ipc_cmd_t cmd, uint32_t value)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, cmd);
    msg.value = value;

    return cm33_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm33_ipc_send_data(ipc_cmd_t cmd, const char *data)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, cmd);

    if (data != NULL) {
        strncpy(msg.data, data, IPC_DATA_MAX_LEN - 1);
        msg.data[IPC_DATA_MAX_LEN - 1] = '\0';
    }

    return cm33_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * Receive Functions
 ******************************************************************************/

bool cm33_ipc_msg_pending(void)
{
    return msg_received;
}

bool cm33_ipc_get_msg(ipc_msg_t *msg)
{
    if (!msg_received || msg == NULL) {
        return false;
    }

    /* Disable interrupts for atomic copy */
    uint32_t irq_state = Cy_SysLib_EnterCriticalSection();
    memcpy(msg, &rx_buffer, sizeof(ipc_msg_t));
    msg_received = false;
    Cy_SysLib_ExitCriticalSection(irq_state);

    return true;
}

void cm33_ipc_register_callback(cm33_ipc_rx_callback_t callback, void *user_data)
{
    rx_callback = callback;
    rx_callback_user_data = user_data;
}

void cm33_ipc_process(void)
{
    ipc_msg_t msg;

    if (cm33_ipc_get_msg(&msg)) {
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
                    cm33_ipc_send_retry(&pong, 0);
                }
                break;

            case IPC_CMD_LOG:
            case IPC_CMD_LOG_ERROR:
            case IPC_CMD_LOG_WARN:
            case IPC_CMD_LOG_INFO:
            case IPC_CMD_LOG_DEBUG:
                /* Print log from CM55 */
                cm33_ipc_handle_log(&msg);
                break;

            case IPC_CMD_LED_SET:
            case IPC_CMD_LED_BRIGHTNESS:
                /* Handle LED control - application should handle via callback */
                break;

            case IPC_CMD_GPIO_GET:
                /* Handle GPIO read request - application should handle via callback */
                break;

            /* WiFi Commands (0xD0-0xDF) + NTP (0xF0) - Route to WiFi task */
            case IPC_CMD_WIFI_SCAN_START:
            case IPC_CMD_WIFI_CONNECT:
            case IPC_CMD_WIFI_DISCONNECT:
            case IPC_CMD_WIFI_STATUS:
            case IPC_CMD_WIFI_GET_TCPIP:
            case IPC_CMD_WIFI_GET_HARDWARE:
            case IPC_CMD_NTP_SYNC:
                wifi_task_queue_cmd(&msg);
                break;

            /* Bluetooth Commands (0xE0-0xEF) - Route to BT task */
            case IPC_CMD_BT_SCAN_START:
            case IPC_CMD_BT_CONNECT:
            case IPC_CMD_BT_DISCONNECT:
            case IPC_CMD_BT_STATUS:
            case IPC_CMD_BT_GET_HARDWARE:
                bt_task_queue_cmd(&msg);
                break;

            /* CAPSENSE request - respond with current state */
            case IPC_CMD_CAPSENSE_REQ:
                capsense_module_send_current();
                break;

            default:
                /* Unknown commands are handled by callback only */
                break;
        }
    }
}

/*******************************************************************************
 * Sensor Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm33_ipc_send_imu(const ipc_imu_data_t *data)
{
    if (data == NULL) {
        return CY_IPC_PIPE_ERROR_BAD_HANDLE;
    }

    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_IMU_DATA);
    memcpy(msg.data, data, sizeof(ipc_imu_data_t));

    return cm33_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm33_ipc_send_adc(const ipc_adc_data_t *data)
{
    if (data == NULL) {
        return CY_IPC_PIPE_ERROR_BAD_HANDLE;
    }

    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_ADC_DATA);
    memcpy(msg.data, data, sizeof(ipc_adc_data_t));

    return cm33_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * GPIO Functions
 ******************************************************************************/

cy_en_ipc_pipe_status_t cm33_ipc_send_button_event(uint8_t button_id, bool pressed)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_BUTTON_EVENT);

    ipc_button_data_t btn_data = {
        .button_id = button_id,
        .pressed = pressed ? 1 : 0,
        .long_press = 0,
        .reserved = 0,
        .timestamp = 0  /* TODO: Add timestamp */
    };
    memcpy(msg.data, &btn_data, sizeof(ipc_button_data_t));

    return cm33_ipc_send_retry(&msg, 0);
}

cy_en_ipc_pipe_status_t cm33_ipc_send_led_state(uint8_t led_id, bool state)
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

    return cm33_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * Logging Functions
 ******************************************************************************/

void cm33_ipc_handle_log(const ipc_msg_t *msg)
{
    const char *level = "LOG";

    switch (msg->cmd) {
        case IPC_CMD_LOG_ERROR: level = "ERROR"; break;
        case IPC_CMD_LOG_WARN:  level = "WARN";  break;
        case IPC_CMD_LOG_INFO:  level = "INFO";  break;
        case IPC_CMD_LOG_DEBUG: level = "DEBUG"; break;
        default: break;
    }

    printf("[CM55/%s] %s\r\n", level, msg->data);
}

/*******************************************************************************
 * Statistics
 ******************************************************************************/

void cm33_ipc_get_stats(uint32_t *tx, uint32_t *rx, uint32_t *errors)
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

void cm33_ipc_reset_stats(void)
{
    tx_count = 0;
    rx_count = 0;
    error_count = 0;
}
