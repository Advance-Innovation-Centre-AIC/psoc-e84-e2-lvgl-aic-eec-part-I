/*******************************************************************************
 * File: wifi_shared.h
 * Description: Shared WiFi definitions for CM33-NS and CM55
 *
 * This file defines WiFi-specific structures and constants used for
 * Inter-Processor Communication between CM33-NS (WiFi driver) and CM55 (UI).
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#ifndef WIFI_SHARED_H
#define WIFI_SHARED_H

#include <stdint.h>
#include <stdbool.h>
#include "ipc_shared.h"

/*******************************************************************************
 * WiFi Configuration Constants
 ******************************************************************************/

#define WIFI_SSID_MAX_LEN       (33U)   /* 32 chars + null terminator */
#define WIFI_PASSWORD_MAX_LEN   (65U)   /* 64 chars + null terminator */
#define WIFI_SCAN_MAX_NETWORKS  (16U)   /* Maximum networks in scan result */
#define WIFI_MAC_ADDR_LEN       (6U)    /* MAC address length */
#define WIFI_IP_ADDR_LEN        (4U)    /* IPv4 address length */

/*******************************************************************************
 * WiFi Security Types
 ******************************************************************************/

typedef enum {
    WIFI_SECURITY_OPEN          = 0,
    WIFI_SECURITY_WEP           = 1,
    WIFI_SECURITY_WPA           = 2,
    WIFI_SECURITY_WPA2          = 3,
    WIFI_SECURITY_WPA3          = 4,
    WIFI_SECURITY_WPA_WPA2      = 5,
    WIFI_SECURITY_WPA2_WPA3     = 6,
    WIFI_SECURITY_ENTERPRISE    = 7,
    WIFI_SECURITY_UNKNOWN       = 0xFF
} wifi_security_t;

/*******************************************************************************
 * WiFi Band Types
 ******************************************************************************/

typedef enum {
    WIFI_BAND_2_4GHZ    = 0,
    WIFI_BAND_5GHZ      = 1,
    WIFI_BAND_6GHZ      = 2,
    WIFI_BAND_UNKNOWN   = 0xFF
} wifi_band_t;

/*******************************************************************************
 * WiFi Connection State
 ******************************************************************************/

typedef enum {
    WIFI_STATE_DISCONNECTED     = 0,
    WIFI_STATE_CONNECTING       = 1,
    WIFI_STATE_CONNECTED        = 2,
    WIFI_STATE_DISCONNECTING    = 3,
    WIFI_STATE_SCANNING         = 4,
    WIFI_STATE_ERROR            = 5
} wifi_state_t;

/* WiFi IPC Commands (0xD0-0xDF) are defined in ipc_shared.h */

/*******************************************************************************
 * WiFi Network Info Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    char        ssid[WIFI_SSID_MAX_LEN];    /* Network SSID */
    int8_t      rssi;                        /* Signal strength in dBm */
    uint8_t     security;                    /* Security type (wifi_security_t) */
    uint8_t     channel;                     /* WiFi channel (1-14 for 2.4GHz) */
    uint8_t     band;                        /* Frequency band (wifi_band_t) */
    uint8_t     flags;                       /* Bit flags: bit0=connected, bit1=saved */
    uint8_t     reserved[2];                 /* Padding for alignment */
} ipc_wifi_network_t;

/*******************************************************************************
 * WiFi Scan Result Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t             count;              /* Number of networks found */
    int8_t              connected_idx;      /* Index of connected network (-1 if none) */
    uint8_t             reserved[2];        /* Padding */
    ipc_wifi_network_t  networks[WIFI_SCAN_MAX_NETWORKS];
} ipc_wifi_scan_t;

/*******************************************************************************
 * WiFi Connect Request Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    char        ssid[WIFI_SSID_MAX_LEN];
    char        password[WIFI_PASSWORD_MAX_LEN];
    uint8_t     security;                   /* wifi_security_t */
    uint8_t     reserved[3];                /* Padding */
} ipc_wifi_connect_t;

/*******************************************************************************
 * WiFi TCP/IP Info Structure (for Network Details - TCP/IP tab)
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     dhcp_enabled;               /* 1 = DHCP, 0 = Static */
    uint8_t     reserved[3];                /* Padding */
    uint8_t     ip_addr[WIFI_IP_ADDR_LEN];  /* IPv4 address */
    uint8_t     subnet[WIFI_IP_ADDR_LEN];   /* Subnet mask */
    uint8_t     gateway[WIFI_IP_ADDR_LEN];  /* Default gateway (Router) */
    uint8_t     dns1[WIFI_IP_ADDR_LEN];     /* Primary DNS */
    uint8_t     dns2[WIFI_IP_ADDR_LEN];     /* Secondary DNS (optional) */
    uint32_t    lease_time;                 /* DHCP lease time in seconds */
} ipc_wifi_tcpip_t;

