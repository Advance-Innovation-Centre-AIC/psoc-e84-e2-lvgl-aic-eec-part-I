/*******************************************************************************
 * File: bt_task.c
 * Description: Bluetooth task for CM33-NS - Implementation
 *
 * Manages WICED BT Stack initialization, BLE scanning,
 * and processes BT IPC commands from CM55.
 *
 * Based on IoT Gateway reference project (btstack-integration).
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#include "bt_task.h"
#include "../../shared/bt_shared.h"
#include "../ipc/cm33_ipc_pipe.h"

/* WICED BT Stack */
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Standard */
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Local BT Configuration
 *
 * This project previously depended on auto-generated BT config headers
 * (cycfg_bt_settings.h/cycfg_gap.h/cycfg_gatt_db.h). Those files are not
 * present in this repository snapshot, so provide a minimal scanner-oriented
 * runtime configuration here.
 ******************************************************************************/

static uint8_t bt_device_name[] = "AIC-EEC CM33-NS";

static const wiced_bt_cfg_ble_scan_settings_t bt_scan_settings =
{
    .scan_mode = BTM_BLE_SCAN_MODE_PASSIVE,
    .high_duty_scan_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_INTERVAL,
    .high_duty_scan_window = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_WINDOW,
    .high_duty_scan_duration = 5,
    .low_duty_scan_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_SCAN_INTERVAL,
    .low_duty_scan_window = WICED_BT_CFG_DEFAULT_LOW_DUTY_SCAN_WINDOW,
    .low_duty_scan_duration = 60,
    .high_duty_conn_scan_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_CONN_SCAN_INTERVAL,
    .high_duty_conn_scan_window = WICED_BT_CFG_DEFAULT_HIGH_DUTY_CONN_SCAN_WINDOW,
    .high_duty_conn_duration = 30,
    .low_duty_conn_scan_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_CONN_SCAN_INTERVAL,
    .low_duty_conn_scan_window = WICED_BT_CFG_DEFAULT_LOW_DUTY_CONN_SCAN_WINDOW,
    .low_duty_conn_duration = 30,
    .conn_min_interval = WICED_BT_CFG_DEFAULT_CONN_MIN_INTERVAL,
    .conn_max_interval = WICED_BT_CFG_DEFAULT_CONN_MAX_INTERVAL,
    .conn_latency = WICED_BT_CFG_DEFAULT_CONN_LATENCY,
    .conn_supervision_timeout = WICED_BT_CFG_DEFAULT_CONN_SUPERVISION_TIMEOUT
};

static const wiced_bt_cfg_ble_advert_settings_t bt_adv_settings =
{
    .channel_map = BTM_BLE_ADVERT_CHNL_37 | BTM_BLE_ADVERT_CHNL_38 | BTM_BLE_ADVERT_CHNL_39,
    .high_duty_min_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_ADV_MIN_INTERVAL,
    .high_duty_max_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_ADV_MAX_INTERVAL,
    .high_duty_duration = 30,
    .low_duty_min_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_ADV_MIN_INTERVAL,
    .low_duty_max_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_ADV_MAX_INTERVAL,
    .low_duty_duration = 0,
    .high_duty_directed_min_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_DIRECTED_ADV_MIN_INTERVAL,
    .high_duty_directed_max_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_DIRECTED_ADV_MAX_INTERVAL,
    .low_duty_directed_min_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_DIRECTED_ADV_MIN_INTERVAL,
    .low_duty_directed_max_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_DIRECTED_ADV_MAX_INTERVAL,
    .low_duty_directed_duration = 30,
    .high_duty_nonconn_min_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_NONCONN_ADV_MIN_INTERVAL,
    .high_duty_nonconn_max_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_NONCONN_ADV_MAX_INTERVAL,
    .high_duty_nonconn_duration = 30,
    .low_duty_nonconn_min_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_NONCONN_ADV_MIN_INTERVAL,
    .low_duty_nonconn_max_interval = WICED_BT_CFG_DEFAULT_LOW_DUTY_NONCONN_ADV_MAX_INTERVAL,
    .low_duty_nonconn_duration = 0
};

