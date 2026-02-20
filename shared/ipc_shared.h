/*******************************************************************************
 * File: ipc_shared.h
 * Description: Shared IPC definitions for CM33-NS and CM55
 *
 * This file defines the common structures and constants used for
 * Inter-Processor Communication between CM33-NS and CM55 cores.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef IPC_SHARED_H
#define IPC_SHARED_H

#include <stdint.h>

/*******************************************************************************
 * IPC Configuration
 *
 * Endpoint addresses, channels, interrupts, and masks are defined in
 * shared/include/ipc_communication.h (the Pipe infrastructure header).
 ******************************************************************************/

/* Data Limits */
#define IPC_DATA_MAX_LEN        (128U)

/* Retry Configuration */
#define IPC_SEND_MAX_RETRIES    (10U)
#define IPC_SEND_RETRY_DELAY_MS (1U)

/*******************************************************************************
 * IPC Commands
 ******************************************************************************/

typedef enum {
    IPC_CMD_NONE        = 0x00,

    /* System Commands (0x40-0x4F) */
    IPC_CMD_STATUS      = 0x41,
    IPC_CMD_PING        = 0x42,
    IPC_CMD_PONG        = 0x43,
    IPC_CMD_ACK         = 0x44,
    IPC_CMD_NACK        = 0x45,

    /* Control Commands (0x80-0x8F) */
    IPC_CMD_INIT        = 0x81,
    IPC_CMD_START       = 0x82,
    IPC_CMD_STOP        = 0x83,
    IPC_CMD_RESET       = 0x84,

    /* Logging Commands (0x90-0x9F) */
    IPC_CMD_LOG         = 0x90,
    IPC_CMD_LOG_LEVEL   = 0x91,
    IPC_CMD_LOG_ERROR   = 0x92,
    IPC_CMD_LOG_WARN    = 0x93,
    IPC_CMD_LOG_INFO    = 0x94,
    IPC_CMD_LOG_DEBUG   = 0x95,

    /* Sensor Commands (0xA0-0xAF) */
    IPC_CMD_SENSOR_REQ  = 0xA0,
    IPC_CMD_SENSOR_DATA = 0xA1,
    IPC_CMD_IMU_DATA    = 0xA2,
    IPC_CMD_ADC_DATA    = 0xA3,
    IPC_CMD_TEMP_DATA   = 0xA4,

    /* GPIO Commands (0xB0-0xBF) */
    IPC_CMD_GPIO_SET    = 0xB0,
    IPC_CMD_GPIO_GET    = 0xB1,
    IPC_CMD_LED_SET     = 0xB2,
    IPC_CMD_LED_BRIGHTNESS = 0xB3,
    IPC_CMD_BUTTON      = 0xB4,
    IPC_CMD_BUTTON_EVENT = 0xB5,
    IPC_CMD_CAPSENSE_DATA = 0xB6,   /* CM33→CM55: CAPSENSE button/slider state */
    IPC_CMD_CAPSENSE_REQ  = 0xB7,   /* CM55→CM33: Request current CAPSENSE state */

    /* Event Commands (0xC0-0xCF) */
    IPC_CMD_EVENT       = 0xC0,
    IPC_CMD_SUBSCRIBE   = 0xC1,
    IPC_CMD_UNSUBSCRIBE = 0xC2,

    /* WiFi Commands (0xD0-0xDF) - See wifi_shared.h for details */
    IPC_CMD_WIFI_SCAN_START     = 0xD0,
    IPC_CMD_WIFI_SCAN_RESULT    = 0xD1,
    IPC_CMD_WIFI_SCAN_COMPLETE  = 0xD2,
    IPC_CMD_WIFI_CONNECT        = 0xD3,
    IPC_CMD_WIFI_DISCONNECT     = 0xD4,
    IPC_CMD_WIFI_STATUS         = 0xD5,
    IPC_CMD_WIFI_GET_TCPIP      = 0xD6,
    IPC_CMD_WIFI_TCPIP_INFO     = 0xD7,
    IPC_CMD_WIFI_GET_HARDWARE   = 0xD8,
    IPC_CMD_WIFI_HARDWARE_INFO  = 0xD9,
    IPC_CMD_WIFI_CONNECTED      = 0xDA,
    IPC_CMD_WIFI_DISCONNECTED   = 0xDB,
    IPC_CMD_WIFI_ERROR          = 0xDC,

    /* Bluetooth Commands (0xE0-0xEF) - See bt_shared.h for details */
    IPC_CMD_BT_SCAN_START       = 0xE0,
    IPC_CMD_BT_SCAN_RESULT      = 0xE1,
    IPC_CMD_BT_SCAN_COMPLETE    = 0xE2,
    IPC_CMD_BT_CONNECT          = 0xE3,
    IPC_CMD_BT_DISCONNECT       = 0xE4,
    IPC_CMD_BT_STATUS           = 0xE5,
    IPC_CMD_BT_GET_HARDWARE     = 0xE6,
    IPC_CMD_BT_HARDWARE_INFO    = 0xE7,
    IPC_CMD_BT_CONNECTED        = 0xE8,
    IPC_CMD_BT_DISCONNECTED     = 0xE9,
    IPC_CMD_BT_ERROR            = 0xEA,

    /* NTP/Time Commands (0xF0-0xF3) */
    IPC_CMD_NTP_SYNC            = 0xF0,   /* CM55→CM33: Request NTP time sync */
    IPC_CMD_NTP_TIME            = 0xF1,   /* CM33→CM55: Time result (value=epoch) */
    IPC_CMD_NTP_ERROR           = 0xF2,   /* CM33→CM55: Sync failed */

} ipc_cmd_t;

