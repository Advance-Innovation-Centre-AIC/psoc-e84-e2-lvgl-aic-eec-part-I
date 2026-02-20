/*******************************************************************************
 * File: wifi_task.c
 * Description: WiFi task for CM33-NS
 *
 * Implements WiFi infrastructure on CM33-NS core:
 * - SDIO interface initialization for CYW55513 WiFi+BLE combo chip
 * - WiFi Connection Manager (cy_wcm) initialization
 * - WiFi scan with results sent back to CM55 via IPC
 * - WiFi connect/disconnect with status via IPC
 * - WiFi status and TCP/IP info queries
 *
 * Based on: PSOC_Edge_Wi-Fi_Scan reference project (scan_task.c)
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#include "wifi_task.h"
#include "cybsp.h"
#include "cy_wcm.h"
#include "retarget_io_init.h"
#include "../../shared/wifi_shared.h"
#include "../ipc/cm33_ipc_pipe.h"

#include <stdio.h>
#include <string.h>

/* lwIP for NTP UDP socket */
#include "lwip/sockets.h"
#include "lwip/inet.h"

/*******************************************************************************
 * SDIO Configuration Macros
 ******************************************************************************/
#define APP_SDIO_INTERRUPT_PRIORITY         (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY    (2U)
#define APP_SDIO_FREQUENCY_HZ              (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK            (64U)

/* Deep sleep callback macros */
#define SYSPM_SKIP_MODE         (0U)
#define SYSPM_CALLBACK_ORDER    (1U)

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* FreeRTOS command queue */
static QueueHandle_t wifi_cmd_queue = NULL;

/* Task handle for scan completion notification */
static TaskHandle_t wifi_task_handle = NULL;

/* SDIO and WCM instances */
static mtb_hal_sdio_t sdio_instance;
static cy_stc_sd_host_context_t sdhc_host_context;
static cy_wcm_config_t wcm_config;

/* Forward declarations */
static void handle_ntp_sync(void);

/* WiFi state */
static bool wifi_initialized = false;
static wifi_state_t wifi_state = WIFI_STATE_DISCONNECTED;
static char connected_ssid[WIFI_SSID_MAX_LEN] = "";

/* Scan result buffer */
static ipc_wifi_network_t scan_results[WIFI_SCAN_MAX_RESULTS];
static volatile uint32_t scan_result_count = 0;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
/* SysPm callback for SDHC deep sleep */
static cy_stc_syspm_callback_params_t sdcardDSParams =
{
    .context   = &sdhc_host_context,
    .base      = CYBSP_WIFI_SDIO_HW
};

static cy_stc_syspm_callback_t sdhcDeepSleepCallbackHandler =
{
    .callback           = Cy_SD_Host_DeepSleepCallback,
    .skipMode           = SYSPM_SKIP_MODE,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &sdcardDSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};
#endif

/*******************************************************************************
 * SDIO Interrupt Handlers
 ******************************************************************************/

static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&wcm_config.wifi_host_wake_pin);
}

/*******************************************************************************
 * SDIO Initialization
 * Configures the SDIO interface for communication with CYW55513
 ******************************************************************************/
static bool app_sdio_init(void)
{
    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;

    /* SDIO interrupt configuration */
    cy_stc_sysint_t sdio_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY
    };

    /* Host wake interrupt configuration */
    cy_stc_sysint_t host_wake_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
        .intrPriority = APP_HOST_WAKE_INTERRUPT_PRIORITY
    };

    /* Initialize the SDIO interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&sdio_intr_cfg, sdio_interrupt_handler))
    {
        printf("[CM33-WiFi] SDIO interrupt init failed\r\n");
        return false;
    }
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO using HAL */
    result = mtb_hal_sdio_setup(&sdio_instance,
                                 &CYBSP_WIFI_SDIO_sdio_hal_config,
                                 NULL, &sdhc_host_context);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("[CM33-WiFi] SDIO setup failed: 0x%08X\r\n", (unsigned int)result);
        return false;
    }

    /* Initialize and enable SD Host */
    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW,
                     CYBSP_WIFI_SDIO_sdio_hal_config.host_config,
                     &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    /* Configure SDIO frequency and block size */
    sdio_hal_cfg.frequencyhal_hz = APP_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = SDHC_SDIO_64BYTES_BLOCK;
    mtb_hal_sdio_configure(&sdio_instance, &sdio_hal_cfg);

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
    Cy_SysPm_RegisterCallback(&sdhcDeepSleepCallbackHandler);
