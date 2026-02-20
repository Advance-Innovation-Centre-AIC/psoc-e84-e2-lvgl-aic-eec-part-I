/*******************************************************************************
 * File: aic_log.h
 * Description: AIC-EEC Logging System
 *
 * Thread-safe, non-blocking logging system with queue-based output.
 * Supports log levels and optional IPC forwarding to CM33 console.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef AIC_LOG_H
#define AIC_LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Log Levels
 ******************************************************************************/

typedef enum {
    AIC_LOG_NONE = 0,   /**< No logging */
    AIC_LOG_ERROR,      /**< Error messages only */
    AIC_LOG_WARN,       /**< Warnings and errors */
    AIC_LOG_INFO,       /**< Info, warnings, and errors */
    AIC_LOG_DEBUG,      /**< Debug and all above */
    AIC_LOG_VERBOSE     /**< All messages */
} aic_log_level_t;

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define AIC_LOG_QUEUE_SIZE      (16U)       /**< Number of messages in queue */
#define AIC_LOG_MSG_MAX_LEN     (128U)      /**< Max message length */
#define AIC_LOG_TASK_STACK      (256U)      /**< Log task stack size (words) */
#define AIC_LOG_TASK_PRIORITY   (2U)        /**< Log task priority */

/*******************************************************************************
 * Log Output Targets
 ******************************************************************************/

typedef enum {
    AIC_LOG_TARGET_NONE     = 0x00,     /**< No output */
    AIC_LOG_TARGET_PRINTF   = 0x01,     /**< Output via printf (UART) */
    AIC_LOG_TARGET_IPC      = 0x02,     /**< Output via IPC to CM33 */
    AIC_LOG_TARGET_LVGL     = 0x04,     /**< Output to LVGL label (if set) */
    AIC_LOG_TARGET_ALL      = 0x07      /**< All targets */
} aic_log_target_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize logging system
 * @return true on success
 */
bool aic_log_init(void);

/**
 * @brief Deinitialize logging system
 */
void aic_log_deinit(void);

/**
 * @brief Check if logging is initialized
 * @return true if initialized
 */
bool aic_log_is_init(void);

/**
 * @brief Set current log level
 * @param level Minimum level to output (messages below this are filtered)
 */
void aic_log_set_level(aic_log_level_t level);

/**
 * @brief Get current log level
 * @return Current log level
 */
aic_log_level_t aic_log_get_level(void);

/**
 * @brief Set log output targets
 * @param targets Bitmask of output targets
 */
void aic_log_set_targets(uint8_t targets);

/**
 * @brief Get current log output targets
 * @return Bitmask of output targets
 */
uint8_t aic_log_get_targets(void);

/**
 * @brief Log message (queued, non-blocking)
 * @param level Log level
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 *
 * This function is safe to call from any context including ISR.
 * Messages are queued and output asynchronously.
 */
void aic_log(aic_log_level_t level, const char *fmt, ...);

/**
 * @brief Log message with tag (queued, non-blocking)
 * @param level Log level
 * @param tag Module/component tag
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void aic_log_tag(aic_log_level_t level, const char *tag, const char *fmt, ...);

/**
 * @brief Flush log queue (blocking)
 *
 * Wait until all queued messages are output.
 * Useful before system shutdown or debug breakpoints.
 */
void aic_log_flush(void);

/**
 * @brief Get number of messages in queue
 * @return Number of pending messages
 */
uint32_t aic_log_queue_count(void);

/**
 * @brief Get number of dropped messages (queue overflow)
 * @return Number of messages dropped since init
 */
uint32_t aic_log_dropped_count(void);

/*******************************************************************************
 * Convenience Macros
 ******************************************************************************/

/* Basic logging macros */
#define AIC_LOGE(fmt, ...)  aic_log(AIC_LOG_ERROR, fmt, ##__VA_ARGS__)
#define AIC_LOGW(fmt, ...)  aic_log(AIC_LOG_WARN, fmt, ##__VA_ARGS__)
#define AIC_LOGI(fmt, ...)  aic_log(AIC_LOG_INFO, fmt, ##__VA_ARGS__)
#define AIC_LOGD(fmt, ...)  aic_log(AIC_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define AIC_LOGV(fmt, ...)  aic_log(AIC_LOG_VERBOSE, fmt, ##__VA_ARGS__)

/* Tagged logging macros */
#define AIC_LOGE_TAG(tag, fmt, ...)  aic_log_tag(AIC_LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define AIC_LOGW_TAG(tag, fmt, ...)  aic_log_tag(AIC_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define AIC_LOGI_TAG(tag, fmt, ...)  aic_log_tag(AIC_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define AIC_LOGD_TAG(tag, fmt, ...)  aic_log_tag(AIC_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define AIC_LOGV_TAG(tag, fmt, ...)  aic_log_tag(AIC_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)

/* Define module tag for current file (use in .c files) */
#define AIC_LOG_TAG(tag)    static const char *LOG_TAG = tag

/* Use module tag */
#define LOGE(fmt, ...)      AIC_LOGE_TAG(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)      AIC_LOGW_TAG(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)      AIC_LOGI_TAG(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)      AIC_LOGD_TAG(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGV(fmt, ...)      AIC_LOGV_TAG(LOG_TAG, fmt, ##__VA_ARGS__)

/*******************************************************************************
 * LVGL Integration (Optional)
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"

/**
 * @brief Set LVGL label for log output
 * @param label LVGL label object (NULL to disable)
 * @param max_lines Maximum lines to keep (0 = unlimited)
 */
void aic_log_set_lvgl_label(lv_obj_t *label, uint8_t max_lines);

#endif /* LV_LVGL_H_INCLUDE_SIMPLE */

/*******************************************************************************
 * FreeRTOS Task (Internal)
 ******************************************************************************/

/**
 * @brief Create log output task
 *
 * Called automatically by aic_log_init() if FreeRTOS is available.
 */
void aic_log_create_task(void);

/**
 * @brief Delete log output task
 */
void aic_log_delete_task(void);

/**
 * @brief Process log queue (for non-FreeRTOS or manual processing)
 *
 * Call this periodically if not using FreeRTOS task.
 */
void aic_log_process(void);

#ifdef __cplusplus
}
#endif

#endif /* AIC_LOG_H */
