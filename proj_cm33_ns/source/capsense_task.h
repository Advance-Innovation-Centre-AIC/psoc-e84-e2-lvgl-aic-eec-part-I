/*******************************************************************************
 * File: capsense_task.h
 * Description: CAPSENSE I2C reader for CM33-NS
 *
 * Reads PSoC 4000T CAPSENSE chip (I2C address 0x08) and sends
 * state changes to CM55 via IPC. Called from imu_task to share
 * the I2C bus without contention.
 *
 * Part of BiiL Course: Embedded C for IoT - Week 6 Ex9
 ******************************************************************************/

#ifndef CAPSENSE_TASK_H
#define CAPSENSE_TASK_H

#include "cy_scb_i2c.h"
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * CAPSENSE I2C Configuration
 ******************************************************************************/
#define CAPSENSE_I2C_SLAVE_ADDR     (0x08U)
#define CAPSENSE_I2C_READ_SIZE      (3U)
#define CAPSENSE_I2C_TIMEOUT_MS     (0U)    /* Blocking */
#define CAPSENSE_ASCII_OFFSET       (0x30U)

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * Initialize CAPSENSE module.
 * @param hw      I2C SCB hardware pointer (e.g. CYBSP_I2C_CONTROLLER_HW)
 * @param context I2C context pointer (shared with IMU)
 */
void capsense_module_init(CySCB_Type *hw, cy_stc_scb_i2c_context_t *context);

/**
 * Poll CAPSENSE and send IPC on state change.
 * Call this periodically (e.g. every 100ms from imu_task).
 */
void capsense_module_poll(void);

/**
 * Send current CAPSENSE state via IPC (for CAPSENSE_REQ).
 */
void capsense_module_send_current(void);

#endif /* CAPSENSE_TASK_H */