#endif

    /* Setup GPIO for WiFi WL REG ON */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin,
                        CYBSP_WIFI_WL_REG_ON_PORT_NUM,
                        CYBSP_WIFI_WL_REG_ON_PIN);

    /* Setup GPIO for WiFi HOST WAKE PIN */
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin,
                        CYBSP_WIFI_HOST_WAKE_PORT_NUM,
                        CYBSP_WIFI_HOST_WAKE_PIN);

    /* Initialize the Host wakeup interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&host_wake_intr_cfg,
                                              host_wake_interrupt_handler))
    {
        printf("[CM33-WiFi] Host wake interrupt init failed\r\n");
        return false;
    }
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);

    printf("[CM33-WiFi] SDIO initialized (4-bit, 25MHz)\r\n");
    return true;
}

/*******************************************************************************
 * WiFi Security Mapping: cy_wcm → wifi_shared.h
 ******************************************************************************/
static uint8_t map_wcm_security(cy_wcm_security_t sec)
{
    switch (sec)
    {
        case CY_WCM_SECURITY_OPEN:
            return WIFI_SECURITY_OPEN;
        case CY_WCM_SECURITY_WEP_PSK:
        case CY_WCM_SECURITY_WEP_SHARED:
            return WIFI_SECURITY_WEP;
        case CY_WCM_SECURITY_WPA_TKIP_PSK:
        case CY_WCM_SECURITY_WPA_AES_PSK:
        case CY_WCM_SECURITY_WPA_MIXED_PSK:
            return WIFI_SECURITY_WPA;
        case CY_WCM_SECURITY_WPA2_AES_PSK:
        case CY_WCM_SECURITY_WPA2_TKIP_PSK:
        case CY_WCM_SECURITY_WPA2_MIXED_PSK:
        case CY_WCM_SECURITY_WPA2_FBT_PSK:
            return WIFI_SECURITY_WPA2;
        case CY_WCM_SECURITY_WPA3_SAE:
            return WIFI_SECURITY_WPA3;
        case CY_WCM_SECURITY_WPA3_WPA2_PSK:
            return WIFI_SECURITY_WPA2_WPA3;
        case CY_WCM_SECURITY_WPA2_WPA_AES_PSK:
        case CY_WCM_SECURITY_WPA2_WPA_MIXED_PSK:
            return WIFI_SECURITY_WPA_WPA2;
        case CY_WCM_SECURITY_WPA_TKIP_ENT:
        case CY_WCM_SECURITY_WPA_AES_ENT:
        case CY_WCM_SECURITY_WPA_MIXED_ENT:
        case CY_WCM_SECURITY_WPA2_TKIP_ENT:
        case CY_WCM_SECURITY_WPA2_AES_ENT:
        case CY_WCM_SECURITY_WPA2_MIXED_ENT:
        case CY_WCM_SECURITY_WPA2_FBT_ENT:
            return WIFI_SECURITY_ENTERPRISE;
        default:
            return WIFI_SECURITY_UNKNOWN;
    }
}

/*******************************************************************************
 * WiFi Band Mapping
 ******************************************************************************/
static uint8_t map_channel_to_band(uint16_t channel)
{
    if (channel >= 1 && channel <= 14) return WIFI_BAND_2_4GHZ;
    if (channel >= 36 && channel <= 177) return WIFI_BAND_5GHZ;
    return WIFI_BAND_UNKNOWN;
}

/*******************************************************************************
 * Scan Callback - Called for each AP found
 * Runs in WCM internal thread context
 ******************************************************************************/
