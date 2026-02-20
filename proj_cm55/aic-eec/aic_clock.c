/*******************************************************************************
 * File: aic_clock.c
 * Description: NTP Clock Display for LVGL
 *
 * Displays current date/time in the top-right corner of the screen.
 * Time is synced via NTP from CM33-NS and maintained locally using
 * FreeRTOS tick counter between syncs.
 *
 * Part of BiiL Course: Embedded C for IoT - Week 7
 ******************************************************************************/

#include "aic_clock.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/*******************************************************************************
 * Day/Month Name Tables
 ******************************************************************************/

static const char* const day_names[7]   = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char* const month_names[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const int days_in_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*******************************************************************************
 * Epoch to Date/Time Conversion (with UTC+7 applied)
 ******************************************************************************/

static void epoch_to_datetime(uint32_t epoch,
                              int *year, int *month, int *day,
                              int *hour, int *min, int *wday)
{
    /* Apply Thailand timezone (UTC+7) */
    epoch += AIC_CLOCK_UTC_OFFSET_SEC;

    /* Time of day */
    uint32_t total_secs = epoch;
    *min  = (int)((total_secs / 60) % 60);
    *hour = (int)((total_secs / 3600) % 24);

    /* Total days since 1970-01-01 */
    uint32_t total_days = total_secs / 86400;

    /* Day of week: 1970-01-01 = Thursday (4) */
    *wday = (int)((total_days + 4) % 7);  /* 0=Sun, 4=Thu */

    /* Year */
    int y = 1970;
    for (;;) {
        bool leap = (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
        uint32_t ydays = leap ? 366U : 365U;
        if (total_days < ydays) break;
        total_days -= ydays;
        y++;
    }
    *year = y;

    /* Month */
    bool leap = (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
    int m = 0;
    while (m < 12) {
        int dim = days_in_month[m];
        if (m == 1 && leap) dim = 29;
        if (total_days < (uint32_t)dim) break;
        total_days -= (uint32_t)dim;
        m++;
    }
    *month = m;   /* 0-based */
    *day = (int)total_days + 1;
}

/*******************************************************************************
 * Format Time String
 ******************************************************************************/

static void format_time_string(char *buf, size_t buflen, uint32_t epoch)
{
    int year, month, day, hour, min, wday;
    epoch_to_datetime(epoch, &year, &month, &day, &hour, &min, &wday);

    (void)year;  /* Not displayed in compact format */
    snprintf(buf, buflen, "%s %d %s %02d:%02d",
             day_names[wday], day, month_names[month], hour, min);
}

/*******************************************************************************
 * Timer Callback (runs every 60 seconds in LVGL thread)
 ******************************************************************************/

static void clock_timer_cb(lv_timer_t *timer)
{
    aic_clock_ctx_t *ctx = (aic_clock_ctx_t *)lv_timer_get_user_data(timer);
    if (ctx) {
        aic_clock_update(ctx);
    }
}

/*******************************************************************************
 * API Implementation
 ******************************************************************************/

void aic_clock_init(aic_clock_ctx_t* ctx, lv_obj_t* parent)
{
    if (!ctx) return;

    /* Default state */
    ctx->base_epoch   = 0;
    ctx->base_tick    = 0;
    ctx->time_synced  = false;
    ctx->update_timer = NULL;

    /* Use active screen if no parent specified */
    if (!parent) {
        parent = lv_screen_active();
    }

    /* Create time label in top-right corner */
    ctx->lbl_time = lv_label_create(parent);
    lv_label_set_text(ctx->lbl_time, "--:--");
    lv_obj_set_style_text_font(ctx->lbl_time, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->lbl_time, lv_color_hex(0x8E8E93), 0);
    lv_obj_align(ctx->lbl_time, LV_ALIGN_TOP_RIGHT, -10, 6);

    /* Make sure it's on top */
    lv_obj_move_foreground(ctx->lbl_time);

    /* Don't intercept touches */
    lv_obj_remove_flag(ctx->lbl_time, LV_OBJ_FLAG_CLICKABLE);

    /* Start 60-second update timer */
    ctx->update_timer = lv_timer_create(clock_timer_cb,
                                         AIC_CLOCK_UPDATE_MS, ctx);

    printf("[Clock] Initialized (waiting for NTP sync)\r\n");
}

void aic_clock_set_time(aic_clock_ctx_t* ctx, uint32_t unix_epoch)
{
    if (!ctx) return;

    ctx->base_epoch  = unix_epoch;
    ctx->base_tick   = (uint32_t)xTaskGetTickCount();
    ctx->time_synced = true;

    /* Update display immediately */
    aic_clock_update(ctx);

    printf("[Clock] Time set: epoch=%u\r\n", (unsigned int)unix_epoch);
}

void aic_clock_update(aic_clock_ctx_t* ctx)
{
    if (!ctx || !ctx->lbl_time || !ctx->time_synced) return;

    /* Calculate current epoch from base + elapsed ticks */
    uint32_t current_tick = (uint32_t)xTaskGetTickCount();
    uint32_t elapsed_ms   = (current_tick - ctx->base_tick)
                            * portTICK_PERIOD_MS;
    uint32_t current_epoch = ctx->base_epoch + (elapsed_ms / 1000);

    /* Format and display */
    char buf[32];
    format_time_string(buf, sizeof(buf), current_epoch);
    lv_label_set_text(ctx->lbl_time, buf);
}