static const wiced_bt_cfg_ble_t bt_ble_cfg =
{
    .ble_max_simultaneous_links = 1,
    .ble_max_rx_pdu_size = 65,
    .appearance = APPEARANCE_GENERIC_TAG,
    .rpa_refresh_timeout = WICED_BT_CFG_DEFAULT_RANDOM_ADDRESS_CHANGE_TIMEOUT,
    .host_addr_resolution_db_size = 3,
    .p_ble_scan_cfg = &bt_scan_settings,
    .p_ble_advert_cfg = &bt_adv_settings,
    .default_ble_power_level = 0
};

static const wiced_bt_cfg_gatt_t bt_gatt_cfg =
{
    .max_db_service_modules = 0,
    .max_eatt_bearers = 0
};

static const wiced_bt_cfg_settings_t cy_bt_cfg_settings =
{
    .device_name = bt_device_name,
    .security_required = BTM_SEC_BEST_EFFORT,
    .p_br_cfg = NULL,
    .p_ble_cfg = &bt_ble_cfg,
    .p_gatt_cfg = &bt_gatt_cfg,
    .p_isoc_cfg = NULL,
    .p_l2cap_app_cfg = NULL
};

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* Command queue for IPC messages */
static QueueHandle_t bt_cmd_queue = NULL;

/* BT state */
static bt_state_t bt_state = BT_STATE_OFF;
static bool bt_initialized = false;
static bool bt_scanning = false;

/* Scan results buffer */
static ipc_bt_device_t scan_results[BT_SCAN_MAX_RESULTS];
static uint8_t scan_result_count = 0;

/* Task handle for notifications */
static TaskHandle_t bt_task_handle = NULL;

/* Connected device info */
static uint16_t bt_connection_id = 0;

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/

static wiced_result_t bt_management_callback(wiced_bt_management_evt_t event,
                                              wiced_bt_management_evt_data_t *p_event_data);
static wiced_bt_gatt_status_t bt_gatt_callback(wiced_bt_gatt_evt_t event,
                                                wiced_bt_gatt_event_data_t *p_event_data);
static void handle_bt_scan(void);
static void handle_bt_get_status(void);
static void handle_bt_get_hardware(void);
static void process_bt_command(const ipc_msg_t *msg);

/*******************************************************************************
 * BT Management Callback
 ******************************************************************************/

static wiced_result_t bt_management_callback(wiced_bt_management_evt_t event,
                                              wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;

    switch (event)
    {
        case BTM_ENABLED_EVT:
            if (p_event_data->enabled.status == WICED_BT_SUCCESS)
            {
                printf("[CM33-BT] Stack enabled successfully\r\n");
                bt_initialized = true;
                bt_state = BT_STATE_READY;

                /* Register GATT callback */
                wiced_bt_gatt_register(bt_gatt_callback);

                /* Notify task that stack is ready */
                if (bt_task_handle != NULL)
                {
                    xTaskNotifyGive(bt_task_handle);
                }
            }
            else
            {
                printf("[CM33-BT] Stack init failed: %d\r\n",
                       (int)p_event_data->enabled.status);
                bt_state = BT_STATE_ERROR;
            }
            break;

        case BTM_DISABLED_EVT:
            printf("[CM33-BT] Stack disabled\r\n");
            bt_initialized = false;
            bt_state = BT_STATE_OFF;
            break;

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
            /* Not advertising in scanner mode, but handle gracefully */
            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            printf("[CM33-BT] Connection param update\r\n");
            break;

        case BTM_BLE_PHY_UPDATE_EVT:
            printf("[CM33-BT] PHY update: TX=%d RX=%d\r\n",
                   (int)p_event_data->ble_phy_update_event.tx_phy,
                   (int)p_event_data->ble_phy_update_event.rx_phy);
            break;

        default:
            printf("[CM33-BT] Unhandled event: %d\r\n", (int)event);
            break;
    }

    return result;
}

