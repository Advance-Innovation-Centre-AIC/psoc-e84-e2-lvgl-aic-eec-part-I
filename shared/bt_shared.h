/*******************************************************************************
 * File: bt_shared.h
 * Description: Shared Bluetooth definitions for CM33-NS and CM55
 *
 * This file defines BLE-specific structures and constants used for
 * Inter-Processor Communication between CM33-NS (BT driver) and CM55 (UI).
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#ifndef BT_SHARED_H
#define BT_SHARED_H

#include <stdint.h>
#include <stdbool.h>
#include "ipc_shared.h"

/*******************************************************************************
 * Bluetooth Configuration Constants
 ******************************************************************************/

#define BT_DEVICE_NAME_MAX_LEN  (33U)   /* 32 chars + null terminator */
#define BT_ADDR_LEN             (6U)    /* Bluetooth device address length */
#define BT_SCAN_MAX_DEVICES     (16U)   /* Maximum devices in scan result */
#define BT_UUID_MAX_LEN         (16U)   /* 128-bit UUID */

/*******************************************************************************
 * Bluetooth Device Type
 ******************************************************************************/

typedef enum {
    BT_DEVICE_TYPE_UNKNOWN      = 0,
    BT_DEVICE_TYPE_LE           = 1,    /* BLE only */
    BT_DEVICE_TYPE_CLASSIC      = 2,    /* Classic BT */
    BT_DEVICE_TYPE_DUAL         = 3     /* Dual mode */
} bt_device_type_t;

/*******************************************************************************
 * Bluetooth Connection State
 ******************************************************************************/

typedef enum {
    BT_STATE_OFF                = 0,
    BT_STATE_INITIALIZING       = 1,
    BT_STATE_READY              = 2,    /* Stack initialized, idle */
    BT_STATE_SCANNING           = 3,
    BT_STATE_CONNECTING         = 4,
    BT_STATE_CONNECTED          = 5,
    BT_STATE_DISCONNECTING      = 6,
    BT_STATE_ERROR              = 7
} bt_state_t;

/*******************************************************************************
 * BLE Address Type
 ******************************************************************************/

typedef enum {
    BT_ADDR_PUBLIC              = 0,
    BT_ADDR_RANDOM              = 1,
    BT_ADDR_PUBLIC_ID           = 2,
    BT_ADDR_RANDOM_ID           = 3
} bt_addr_type_t;

/* BT IPC Commands (0xE0-0xEF) are defined in ipc_shared.h */

/*******************************************************************************
 * BLE Device Info Structure (scan result entry)
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     addr[BT_ADDR_LEN];          /* Device Bluetooth address */
    uint8_t     addr_type;                   /* bt_addr_type_t */
    int8_t      rssi;                        /* Signal strength in dBm */
    char        name[BT_DEVICE_NAME_MAX_LEN];/* Device name (may be empty) */
    uint8_t     device_type;                 /* bt_device_type_t */
    uint8_t     flags;                       /* Bit flags: bit0=connectable, bit1=paired */
    uint8_t     reserved;                    /* Padding */
} ipc_bt_device_t;

/*******************************************************************************
 * BLE Scan Result Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t         count;                  /* Number of devices found */
    int8_t          connected_idx;          /* Index of connected device (-1 if none) */
    uint8_t         reserved[2];            /* Padding */
} ipc_bt_scan_header_t;

/*******************************************************************************
 * BLE Connect Request Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     addr[BT_ADDR_LEN];          /* Target device address */
    uint8_t     addr_type;                   /* bt_addr_type_t */
    uint8_t     reserved;                    /* Padding */
} ipc_bt_connect_t;

/*******************************************************************************
 * BLE Hardware Info Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     addr[BT_ADDR_LEN];          /* Local BT address */
    uint8_t     state;                       /* bt_state_t */
    uint8_t     num_connections;             /* Active connections */
    char        fw_version[16];             /* BT firmware version */
    char        chip_name[16];              /* BT chip name */
} ipc_bt_hardware_t;

/*******************************************************************************
 * BLE Status Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     state;                       /* bt_state_t */
    uint8_t     num_connections;             /* Active connections */
    uint8_t     is_scanning;                 /* 1 if currently scanning */
    uint8_t     is_advertising;              /* 1 if currently advertising */
    uint8_t     connected_addr[BT_ADDR_LEN];/* Connected device address */
    int8_t      connected_rssi;              /* RSSI of connected device */
    uint8_t     reserved;                    /* Padding */
    char        connected_name[BT_DEVICE_NAME_MAX_LEN]; /* Connected device name */
} ipc_bt_status_t;

/*******************************************************************************
 * BLE Error Codes
 ******************************************************************************/

typedef enum {
    BT_ERR_NONE                 = 0,
    BT_ERR_TIMEOUT              = 1,
    BT_ERR_AUTH_FAILED          = 2,
    BT_ERR_DEVICE_NOT_FOUND     = 3,
    BT_ERR_CONNECTION_LOST      = 4,
    BT_ERR_STACK_INIT           = 5,
    BT_ERR_SCAN_FAILED          = 6,
    BT_ERR_NOT_READY            = 7,
    BT_ERR_UNKNOWN              = 0xFF
} bt_error_t;

/*******************************************************************************
 * Helper Macros
 ******************************************************************************/

/* Convert RSSI to signal bars (0-4) for UI display */
#define BT_RSSI_TO_BARS(rssi) \
    ((rssi) >= -50 ? 4 : \
     (rssi) >= -60 ? 3 : \
     (rssi) >= -70 ? 2 : \
     (rssi) >= -80 ? 1 : 0)

/* Check if device is connectable */
#define BT_IS_CONNECTABLE(device)   (((device)->flags & 0x01) != 0)

/* Check if device is paired */
#define BT_IS_PAIRED(device)        (((device)->flags & 0x02) != 0)

/* Format BT address to string */
#define BT_ADDR_TO_STR(addr, buf) \
    snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X", \
             (addr)[0], (addr)[1], (addr)[2], (addr)[3], (addr)[4], (addr)[5])

/* Device type to string */
static inline const char* bt_device_type_to_str(uint8_t type)
{
    switch (type) {
        case BT_DEVICE_TYPE_LE:      return "BLE";
        case BT_DEVICE_TYPE_CLASSIC: return "Classic";
        case BT_DEVICE_TYPE_DUAL:    return "Dual";
        default:                     return "Unknown";
    }
}

/* State to string */
static inline const char* bt_state_to_str(uint8_t state)
{
    switch (state) {
        case BT_STATE_OFF:           return "Off";
        case BT_STATE_INITIALIZING:  return "Initializing...";
        case BT_STATE_READY:         return "Ready";
        case BT_STATE_SCANNING:      return "Scanning...";
        case BT_STATE_CONNECTING:    return "Connecting...";
        case BT_STATE_CONNECTED:     return "Connected";
        case BT_STATE_DISCONNECTING: return "Disconnecting...";
        case BT_STATE_ERROR:         return "Error";
        default:                     return "Unknown";
    }
}

#endif /* BT_SHARED_H */