/*******************************************************************************
 * WiFi Hardware Info Structure (for Network Details - Hardware tab)
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     mac_addr[WIFI_MAC_ADDR_LEN];/* MAC address */
    uint8_t     band;                       /* Current band (wifi_band_t) */
    uint8_t     channel;                    /* Current channel */
    int8_t      rssi;                       /* Current RSSI */
    int8_t      tx_power;                   /* TX power in dBm */
    uint16_t    mtu;                        /* MTU size */
    uint32_t    link_speed;                 /* Link speed in Mbps */
    char        fw_version[16];             /* Firmware version string */
} ipc_wifi_hardware_t;

/*******************************************************************************
 * WiFi Status Structure
 ******************************************************************************/

typedef struct __attribute__((packed)) {
    uint8_t     state;                      /* wifi_state_t */
    int8_t      rssi;                       /* Current RSSI if connected */
    uint8_t     security;                   /* Security of connected network */
    uint8_t     reserved;                   /* Padding */
    char        ssid[WIFI_SSID_MAX_LEN];    /* Connected SSID */
    uint8_t     ip_addr[WIFI_IP_ADDR_LEN];  /* IP if connected */
    uint32_t    uptime;                     /* Connection uptime in seconds */
} ipc_wifi_status_t;

/*******************************************************************************
 * WiFi Error Codes
 ******************************************************************************/

typedef enum {
    WIFI_ERR_NONE               = 0,
    WIFI_ERR_TIMEOUT            = 1,
    WIFI_ERR_AUTH_FAILED        = 2,
    WIFI_ERR_NO_AP              = 3,
    WIFI_ERR_CONNECTION_LOST    = 4,
    WIFI_ERR_DRIVER             = 5,
    WIFI_ERR_SCAN_FAILED        = 6,
    WIFI_ERR_DHCP_FAILED        = 7,
    WIFI_ERR_UNKNOWN            = 0xFF
} wifi_error_t;

/*******************************************************************************
 * Helper Macros
 ******************************************************************************/

/* Convert RSSI to signal bars (0-4) for UI display */
#define WIFI_RSSI_TO_BARS(rssi) \
    ((rssi) >= -50 ? 4 : \
     (rssi) >= -60 ? 3 : \
     (rssi) >= -70 ? 2 : \
     (rssi) >= -80 ? 1 : 0)

/* Check if network is connected */
#define WIFI_IS_CONNECTED(network)  (((network)->flags & 0x01) != 0)

/* Check if network is saved */
#define WIFI_IS_SAVED(network)      (((network)->flags & 0x02) != 0)

/* Format IP address to string */
#define WIFI_IP_TO_STR(ip, buf) \
    snprintf(buf, 16, "%u.%u.%u.%u", (ip)[0], (ip)[1], (ip)[2], (ip)[3])

/* Format MAC address to string */
#define WIFI_MAC_TO_STR(mac, buf) \
    snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X", \
             (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5])

/* Security type to string */
static inline const char* wifi_security_to_str(uint8_t security)
{
    switch (security) {
        case WIFI_SECURITY_OPEN:        return "Open";
        case WIFI_SECURITY_WEP:         return "WEP";
        case WIFI_SECURITY_WPA:         return "WPA";
        case WIFI_SECURITY_WPA2:        return "WPA2";
        case WIFI_SECURITY_WPA3:        return "WPA3";
        case WIFI_SECURITY_WPA_WPA2:    return "WPA/WPA2";
        case WIFI_SECURITY_WPA2_WPA3:   return "WPA2/WPA3";
        case WIFI_SECURITY_ENTERPRISE:  return "Enterprise";
        default:                        return "Unknown";
    }
}

/* Band to string */
static inline const char* wifi_band_to_str(uint8_t band)
{
    switch (band) {
        case WIFI_BAND_2_4GHZ:  return "2.4 GHz";
        case WIFI_BAND_5GHZ:    return "5 GHz";
        case WIFI_BAND_6GHZ:    return "6 GHz";
        default:                return "Unknown";
    }
}

/* State to string */
static inline const char* wifi_state_to_str(uint8_t state)
{
    switch (state) {
        case WIFI_STATE_DISCONNECTED:   return "Disconnected";
        case WIFI_STATE_CONNECTING:     return "Connecting...";
        case WIFI_STATE_CONNECTED:      return "Connected";
        case WIFI_STATE_DISCONNECTING:  return "Disconnecting...";
        case WIFI_STATE_SCANNING:       return "Scanning...";
        case WIFI_STATE_ERROR:          return "Error";
        default:                        return "Unknown";
    }
}

#endif /* WIFI_SHARED_H */
