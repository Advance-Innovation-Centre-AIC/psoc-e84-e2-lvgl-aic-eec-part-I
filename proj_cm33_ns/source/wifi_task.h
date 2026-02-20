/*******************************************************************************
 * File: wifi_task.h
 * Description: WiFi task for CM33-NS - Header
 *
 * Manages WiFi SDIO interface, WiFi Connection Manager (cy_wcm),
 * and processes WiFi IPC commands from CM55.
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../../shared/ipc_shared.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define WIFI_TASK_STACK_SIZE            (4096U)
#define WIFI_TASK_PRIORITY              (3U)
#define WIFI_CMD_QUEUE_LENGTH           (8U)

/* Scan configuration */
#define WIFI_SCAN_MAX_RESULTS           (16U)

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief WiFi FreeRTOS task entry point
 *
 * Initializes SDIO interface, WiFi Connection Manager (cy_wcm),
 * and processes WiFi IPC commands from CM55.
 *
 * @param pvParameters  Unused task parameter
 */
void wifi_task(void *pvParameters);

/**
 * @brief Queue an IPC command for the WiFi task to process
 *
 * Called from cm33_ipc_pipe.c to route WiFi commands (0xD0-0xDF)
 * to the WiFi task for processing.
 *
 * @param msg  Pointer to the IPC message
 * @return true if command was queued, false if queue is full
 */
bool wifi_task_queue_cmd(const ipc_msg_t *msg);

#endif /* WIFI_TASK_H */
