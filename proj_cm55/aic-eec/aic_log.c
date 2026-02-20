/*******************************************************************************
 * File: aic_log.c
 * Description: AIC-EEC Logging System Implementation
 *
 * Thread-safe, non-blocking logging system with queue-based output.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#include "aic_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

typedef struct {
    aic_log_level_t level;
    char message[AIC_LOG_MSG_MAX_LEN];
} log_entry_t;

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

static bool log_initialized = false;
static aic_log_level_t current_level = AIC_LOG_INFO;
static uint8_t output_targets = AIC_LOG_TARGET_PRINTF;

static QueueHandle_t log_queue = NULL;
static TaskHandle_t log_task_handle = NULL;
static SemaphoreHandle_t log_mutex = NULL;

static uint32_t dropped_count = 0;

/* Level prefixes */
static const char *level_prefixes[] = {
    "",         /* NONE */
    "[E] ",     /* ERROR */
    "[W] ",     /* WARN */
    "[I] ",     /* INFO */
    "[D] ",     /* DEBUG */
    "[V] "      /* VERBOSE */
};

/* Level colors (ANSI escape codes) */
static const char *level_colors[] = {
    "",             /* NONE */
    "\033[31m",     /* ERROR - Red */
    "\033[33m",     /* WARN - Yellow */
    "\033[32m",     /* INFO - Green */
    "\033[36m",     /* DEBUG - Cyan */
    "\033[37m"      /* VERBOSE - White */
};

static const char *color_reset = "\033[0m";

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
static lv_obj_t *lvgl_label = NULL;
static uint8_t lvgl_max_lines = 10;
static char lvgl_buffer[AIC_LOG_MSG_MAX_LEN * 10];
#endif

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static void output_message(const log_entry_t *entry)
{
    if (output_targets & AIC_LOG_TARGET_PRINTF) {
        printf("%s%s%s%s\r\n",
               level_colors[entry->level],
               level_prefixes[entry->level],
               entry->message,
               color_reset);
    }

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    if ((output_targets & AIC_LOG_TARGET_LVGL) && lvgl_label != NULL) {
        /* Append to LVGL buffer */
        size_t current_len = strlen(lvgl_buffer);
        size_t msg_len = strlen(entry->message);

        /* Check if we need to scroll (remove first line) */
        if (lvgl_max_lines > 0) {
            int line_count = 0;
            for (size_t i = 0; i < current_len; i++) {
                if (lvgl_buffer[i] == '\n') line_count++;
            }

            while (line_count >= lvgl_max_lines && current_len > 0) {
                /* Find first newline and remove everything before it */
                char *newline = strchr(lvgl_buffer, '\n');
                if (newline != NULL) {
                    size_t remove_len = (newline - lvgl_buffer) + 1;
                    memmove(lvgl_buffer, newline + 1, current_len - remove_len + 1);
                    current_len -= remove_len;
                    line_count--;
                } else {
                    break;
                }
            }
        }

        /* Append new message */
        if (current_len + msg_len + 2 < sizeof(lvgl_buffer)) {
            if (current_len > 0) {
                strcat(lvgl_buffer, "\n");
            }
            strcat(lvgl_buffer, level_prefixes[entry->level]);
            strcat(lvgl_buffer, entry->message);

            lv_label_set_text(lvgl_label, lvgl_buffer);
        }
    }
#endif
}

/*******************************************************************************
 * Log Task
 ******************************************************************************/