/*******************************************************************************
 * GATT Callback (minimal for scanner)
 ******************************************************************************/

static wiced_bt_gatt_status_t bt_gatt_callback(wiced_bt_gatt_evt_t event,
                                                wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            if (p_event_data->connection_status.connected)
            {
                bt_connection_id = p_event_data->connection_status.conn_id;
                bt_state = BT_STATE_CONNECTED;
                printf("[CM33-BT] Device connected (conn_id=%d)\r\n",
                       (int)bt_connection_id);

                /* Send connected notification to CM55 */
                ipc_msg_t resp;
                IPC_MSG_INIT(&resp, IPC_CMD_BT_CONNECTED);
                memcpy(resp.data, p_event_data->connection_status.bd_addr, BT_ADDR_LEN);
                cm33_ipc_send_retry(&resp, 0);
            }
            else
            {
                printf("[CM33-BT] Device disconnected (conn_id=%d)\r\n",
                       (int)bt_connection_id);
                bt_connection_id = 0;
                bt_state = BT_STATE_READY;

                /* Send disconnected notification to CM55 */
                cm33_ipc_send_cmd(IPC_CMD_BT_DISCONNECTED, 0);
            }
            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            /* Minimal handling - no custom attributes in scanner mode */
            break;

        default:
            break;
    }

    return status;
}

/*******************************************************************************
 * BLE Scan Callback
 ******************************************************************************/

static void ble_scan_result_callback(wiced_bt_ble_scan_results_t *p_scan_result,
                                      uint8_t *p_adv_data)
{
    if (p_scan_result == NULL)
    {
        /* Scan complete */
        printf("[CM33-BT] Scan complete, %d devices found\r\n",
               (int)scan_result_count);
        bt_scanning = false;
        bt_state = BT_STATE_READY;

        /* Notify task */
        if (bt_task_handle != NULL)
        {
            xTaskNotifyGive(bt_task_handle);
        }
        return;
    }

    /* Check for duplicates */
    for (uint8_t i = 0; i < scan_result_count; i++)
    {
        if (memcmp(scan_results[i].addr, p_scan_result->remote_bd_addr, BT_ADDR_LEN) == 0)
        {
            /* Update RSSI for existing device */
            scan_results[i].rssi = p_scan_result->rssi;
            return;
        }
    }

    /* Add new device if space available */
    if (scan_result_count < BT_SCAN_MAX_RESULTS)
    {
        ipc_bt_device_t *dev = &scan_results[scan_result_count];
        memset(dev, 0, sizeof(ipc_bt_device_t));

        memcpy(dev->addr, p_scan_result->remote_bd_addr, BT_ADDR_LEN);
        dev->addr_type = p_scan_result->ble_addr_type;
        dev->rssi = p_scan_result->rssi;
        dev->device_type = BT_DEVICE_TYPE_LE;

        /* Set connectable flag based on advertising type */
        if (p_scan_result->ble_evt_type == BTM_BLE_EVT_CONNECTABLE_ADVERTISEMENT ||
            p_scan_result->ble_evt_type == BTM_BLE_EVT_CONNECTABLE_DIRECTED_ADVERTISEMENT)
        {
            dev->flags |= 0x01;  /* connectable */
        }

        /* Extract device name from advertisement data */
        if (p_adv_data != NULL)
        {
            uint8_t adv_name_len = 0;
            uint8_t *p_name = wiced_bt_ble_check_advertising_data(
                p_adv_data, BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, &adv_name_len);

            if (p_name == NULL)
            {
                p_name = wiced_bt_ble_check_advertising_data(
                    p_adv_data, BTM_BLE_ADVERT_TYPE_NAME_SHORT, &adv_name_len);
            }

            if (p_name != NULL && adv_name_len > 0)
            {
                uint8_t copy_len = (adv_name_len < BT_DEVICE_NAME_MAX_LEN - 1) ?
                                    adv_name_len : (BT_DEVICE_NAME_MAX_LEN - 1);
                memcpy(dev->name, p_name, copy_len);
                dev->name[copy_len] = '\0';
            }
        }

        scan_result_count++;
    }
}

