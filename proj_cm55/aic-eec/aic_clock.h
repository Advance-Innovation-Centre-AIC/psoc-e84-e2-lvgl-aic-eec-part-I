/*******************************************************************************
 * File: aic_clock.h
 * Description: NTP Clock Display for LVGL
 *
 * Displays current date/time in the top-right corner of the screen.
 * Time is synced via NTP from CM33-NS and maintained locally using
 * FreeRTOS tick counter for auto-update every 60 seconds.
 *
 * Format: "Thu 12 Feb 23:40" (UTC+7 Thailand)
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#ifndef AIC_CLOCK_H
#define AIC_CLOCK_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define AIC_CLOCK_UTC_OFFSET_SEC    (7 * 3600)  /* UTC+7 Thailand */
#define AIC_CLOCK_UPDATE_MS         (60000U)    /* Update display every 60s */

/*******************************************************************************
 * Clock Context Structure
 ******************************************************************************/

typedef struct {
    lv_obj_t*   lbl_time;       /* LVGL label showing time */
    lv_timer_t* update_timer;   /* 60-second update timer */
    uint32_t    base_epoch;     /* Unix epoch from NTP sync */
    uint32_t    base_tick;      /* FreeRTOS tick at sync moment */
    bool        time_synced;    /* true after first NTP sync */
} aic_clock_ctx_t;

/*******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Initialize clock display on given parent
 *
 * Creates a label at the top-right corner showing "--:--" until NTP syncs.
 * Starts a 60-second LVGL timer for auto-update.
 *
 * @param ctx    Pointer to clock context (caller allocates)
 * @param parent LVGL parent object (NULL = active screen)
 */
void aic_clock_init(aic_clock_ctx_t* ctx, lv_obj_t* parent);

/**
 * @brief Set time from NTP epoch
 *
 * Called when IPC_CMD_NTP_TIME is received from CM33.
 * Stores base_epoch and base_tick for local time calculation.
 * Updates the display immediately.
 *
 * @param ctx        Pointer to clock context
 * @param unix_epoch Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
 */
void aic_clock_set_time(aic_clock_ctx_t* ctx, uint32_t unix_epoch);

/**
 * @brief Force-update the clock display
 *
 * Recalculates current time from base_epoch + elapsed ticks.
 * Called automatically by the 60-second timer.
 *
 * @param ctx  Pointer to clock context
 */
void aic_clock_update(aic_clock_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* AIC_CLOCK_H */
