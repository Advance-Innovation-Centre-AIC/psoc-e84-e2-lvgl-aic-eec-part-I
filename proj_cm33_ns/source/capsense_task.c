/*******************************************************************************
 * File: capsense_task.c
 * Description: CAPSENSE I2C reader for CM33-NS
 *
 * Reads PSoC 4000T CAPSENSE chip via I2C and sends state changes
 * to CM55 via IPC_CMD_CAPSENSE_DATA. Uses edge detection to only
 * send data when button/slider state actually changes.
 *
 * I2C Protocol:
 *   Slave address: 0x08
 *   Read 3 bytes:
 *     [0] Button 0 (CSB1) - ASCII '0' or '1' (subtract 0x30)
 *     [1] Button 1 (CSB2) - ASCII '0'/'1'/'2' (subtract 0x30)
 *     [2] Slider (CSS1) - raw value 0-100
 *
 * Reference: psoc-edge-lvgl-capsense-main/proj_cm55/capsense_i2c.c
 *
 * Part of BiiL Course: Embedded C for IoT - Week 6 Ex9
 ******************************************************************************/

#include "capsense_task.h"
#include "../../shared/ipc_shared.h"
#include "../ipc/cm33_ipc_pipe.h"
#include <string.h>

/*******************************************************************************
 * Private State
 ******************************************************************************/
static CySCB_Type *cs_hw = NULL;
static cy_stc_scb_i2c_context_t *cs_ctx = NULL;
static bool cs_initialized = false;

/* Previous state for edge detection */
static uint8_t prev_btn0 = 0;
static uint8_t prev_btn1 = 0;
static uint8_t prev_slider = 0;
static uint8_t prev_slider_active = 0;

/* Current state */
static uint8_t cur_btn0 = 0;
static uint8_t cur_btn1 = 0;
static uint8_t cur_slider = 0;
static uint8_t cur_slider_active = 0;

/*******************************************************************************
 * Private: Read 3 bytes from CAPSENSE via I2C
 ******************************************************************************/
static bool capsense_i2c_read(uint8_t *btn0, uint8_t *btn1,
                               uint8_t *slider_pos, uint8_t *slider_active)
{
    if (!cs_initialized || !cs_hw || !cs_ctx) return false;

    uint8_t buffer[CAPSENSE_I2C_READ_SIZE] = {0};
    uint8_t *pData = &buffer[0];
    uint8_t remaining = CAPSENSE_I2C_READ_SIZE;
    cy_en_scb_i2c_command_t ack = CY_SCB_I2C_ACK;
    cy_en_scb_i2c_status_t status;

    /* Start I2C read transaction */
    status = (cs_ctx->state == CY_SCB_I2C_IDLE) ?
        Cy_SCB_I2C_MasterSendStart(cs_hw, CAPSENSE_I2C_SLAVE_ADDR,
                                    CY_SCB_I2C_READ_XFER,
                                    CAPSENSE_I2C_TIMEOUT_MS, cs_ctx) :
        Cy_SCB_I2C_MasterSendReStart(cs_hw, CAPSENSE_I2C_SLAVE_ADDR,
                                      CY_SCB_I2C_READ_XFER,
                                      CAPSENSE_I2C_TIMEOUT_MS, cs_ctx);

    if (CY_SCB_I2C_SUCCESS == status) {
        while (remaining > 0) {
            if (remaining == 1) {
                ack = CY_SCB_I2C_NAK;  /* NAK on last byte */
            }
            status = Cy_SCB_I2C_MasterReadByte(cs_hw, ack, pData,
                                                CAPSENSE_I2C_TIMEOUT_MS,
                                                cs_ctx);
            if (status != CY_SCB_I2C_SUCCESS) break;
            ++pData;
            --remaining;
        }
    }

    /* Always send STOP */
    Cy_SCB_I2C_MasterSendStop(cs_hw, CAPSENSE_I2C_TIMEOUT_MS, cs_ctx);

    if (status != CY_SCB_I2C_SUCCESS) return false;

    /* Parse: subtract ASCII offset for buttons */
    buffer[0] -= CAPSENSE_ASCII_OFFSET;
    buffer[1] -= CAPSENSE_ASCII_OFFSET;

    /* Button 0: 0=not pressed, non-zero=pressed */
    *btn0 = (buffer[0] != 0) ? 1 : 0;

    /* Button 1: 1=not pressed, 2=pressed (inverted logic) */
    *btn1 = (buffer[1] != 1) ? 1 : 0;

    /* Slider: 0=no touch, 1-100=position */
    *slider_pos = buffer[2];
    *slider_active = (buffer[2] != 0) ? 1 : 0;

    return true;
}

/*******************************************************************************
 * Private: Send current state via IPC
 ******************************************************************************/
static void capsense_send_ipc(void)
{
    ipc_msg_t msg;
    IPC_MSG_INIT(&msg, IPC_CMD_CAPSENSE_DATA);
    msg.data[0] = cur_btn0;
    msg.data[1] = cur_btn1;
    msg.data[2] = cur_slider;
    msg.data[3] = cur_slider_active;
    cm33_ipc_send_retry(&msg, 0);
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void capsense_module_init(CySCB_Type *hw, cy_stc_scb_i2c_context_t *context)
{
    cs_hw = hw;
    cs_ctx = context;
    cs_initialized = (hw != NULL && context != NULL);

    prev_btn0 = 0;
    prev_btn1 = 0;
    prev_slider = 0;
    prev_slider_active = 0;
}

void capsense_module_poll(void)
{
    uint8_t b0, b1, sp, sa;

    if (!capsense_i2c_read(&b0, &b1, &sp, &sa)) return;

    cur_btn0 = b0;
    cur_btn1 = b1;
    cur_slider = sp;
    cur_slider_active = sa;

    /* Edge detection: only send IPC when state changes */
    if (b0 != prev_btn0 || b1 != prev_btn1 ||
        sp != prev_slider || sa != prev_slider_active) {

        capsense_send_ipc();

        prev_btn0 = b0;
        prev_btn1 = b1;
        prev_slider = sp;
        prev_slider_active = sa;
    }
}

void capsense_module_send_current(void)
{
    capsense_send_ipc();
}