static void log_task(void *param)
{
    (void)param;
    log_entry_t entry;

    while (1) {
        if (xQueueReceive(log_queue, &entry, portMAX_DELAY) == pdTRUE) {
            output_message(&entry);
        }
    }
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

bool aic_log_init(void)
{
    if (log_initialized) {
        return true;
    }

    /* Create queue */
    log_queue = xQueueCreate(AIC_LOG_QUEUE_SIZE, sizeof(log_entry_t));
    if (log_queue == NULL) {
        return false;
    }

    /* Create mutex */
    log_mutex = xSemaphoreCreateMutex();
    if (log_mutex == NULL) {
        vQueueDelete(log_queue);
        log_queue = NULL;
        return false;
    }

    /* Create task */
    aic_log_create_task();

    log_initialized = true;
    dropped_count = 0;

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    lvgl_buffer[0] = '\0';
#endif

    return true;
}

void aic_log_deinit(void)
{
    if (!log_initialized) {
        return;
    }

    aic_log_delete_task();

    if (log_mutex != NULL) {
        vSemaphoreDelete(log_mutex);
        log_mutex = NULL;
    }

    if (log_queue != NULL) {
        vQueueDelete(log_queue);
        log_queue = NULL;
    }

    log_initialized = false;
}

bool aic_log_is_init(void)
{
    return log_initialized;
}

void aic_log_set_level(aic_log_level_t level)
{
    current_level = level;
}

aic_log_level_t aic_log_get_level(void)
{
    return current_level;
}

void aic_log_set_targets(uint8_t targets)
{
    output_targets = targets;
}

uint8_t aic_log_get_targets(void)
{
    return output_targets;
}

void aic_log(aic_log_level_t level, const char *fmt, ...)
{
    /* Filter by level */
    if (level > current_level || level == AIC_LOG_NONE) {
        return;
    }

    /* If not initialized, output directly */
    if (!log_initialized || log_queue == NULL) {
        va_list args;
        va_start(args, fmt);
        printf("%s", level_prefixes[level]);
        vprintf(fmt, args);
        printf("\r\n");
        va_end(args);
        return;
    }

    /* Create log entry */
    log_entry_t entry;
    entry.level = level;

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.message, AIC_LOG_MSG_MAX_LEN, fmt, args);
    va_end(args);

    /* Try to queue (don't block) */
    BaseType_t result;
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xQueueSendFromISR(log_queue, &entry, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xQueueSend(log_queue, &entry, 0);
    }

    if (result != pdTRUE) {
        dropped_count++;
    }
}

void aic_log_tag(aic_log_level_t level, const char *tag, const char *fmt, ...)
{
    /* Filter by level */
    if (level > current_level || level == AIC_LOG_NONE) {
        return;
    }

    /* If not initialized, output directly */
    if (!log_initialized || log_queue == NULL) {
        va_list args;
        va_start(args, fmt);
        printf("%s[%s] ", level_prefixes[level], tag);
        vprintf(fmt, args);
        printf("\r\n");
        va_end(args);
        return;
    }

    /* Create log entry with tag */
    log_entry_t entry;
    entry.level = level;

    /* Format: [TAG] message */
    int tag_len = snprintf(entry.message, AIC_LOG_MSG_MAX_LEN, "[%s] ", tag);

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.message + tag_len, AIC_LOG_MSG_MAX_LEN - tag_len, fmt, args);
    va_end(args);

    /* Try to queue (don't block) */
    BaseType_t result;
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xQueueSendFromISR(log_queue, &entry, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xQueueSend(log_queue, &entry, 0);
    }

    if (result != pdTRUE) {
        dropped_count++;
    }
}

void aic_log_flush(void)
{
    if (!log_initialized || log_queue == NULL) {
        return;
    }

    /* Wait until queue is empty */
    while (uxQueueMessagesWaiting(log_queue) > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    /* Small delay to ensure output is complete */
    vTaskDelay(pdMS_TO_TICKS(10));
}

uint32_t aic_log_queue_count(void)
{
    if (log_queue == NULL) {
        return 0;
    }
    return uxQueueMessagesWaiting(log_queue);
}

uint32_t aic_log_dropped_count(void)
{
    return dropped_count;
}

void aic_log_create_task(void)
{
    if (log_task_handle != NULL) {
        return;
    }

    xTaskCreate(
        log_task,
        "AIC_LOG",
        AIC_LOG_TASK_STACK,
        NULL,
        AIC_LOG_TASK_PRIORITY,
        &log_task_handle
    );
}

void aic_log_delete_task(void)
{
    if (log_task_handle != NULL) {
        vTaskDelete(log_task_handle);
        log_task_handle = NULL;
    }
}

void aic_log_process(void)
{
    if (!log_initialized || log_queue == NULL) {
        return;
    }

    log_entry_t entry;
    while (xQueueReceive(log_queue, &entry, 0) == pdTRUE) {
        output_message(&entry);
    }
}

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
void aic_log_set_lvgl_label(lv_obj_t *label, uint8_t max_lines)
{
    lvgl_label = label;
    lvgl_max_lines = max_lines;
    lvgl_buffer[0] = '\0';

    if (label != NULL) {
        output_targets |= AIC_LOG_TARGET_LVGL;
    } else {
        output_targets &= ~AIC_LOG_TARGET_LVGL;
    }
}
#endif
