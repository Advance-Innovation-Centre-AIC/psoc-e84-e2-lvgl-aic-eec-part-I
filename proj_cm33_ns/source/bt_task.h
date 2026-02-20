/*******************************************************************************
 * File: bt_task.h
 * Description: Bluetooth task for CM33-NS - Header
 *
 * Manages WICED BT Stack initialization, BLE scanning,
 * and processes BT IPC commands from CM55.
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#ifndef BT_TASK_H
#define BT_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../../shared/ipc_shared.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define BT_TASK_STACK_SIZE              (4096U)
#define BT_TASK_PRIORITY                (3U)
#define BT_CMD_QUEUE_LENGTH             (8U)

/* Scan configuration */
#define BT_SCAN_MAX_RESULTS             (16U)
#define BT_SCAN_DURATION_SEC            (10U)

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Bluetooth FreeRTOS task entry point
 *
 * Initializes WICED BT Stack, registers management callback,
 * and processes BT IPC commands from CM55.
 *
 * @param pvParameters  Unused task parameter
 */
void bt_task(void *pvParameters);

/**
 * @brief Queue an IPC command for the BT task to process
 *
 * Called from cm33_ipc_pipe.c to route BT commands (0xE0-0xEF)
 * to the BT task for processing.
 *
 * @param msg  Pointer to the IPC message
 * @return true if command was queued, false if queue is full
 */
bool bt_task_queue_cmd(const ipc_msg_t *msg);

#endif /* BT_TASK_H */