static void wifi_scan_callback(cy_wcm_scan_result_t *result_ptr,
                                void *user_data,
                                cy_wcm_scan_status_t status)
{
    (void)user_data;

    if ((result_ptr != NULL) && (status == CY_WCM_SCAN_INCOMPLETE))
    {
        /* Check SSID is valid and we have room */
        uint8_t ssid_len = strlen((const char *)result_ptr->SSID);
        if ((ssid_len > 0) && (scan_result_count < WIFI_SCAN_MAX_RESULTS))
        {
            ipc_wifi_network_t *net = &scan_results[scan_result_count];
            memset(net, 0, sizeof(ipc_wifi_network_t));

            /* Copy network info */
            strncpy(net->ssid, (const char *)result_ptr->SSID,
                    WIFI_SSID_MAX_LEN - 1);
            net->ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
            net->rssi = (int8_t)result_ptr->signal_strength;
            net->security = map_wcm_security(result_ptr->security);
            net->channel = (uint8_t)result_ptr->channel;
            net->band = map_channel_to_band(result_ptr->channel);
            net->flags = 0;

            scan_result_count++;
        }
    }

    if (status == CY_WCM_SCAN_COMPLETE)
    {
        /* Notify wifi_task that scan is done */
        if (wifi_task_handle != NULL)
        {
            xTaskNotifyGive(wifi_task_handle);
        }
    }
}

/*******************************************************************************
 * Send Scan Results via IPC (one per message)
 ******************************************************************************/
static void send_scan_results_via_ipc(void)
{
    /* Mark connected network in scan results */
    if (cy_wcm_is_connected_to_ap() && connected_ssid[0] != '\0')
    {
        for (uint32_t i = 0; i < scan_result_count; i++)
        {
            if (strcmp(scan_results[i].ssid, connected_ssid) == 0)
            {
                scan_results[i].flags |= 0x01;  /* Mark as connected */
                break;
            }
        }
    }

    /* Send each network as a separate IPC message */
    for (uint32_t i = 0; i < scan_result_count; i++)
    {
        ipc_msg_t msg;
        IPC_MSG_INIT(&msg, IPC_CMD_WIFI_SCAN_RESULT);
        msg.value = i;  /* Network index */

        /* Copy network info into IPC data payload */
        memcpy(msg.data, &scan_results[i], sizeof(ipc_wifi_network_t));

        cm33_ipc_send_retry(&msg, 0);

        /* Delay between sends to avoid single-buffer overwrite on CM55 */
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* Send scan complete with total count */
    ipc_msg_t done_msg;
    IPC_MSG_INIT(&done_msg, IPC_CMD_WIFI_SCAN_COMPLETE);
    done_msg.value = scan_result_count;
    cm33_ipc_send_retry(&done_msg, 0);

    printf("[CM33-WiFi] Scan complete: %u networks found\r\n",
           (unsigned int)scan_result_count);
}

/*******************************************************************************
 * Handle WiFi Scan Command
 ******************************************************************************/
static void handle_wifi_scan(void)
{
    if (!wifi_initialized)
    {
        /* Send error back */
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, WIFI_ERR_DRIVER);
        return;
    }

    wifi_state = WIFI_STATE_SCANNING;
    scan_result_count = 0;
    memset(scan_results, 0, sizeof(scan_results));

    printf("[CM33-WiFi] Starting WiFi scan...\r\n");

    cy_rslt_t result = cy_wcm_start_scan(wifi_scan_callback, NULL, NULL);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("[CM33-WiFi] Scan start failed: 0x%08X\r\n",
               (unsigned int)result);
        wifi_state = WIFI_STATE_DISCONNECTED;
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, WIFI_ERR_SCAN_FAILED);
        return;
    }

    /* Wait for scan completion (max 10 seconds) */
    uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));
    if (notified == 0)
    {
        printf("[CM33-WiFi] Scan timeout\r\n");
        cy_wcm_stop_scan();
    }

    /* Send results back to CM55 via IPC */
    send_scan_results_via_ipc();

    /* Restore state */
    if (cy_wcm_is_connected_to_ap())
    {
        wifi_state = WIFI_STATE_CONNECTED;
    }
    else
    {
        wifi_state = WIFI_STATE_DISCONNECTED;
    }
}

/*******************************************************************************
 * Handle WiFi Connect Command
 ******************************************************************************/
