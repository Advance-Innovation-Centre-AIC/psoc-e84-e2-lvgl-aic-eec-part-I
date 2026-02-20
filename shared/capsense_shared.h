/*******************************************************************************
 * File Name:   capsense_shared.h
 *
 * Description: Shared memory structure for CAPSENSE data between CM33 and CM55
 *              Used for Inter-Processor Communication (IPC)
 *
 *              CM33 writes CAPSENSE data (from I2C) to shared memory
 *              CM55 reads from shared memory and displays on LVGL
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *              Embedded Systems Engineering, Burapha University
 *
 * Target:      PSoC Edge E84 Evaluation Kit
 *
 * Memory Map:
 *   - m33_m55_shared region: 0x261C0000, size 256KB
 *   - CAPSENSE data starts at offset 0 (first 64 bytes)
 *
 * Usage:
 *   CM33 (writer):
 *     #include "capsense_shared.h"
 *     capsense_shared_t *caps = CAPSENSE_SHARED_PTR;
 *     caps->btn0_pressed = true;
 *     caps->valid = true;
 *
 *   CM55 (reader):
 *     #include "capsense_shared.h"
 *     capsense_shared_t *caps = CAPSENSE_SHARED_PTR;
 *     if (caps->valid) { ... }
 *
 ******************************************************************************/

#ifndef CAPSENSE_SHARED_H
#define CAPSENSE_SHARED_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Shared Memory Configuration
 ******************************************************************************/

/* Shared memory base address (from linker script) */
#define SHARED_MEM_BASE_ADDR        (0x261C0000UL)

/* CAPSENSE data offset in shared memory */
#define CAPSENSE_SHARED_OFFSET      (0x00000000UL)

/* Magic number to verify valid data (0xCA95E00D = "CAPSEND" in hex-ish) */
#define CAPSENSE_SHARED_MAGIC       (0xCA95E00DUL)

/*******************************************************************************
 * CAPSENSE Shared Data Structure
 *
 * Layout (64 bytes):
 *   Offset 0:  magic (4 bytes) - Must be CAPSENSE_SHARED_MAGIC
 *   Offset 4:  version (4 bytes)
 *   Offset 8:  valid (4 bytes) - Set true when data is valid
 *   Offset 12: update_count (4 bytes) - Incremented on each update
 *   Offset 16: btn0_pressed (1 byte)
 *   Offset 17: btn1_pressed (1 byte)
 *   Offset 18: slider_pos (1 byte, 0-100)
 *   Offset 19: slider_active (1 byte)
 *   Offset 20-63: reserved
 *
 ******************************************************************************/

typedef struct __attribute__((packed, aligned(4))) {
    /* Header - verification and versioning */
    uint32_t magic;             /* Must be CAPSENSE_SHARED_MAGIC */
    uint32_t version;           /* Structure version (currently 1) */
    uint32_t valid;             /* 1 = data valid, 0 = not yet written */
    uint32_t update_count;      /* Incremented each time CM33 updates */

    /* CAPSENSE data (from PSoC 4000T via I2C) */
    uint8_t btn0_pressed;       /* Button 0 (CSB1): 0=released, 1=pressed */
    uint8_t btn1_pressed;       /* Button 1 (CSB2): 0=released, 1=pressed */
    uint8_t slider_pos;         /* Slider position: 0-100 (0 = no touch) */
    uint8_t slider_active;      /* 1 = slider touched, 0 = not touched */

    /* Timestamps for debugging */
    uint32_t last_read_time_ms; /* Time of last I2C read (from CM33) */
    uint32_t error_count;       /* I2C error counter */

    /* Reserved for future use */
    uint8_t reserved[36];       /* Padding to 64 bytes total */

} capsense_shared_t;

/*******************************************************************************
 * Pointer to shared CAPSENSE data
 ******************************************************************************/

#define CAPSENSE_SHARED_PTR  ((volatile capsense_shared_t *)(SHARED_MEM_BASE_ADDR + CAPSENSE_SHARED_OFFSET))

/*******************************************************************************
 * Helper Macros
 ******************************************************************************/

/* Check if CAPSENSE data is valid */
#define CAPSENSE_SHARED_IS_VALID() \
    (CAPSENSE_SHARED_PTR->magic == CAPSENSE_SHARED_MAGIC && CAPSENSE_SHARED_PTR->valid != 0)

/* Get update count (to detect new data) */
#define CAPSENSE_SHARED_GET_COUNT() \
    (CAPSENSE_SHARED_PTR->update_count)

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize shared CAPSENSE structure (called by CM33 at startup)
 */
static inline void capsense_shared_init(void)
{
    volatile capsense_shared_t *caps = CAPSENSE_SHARED_PTR;

    caps->magic = CAPSENSE_SHARED_MAGIC;
    caps->version = 1;
    caps->valid = 0;
    caps->update_count = 0;
    caps->btn0_pressed = 0;
    caps->btn1_pressed = 0;
    caps->slider_pos = 0;
    caps->slider_active = 0;
    caps->last_read_time_ms = 0;
    caps->error_count = 0;
}

/**
 * @brief Update CAPSENSE data in shared memory (called by CM33 after I2C read)
 * @param btn0 Button 0 state
 * @param btn1 Button 1 state
 * @param slider Slider position (0-100)
 * @param active Slider active state
 * @param time_ms Current time in milliseconds
 */
static inline void capsense_shared_update(bool btn0, bool btn1,
                                          uint8_t slider, bool active,
                                          uint32_t time_ms)
{
    volatile capsense_shared_t *caps = CAPSENSE_SHARED_PTR;

    caps->btn0_pressed = btn0 ? 1 : 0;
    caps->btn1_pressed = btn1 ? 1 : 0;
    caps->slider_pos = slider;
    caps->slider_active = active ? 1 : 0;
    caps->last_read_time_ms = time_ms;
    caps->update_count++;
    caps->valid = 1;
}

/**
 * @brief Increment error count (called by CM33 on I2C error)
 */
static inline void capsense_shared_error(void)
{
    CAPSENSE_SHARED_PTR->error_count++;
}

/**
 * @brief Read CAPSENSE data from shared memory (called by CM55)
 * @param btn0 Pointer to store button 0 state
 * @param btn1 Pointer to store button 1 state
 * @param slider Pointer to store slider position
 * @param active Pointer to store slider active state
 * @return true if data is valid, false otherwise
 */
static inline bool capsense_shared_read(bool *btn0, bool *btn1,
                                        uint8_t *slider, bool *active)
{
    volatile capsense_shared_t *caps = CAPSENSE_SHARED_PTR;

    /* Check if data is valid */
    if (caps->magic != CAPSENSE_SHARED_MAGIC || caps->valid == 0) {
        return false;
    }

    /* Read data */
    if (btn0) *btn0 = (caps->btn0_pressed != 0);
    if (btn1) *btn1 = (caps->btn1_pressed != 0);
    if (slider) *slider = caps->slider_pos;
    if (active) *active = (caps->slider_active != 0);

    return true;
}

#endif /* CAPSENSE_SHARED_H */