/*******************************************************************************
 * Handle BLE Scan Command
 ******************************************************************************/

static void handle_bt_scan(void)
{
    if (!bt_initialized)
    {
        printf("[CM33-BT] Cannot scan - stack not ready\r\n");
        cm33_ipc_send_cmd(IPC_CMD_BT_ERROR, BT_ERR_NOT_READY);
        return;
    }

    /* Stop any ongoing scan */
    if (bt_scanning)
    {
        wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
        bt_scanning = false;
    }

    /* Reset scan results */
    scan_result_count = 0;
    memset(scan_results, 0, sizeof(scan_results));

    printf("[CM33-BT] Starting BLE scan (%d sec)...\r\n", BT_SCAN_DURATION_SEC);
    bt_state = BT_STATE_SCANNING;
    bt_scanning = true;

    /* Start BLE observation (scan) */
    wiced_result_t result = wiced_bt_ble_observe(
        WICED_TRUE,
        BT_SCAN_DURATION_SEC,
        ble_scan_result_callback
    );

    if (result != WICED_BT_SUCCESS)
    {
        printf("[CM33-BT] Scan start failed: %d\r\n", (int)result);
        bt_scanning = false;
        bt_state = BT_STATE_READY;
        cm33_ipc_send_cmd(IPC_CMD_BT_ERROR, BT_ERR_SCAN_FAILED);
        return;
    }

    /* Wait for scan completion (notification from callback) */
    uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BT_SCAN_DURATION_SEC * 1000 + 2000));

    if (notified == 0)
    {
        /* Timeout - stop scan manually */
        wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
        bt_scanning = false;
        bt_state = BT_STATE_READY;
        printf("[CM33-BT] Scan timeout\r\n");
    }

    /* Send scan results one-by-one via IPC */
    printf("[CM33-BT] Sending %d scan results via IPC\r\n", (int)scan_result_count);

    for (uint8_t i = 0; i < scan_result_count; i++)
    {
        ipc_msg_t resp;
        IPC_MSG_INIT(&resp, IPC_CMD_BT_SCAN_RESULT);
        resp.value = i;  /* Device index */
        memcpy(resp.data, &scan_results[i], sizeof(ipc_bt_device_t));
        cm33_ipc_send_retry(&resp, 0);

        /* Delay between messages to avoid single-buffer overwrite on CM55 */
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* Send scan complete */
    cm33_ipc_send_cmd(IPC_CMD_BT_SCAN_COMPLETE, scan_result_count);
}

/*******************************************************************************
 * Handle BT Get Status Command
 ******************************************************************************/

static void handle_bt_get_status(void)
{
    ipc_bt_status_t status;
    memset(&status, 0, sizeof(status));

    status.state = (uint8_t)bt_state;
    status.num_connections = (bt_connection_id != 0) ? 1 : 0;
    status.is_scanning = bt_scanning ? 1 : 0;
    status.is_advertising = 0;  /* Not advertising in scanner mode */

    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_BT_STATUS);
    memcpy(resp.data, &status, sizeof(ipc_bt_status_t));
    cm33_ipc_send_retry(&resp, 0);
}

/*******************************************************************************
 * Handle BT Get Hardware Command
 ******************************************************************************/

static void handle_bt_get_hardware(void)
{
    ipc_bt_hardware_t hw;
    memset(&hw, 0, sizeof(hw));

    /* Get local BT address */
    wiced_bt_device_address_t local_addr;
    wiced_bt_dev_read_local_addr(local_addr);
    memcpy(hw.addr, local_addr, BT_ADDR_LEN);

    hw.state = (uint8_t)bt_state;
    hw.num_connections = (bt_connection_id != 0) ? 1 : 0;
    strncpy(hw.chip_name, "CYW55513", sizeof(hw.chip_name) - 1);

    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_BT_HARDWARE_INFO);
    memcpy(resp.data, &hw, sizeof(ipc_bt_hardware_t));
    cm33_ipc_send_retry(&resp, 0);
}