static void handle_wifi_connect(const ipc_msg_t *cmd_msg)
{
    if (!wifi_initialized)
    {
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, WIFI_ERR_DRIVER);
        return;
    }

    /* Extract connect params from IPC data */
    ipc_wifi_connect_t *conn = (ipc_wifi_connect_t *)cmd_msg->data;

    printf("[CM33-WiFi] Connecting to '%s'...\r\n", conn->ssid);
    wifi_state = WIFI_STATE_CONNECTING;

    /* Prepare WCM connect parameters */
    cy_wcm_connect_params_t connect_params;
    memset(&connect_params, 0, sizeof(connect_params));

    strncpy((char *)connect_params.ap_credentials.SSID,
            conn->ssid, CY_WCM_MAX_SSID_LEN - 1);
    strncpy((char *)connect_params.ap_credentials.password,
            conn->password, CY_WCM_MAX_PASSPHRASE_LEN - 1);

    /* Map our security enum back to WCM security */
    switch (conn->security)
    {
        case WIFI_SECURITY_OPEN:
            connect_params.ap_credentials.security = CY_WCM_SECURITY_OPEN;
            break;
        case WIFI_SECURITY_WPA2:
            connect_params.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;
            break;
        case WIFI_SECURITY_WPA3:
            connect_params.ap_credentials.security = CY_WCM_SECURITY_WPA3_SAE;
            break;
        case WIFI_SECURITY_WPA2_WPA3:
            connect_params.ap_credentials.security = CY_WCM_SECURITY_WPA3_WPA2_PSK;
            break;
        default:
            connect_params.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;
            break;
    }

    /* Attempt connection */
    cy_wcm_ip_address_t ip_addr;
    cy_rslt_t result = cy_wcm_connect_ap(&connect_params, &ip_addr);

    if (CY_RSLT_SUCCESS == result)
    {
        printf("[CM33-WiFi] Connected! IP: %u.%u.%u.%u\r\n",
               (uint8_t)(ip_addr.ip.v4),
               (uint8_t)(ip_addr.ip.v4 >> 8),
               (uint8_t)(ip_addr.ip.v4 >> 16),
               (uint8_t)(ip_addr.ip.v4 >> 24));

        wifi_state = WIFI_STATE_CONNECTED;
        strncpy(connected_ssid, conn->ssid, WIFI_SSID_MAX_LEN - 1);
        connected_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';

        /* Send connected status with IP */
        ipc_msg_t resp;
        IPC_MSG_INIT(&resp, IPC_CMD_WIFI_CONNECTED);

        ipc_wifi_status_t status;
        memset(&status, 0, sizeof(status));
        status.state = WIFI_STATE_CONNECTED;
        strncpy(status.ssid, conn->ssid, WIFI_SSID_MAX_LEN - 1);
        status.ip_addr[0] = (uint8_t)(ip_addr.ip.v4);
        status.ip_addr[1] = (uint8_t)(ip_addr.ip.v4 >> 8);
        status.ip_addr[2] = (uint8_t)(ip_addr.ip.v4 >> 16);
        status.ip_addr[3] = (uint8_t)(ip_addr.ip.v4 >> 24);

        memcpy(resp.data, &status, sizeof(ipc_wifi_status_t));
        cm33_ipc_send_retry(&resp, 0);

        /* NTP sync is now requested by CM55 via IPC_CMD_NTP_SYNC
         * after it finishes collecting TCP/IP and hardware info.
         * This avoids blocking wifi_task and IPC response collisions. */
    }
    else
    {
        printf("[CM33-WiFi] Connection failed: 0x%08X\r\n",
               (unsigned int)result);
        wifi_state = WIFI_STATE_DISCONNECTED;

        /* Send error with appropriate code */
        uint32_t err_code = WIFI_ERR_UNKNOWN;
        if (result == CY_RSLT_WCM_SECURITY_NOT_FOUND ||
            result == CY_RSLT_WCM_WAIT_TIMEOUT)
        {
            err_code = WIFI_ERR_AUTH_FAILED;
        }
        else if (result == CY_RSLT_WCM_AP_NOT_UP)
        {
            err_code = WIFI_ERR_NO_AP;
        }
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, err_code);
    }
}

/*******************************************************************************
 * Handle WiFi Disconnect Command
 ******************************************************************************/