/*******************************************************************************
 * IPC Message Structure
 ******************************************************************************/

typedef struct {
    uint16_t client_id;             /* Bits 0-15: Destination client ID */
    uint16_t intr_mask;             /* Bits 16-31: Release mask (MANDATORY for Pipe Driver) */
    uint32_t cmd;                   /* Command type (ipc_cmd_t) */
    uint32_t value;                 /* Numeric payload */
    char     data[IPC_DATA_MAX_LEN];/* String/binary payload */
} ipc_msg_t;

/*******************************************************************************
 * Sensor Data Structures (for IPC)
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint32_t timestamp;
} ipc_imu_data_t;

typedef struct __attribute__((packed)) {
    uint16_t adc_ch0;
    uint16_t adc_ch1;
    uint16_t adc_ch2;
    uint16_t adc_ch3;
    uint32_t timestamp;
} ipc_adc_data_t;

typedef struct __attribute__((packed)) {
    int16_t temperature;    /* Temperature in 0.01 degrees C */
    int16_t humidity;       /* Humidity in 0.01 % */
    uint32_t timestamp;
} ipc_env_data_t;

/*******************************************************************************
 * GPIO Data Structures (for IPC)
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t led_id;         /* LED identifier */
    uint8_t state;          /* 0 = off, 1 = on */
    uint8_t brightness;     /* 0-100 percent */
    uint8_t reserved;
} ipc_led_data_t;

typedef struct __attribute__((packed)) {
    uint8_t button_id;      /* Button identifier */
    uint8_t pressed;        /* 0 = released, 1 = pressed */
    uint8_t long_press;     /* 1 = long press detected */
    uint8_t reserved;
    uint32_t timestamp;
} ipc_button_data_t;

/*******************************************************************************
 * Helper Macros
 ******************************************************************************/

/* Check if IPC message is valid */
#define IPC_MSG_VALID(msg)  ((msg)->cmd != IPC_CMD_NONE)

/* Clear IPC message */
#define IPC_MSG_CLEAR(msg)  do { \
    (msg)->cmd = IPC_CMD_NONE; \
    (msg)->value = 0; \
    (msg)->data[0] = '\0'; \
} while(0)

/* Initialize IPC message with command */
#define IPC_MSG_INIT(msg, command) do { \
    (msg)->client_id = 0; \
    (msg)->intr_mask = 0; \
    (msg)->cmd = (command); \
    (msg)->value = 0; \
    (msg)->data[0] = '\0'; \
} while(0)

#endif /* IPC_SHARED_H */