/*******************************************************************************
 * Process BT IPC Command
 ******************************************************************************/

static void process_bt_command(const ipc_msg_t *msg)
{
    switch (msg->cmd)
    {
        case IPC_CMD_BT_SCAN_START:
            handle_bt_scan();
            break;

        case IPC_CMD_BT_STATUS:
            handle_bt_get_status();
            break;

        case IPC_CMD_BT_GET_HARDWARE:
            handle_bt_get_hardware();
            break;

        case IPC_CMD_BT_CONNECT:
        case IPC_CMD_BT_DISCONNECT:
            /* Connection management - placeholder for future */
            printf("[CM33-BT] Connect/Disconnect not yet implemented\r\n");
            cm33_ipc_send_cmd(IPC_CMD_BT_ERROR, BT_ERR_NOT_READY);
            break;

        default:
            printf("[CM33-BT] Unknown command: 0x%02X\r\n",
                   (unsigned int)msg->cmd);
            break;
    }
}

/*******************************************************************************
 * BT Task Main Function
 ******************************************************************************/

void bt_task(void *pvParameters)
{
    (void)pvParameters;

    bt_task_handle = xTaskGetCurrentTaskHandle();

    printf("[CM33-BT] Task started\r\n");
    bt_state = BT_STATE_INITIALIZING;

    /* Create command queue */
    bt_cmd_queue = xQueueCreate(BT_CMD_QUEUE_LENGTH, sizeof(ipc_msg_t));
    if (bt_cmd_queue == NULL)
    {
        printf("[CM33-BT] FATAL: Queue creation failed\r\n");
        bt_state = BT_STATE_ERROR;
        vTaskDelete(NULL);
        return;
    }

    /***************************************************************************
     * Initialize WICED BT Stack
     ***************************************************************************/
    printf("[CM33-BT] Initializing BT stack...\r\n");

    wiced_result_t bt_result = wiced_bt_stack_init(
        bt_management_callback,
        &cy_bt_cfg_settings
    );

    if (bt_result != WICED_BT_SUCCESS)
    {
        printf("[CM33-BT] Stack init failed: %d\r\n", (int)bt_result);
        bt_state = BT_STATE_ERROR;
        cm33_ipc_send_cmd(IPC_CMD_BT_ERROR, BT_ERR_STACK_INIT);
        vTaskDelete(NULL);
        return;
    }

    printf("[CM33-BT] Waiting for stack enable...\r\n");

    /* Wait for BTM_ENABLED_EVT (notification from callback) */
    uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));
    if (notified == 0 || !bt_initialized)
    {
        printf("[CM33-BT] Stack enable timeout\r\n");
        bt_state = BT_STATE_ERROR;
        cm33_ipc_send_cmd(IPC_CMD_BT_ERROR, BT_ERR_STACK_INIT);
        vTaskDelete(NULL);
        return;
    }

    printf("[CM33-BT] Stack ready - waiting for commands\r\n");

    /***************************************************************************
     * Main command processing loop
     ***************************************************************************/
    ipc_msg_t msg;
    for (;;)
    {
        if (xQueueReceive(bt_cmd_queue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            process_bt_command(&msg);
        }
    }
}

/*******************************************************************************
 * Queue API (called from cm33_ipc_pipe.c)
 ******************************************************************************/

bool bt_task_queue_cmd(const ipc_msg_t *msg)
{
    if (bt_cmd_queue == NULL || msg == NULL)
    {
        return false;
    }

    if (xQueueSend(bt_cmd_queue, msg, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        printf("[CM33-BT] Command queue full\r\n");
        return false;
    }

    return true;
}