static void handle_wifi_disconnect(void)
{
    if (!wifi_initialized)
    {
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, WIFI_ERR_DRIVER);
        return;
    }

    printf("[CM33-WiFi] Disconnecting...\r\n");
    wifi_state = WIFI_STATE_DISCONNECTING;

    cy_rslt_t result = cy_wcm_disconnect_ap();
    if (CY_RSLT_SUCCESS == result)
    {
        wifi_state = WIFI_STATE_DISCONNECTED;
        connected_ssid[0] = '\0';
        cm33_ipc_send_cmd(IPC_CMD_WIFI_DISCONNECTED, 0);
        printf("[CM33-WiFi] Disconnected\r\n");
    }
    else
    {
        wifi_state = WIFI_STATE_ERROR;
        cm33_ipc_send_cmd(IPC_CMD_WIFI_ERROR, WIFI_ERR_DRIVER);
    }
}

/*******************************************************************************
 * Handle WiFi Status Query
 ******************************************************************************/
static void handle_wifi_get_status(void)
{
    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_WIFI_STATUS);

    ipc_wifi_status_t status;
    memset(&status, 0, sizeof(status));
    status.state = wifi_state;

    if (wifi_initialized && cy_wcm_is_connected_to_ap())
    {
        cy_wcm_ip_address_t ip_addr;
        if (CY_RSLT_SUCCESS == cy_wcm_get_ip_addr(CY_WCM_INTERFACE_TYPE_STA,
                                                    &ip_addr))
        {
            status.ip_addr[0] = (uint8_t)(ip_addr.ip.v4);
            status.ip_addr[1] = (uint8_t)(ip_addr.ip.v4 >> 8);
            status.ip_addr[2] = (uint8_t)(ip_addr.ip.v4 >> 16);
            status.ip_addr[3] = (uint8_t)(ip_addr.ip.v4 >> 24);
        }
    }

    memcpy(resp.data, &status, sizeof(ipc_wifi_status_t));
    cm33_ipc_send_retry(&resp, 0);
}

/*******************************************************************************
 * Handle WiFi TCP/IP Info Query
 ******************************************************************************/
static void handle_wifi_get_tcpip(void)
{
    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_WIFI_TCPIP_INFO);

    ipc_wifi_tcpip_t tcpip;
    memset(&tcpip, 0, sizeof(tcpip));
    tcpip.dhcp_enabled = 1;  /* Default: DHCP */

    if (wifi_initialized && cy_wcm_is_connected_to_ap())
    {
        cy_wcm_ip_address_t ip_addr;
        if (CY_RSLT_SUCCESS == cy_wcm_get_ip_addr(CY_WCM_INTERFACE_TYPE_STA,
                                                    &ip_addr))
        {
            tcpip.ip_addr[0] = (uint8_t)(ip_addr.ip.v4);
            tcpip.ip_addr[1] = (uint8_t)(ip_addr.ip.v4 >> 8);
            tcpip.ip_addr[2] = (uint8_t)(ip_addr.ip.v4 >> 16);
            tcpip.ip_addr[3] = (uint8_t)(ip_addr.ip.v4 >> 24);
        }

        cy_wcm_ip_address_t gw_addr;
        if (CY_RSLT_SUCCESS == cy_wcm_get_gateway_ip_address(
                                    CY_WCM_INTERFACE_TYPE_STA, &gw_addr))
        {
            tcpip.gateway[0] = (uint8_t)(gw_addr.ip.v4);
            tcpip.gateway[1] = (uint8_t)(gw_addr.ip.v4 >> 8);
            tcpip.gateway[2] = (uint8_t)(gw_addr.ip.v4 >> 16);
            tcpip.gateway[3] = (uint8_t)(gw_addr.ip.v4 >> 24);
        }

        cy_wcm_ip_address_t netmask;
        if (CY_RSLT_SUCCESS == cy_wcm_get_ip_netmask(
                                    CY_WCM_INTERFACE_TYPE_STA, &netmask))
        {
            tcpip.subnet[0] = (uint8_t)(netmask.ip.v4);
            tcpip.subnet[1] = (uint8_t)(netmask.ip.v4 >> 8);
            tcpip.subnet[2] = (uint8_t)(netmask.ip.v4 >> 16);
            tcpip.subnet[3] = (uint8_t)(netmask.ip.v4 >> 24);
        }
    }

    memcpy(resp.data, &tcpip, sizeof(ipc_wifi_tcpip_t));
    cm33_ipc_send_retry(&resp, 0);
}

/*******************************************************************************
 * Handle WiFi Hardware Info Query
 ******************************************************************************/
static void handle_wifi_get_hardware(void)
{
    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_WIFI_HARDWARE_INFO);

    ipc_wifi_hardware_t hw;
    memset(&hw, 0, sizeof(hw));

    if (wifi_initialized)
    {
        cy_wcm_mac_t mac;
        if (CY_RSLT_SUCCESS == cy_wcm_get_mac_addr(CY_WCM_INTERFACE_TYPE_STA,
                                                     &mac))
        {
            memcpy(hw.mac_addr, mac, WIFI_MAC_ADDR_LEN);
        }

        strncpy(hw.fw_version, "CYW55513", sizeof(hw.fw_version) - 1);
    }

    memcpy(resp.data, &hw, sizeof(ipc_wifi_hardware_t));
    cm33_ipc_send_retry(&resp, 0);
}

/*******************************************************************************
 * NTP Time Sync (uses lwIP UDP socket)
 *
 * Sends NTP request to time.google.com, parses response, and sends
 * Unix epoch back to CM55 via IPC.
 ******************************************************************************/

#define NTP_PORT            (123U)
#define NTP_PACKET_SIZE     (48U)
#define NTP_EPOCH_OFFSET    (2208988800UL)  /* 1900-01-01 to 1970-01-01 */
#define NTP_TIMEOUT_SEC     (5)
#define NTP_SERVER_IP       "216.239.35.0"  /* time.google.com */

/* Track whether we've synced since last WiFi connect */
static bool ntp_synced = false;
static uint32_t ntp_last_sync_tick = 0;

/* Re-sync interval: 30 minutes */
#define NTP_RESYNC_INTERVAL_MS  (30 * 60 * 1000)

static void handle_ntp_sync(void)
{
    if (!wifi_initialized || !cy_wcm_is_connected_to_ap())
    {
        cm33_ipc_send_cmd(IPC_CMD_NTP_ERROR, 1);  /* No WiFi */
        return;
    }

    printf("[CM33-NTP] Syncing time from %s...\r\n", NTP_SERVER_IP);

    /* Create UDP socket */
    int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        printf("[CM33-NTP] Socket creation failed\r\n");
        cm33_ipc_send_cmd(IPC_CMD_NTP_ERROR, 2);
        return;
    }

    /* Set receive timeout */
    struct timeval tv;
    tv.tv_sec = NTP_TIMEOUT_SEC;
    tv.tv_usec = 0;
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* NTP server address */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = lwip_htons(NTP_PORT);
    server.sin_addr.s_addr = inet_addr(NTP_SERVER_IP);

    /* Prepare NTP request packet */
    uint8_t ntp_packet[NTP_PACKET_SIZE];
    memset(ntp_packet, 0, NTP_PACKET_SIZE);
    ntp_packet[0] = 0x1B;  /* LI=0, VN=3 (NTPv3), Mode=3 (client) */

    /* Send request */
    int sent = lwip_sendto(sock, ntp_packet, NTP_PACKET_SIZE, 0,
                           (struct sockaddr *)&server, sizeof(server));
    if (sent < 0)
    {
        printf("[CM33-NTP] Send failed\r\n");
        lwip_close(sock);
        cm33_ipc_send_cmd(IPC_CMD_NTP_ERROR, 3);
        return;
    }

    /* Receive response */
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int n = lwip_recvfrom(sock, ntp_packet, NTP_PACKET_SIZE, 0,
                          (struct sockaddr *)&from, &fromlen);
    lwip_close(sock);

    if (n < (int)NTP_PACKET_SIZE)
    {
        printf("[CM33-NTP] Receive failed or timeout (got %d bytes)\r\n", n);
        cm33_ipc_send_cmd(IPC_CMD_NTP_ERROR, 4);
        return;
    }

    /* Extract transmit timestamp from bytes 40-43 (big-endian) */
    uint32_t ntp_time = ((uint32_t)ntp_packet[40] << 24) |
                        ((uint32_t)ntp_packet[41] << 16) |
                        ((uint32_t)ntp_packet[42] << 8)  |
                        ((uint32_t)ntp_packet[43]);

    /* Convert NTP epoch (1900) to Unix epoch (1970) */
    uint32_t unix_epoch = ntp_time - NTP_EPOCH_OFFSET;

    /* Validate: should be after Jan 1, 2020 (1577836800) */
    if (unix_epoch < 1577836800UL)
    {
        printf("[CM33-NTP] Invalid timestamp: %u\r\n", (unsigned int)unix_epoch);
        cm33_ipc_send_cmd(IPC_CMD_NTP_ERROR, 5);
        return;
    }

    printf("[CM33-NTP] Time synced: epoch=%u\r\n", (unsigned int)unix_epoch);

    /* Send epoch to CM55 */
    ipc_msg_t resp;
    IPC_MSG_INIT(&resp, IPC_CMD_NTP_TIME);
    resp.value = unix_epoch;
    cm33_ipc_send_retry(&resp, 0);

    ntp_synced = true;
    ntp_last_sync_tick = (uint32_t)xTaskGetTickCount();
}

/*******************************************************************************
 * Process WiFi IPC Command
 ******************************************************************************/
static void process_wifi_command(const ipc_msg_t *msg)
{
    switch (msg->cmd)
    {
        case IPC_CMD_WIFI_SCAN_START:
            handle_wifi_scan();
            break;

        case IPC_CMD_WIFI_CONNECT:
            handle_wifi_connect(msg);
            break;

        case IPC_CMD_WIFI_DISCONNECT:
            handle_wifi_disconnect();
            break;

        case IPC_CMD_WIFI_STATUS:
            handle_wifi_get_status();
            break;

        case IPC_CMD_WIFI_GET_TCPIP:
            handle_wifi_get_tcpip();
            break;

        case IPC_CMD_WIFI_GET_HARDWARE:
            handle_wifi_get_hardware();
            break;

        case IPC_CMD_NTP_SYNC:
            handle_ntp_sync();
            break;

        default:
            printf("[CM33-WiFi] Unknown WiFi cmd: 0x%02X\r\n",
                   (unsigned int)msg->cmd);
            break;
    }
}

/*******************************************************************************
 * WiFi Task Entry Point
 ******************************************************************************/
void wifi_task(void *pvParameters)
{
    (void)pvParameters;

    /* Save task handle for scan notification */
    wifi_task_handle = xTaskGetCurrentTaskHandle();

    /* Create command queue */
    wifi_cmd_queue = xQueueCreate(WIFI_CMD_QUEUE_LENGTH, sizeof(ipc_msg_t));
    if (wifi_cmd_queue == NULL)
    {
        printf("[CM33-WiFi] FATAL: Failed to create command queue\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[CM33-WiFi] WiFi task started\r\n");

    /* Phase 1: Initialize SDIO interface */
    if (!app_sdio_init())
    {
        printf("[CM33-WiFi] SDIO init failed - WiFi unavailable\r\n");
        /* Stay alive but don't process commands */
        for (;;) { vTaskDelay(pdMS_TO_TICKS(10000)); }
    }

    /* Phase 2: Initialize WiFi Connection Manager */
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wcm_config.wifi_interface_instance = &sdio_instance;

    cy_rslt_t result = cy_wcm_init(&wcm_config);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("[CM33-WiFi] WCM init failed: 0x%08X\r\n",
               (unsigned int)result);
        for (;;) { vTaskDelay(pdMS_TO_TICKS(10000)); }
    }

    wifi_initialized = true;
    printf("[CM33-WiFi] WiFi Connection Manager initialized\r\n");

    /* Phase 3: Main loop - process WiFi IPC commands */
    ipc_msg_t cmd_msg;
    for (;;)
    {
        /* Wait for command from IPC handler (1 second timeout) */
        if (xQueueReceive(wifi_cmd_queue, &cmd_msg, pdMS_TO_TICKS(1000))
            == pdTRUE)
        {
            process_wifi_command(&cmd_msg);
        }
    }
}

/*******************************************************************************
 * Public API: Queue a WiFi Command
 ******************************************************************************/
bool wifi_task_queue_cmd(const ipc_msg_t *msg)
{
    if (wifi_cmd_queue == NULL || msg == NULL)
    {
        return false;
    }

    if (xQueueSend(wifi_cmd_queue, msg, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        return true;
    }

    printf("[CM33-WiFi] Command queue full\r\n");
    return false;
}

/* [] END OF FILE */
