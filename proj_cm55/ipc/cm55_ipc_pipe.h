/*******************************************************************************
 * File: cm55_ipc_pipe.h
 * Description: CM55 IPC Pipe interface for inter-processor communication
 *
 * This module provides IPC communication from CM55 to CM33-NS core.
 * Uses Infineon IPC Pipe library with FreeRTOS integration.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef CM55_IPC_PIPE_H
#define CM55_IPC_PIPE_H

#include "cy_ipc_pipe.h"
#include "../../shared/ipc_shared.h"
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

/**
 * @brief IPC receive callback function type
 * @param msg Pointer to received message
 * @param user_data User-defined data passed during registration
 */
typedef void (*cm55_ipc_rx_callback_t)(const ipc_msg_t *msg, void *user_data);

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize IPC pipe for CM55
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_init(void);

/**
 * @brief Deinitialize IPC pipe
 */
void cm55_ipc_deinit(void);

/**
 * @brief Check if IPC is initialized
 * @return true if initialized
 */
bool cm55_ipc_is_init(void);

/*******************************************************************************
 * Send Functions
 ******************************************************************************/

/**
 * @brief Send message to CM33-NS
 * @param msg Pointer to message structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send(const ipc_msg_t *msg);

/**
 * @brief Send message with retry on busy
 * @param msg Pointer to message structure
 * @param max_retries Maximum retry attempts (0 = use default)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send_retry(const ipc_msg_t *msg, uint32_t max_retries);

/**
 * @brief Send command with value
 * @param cmd Command to send
 * @param value Numeric value
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send_cmd(ipc_cmd_t cmd, uint32_t value);

/**
 * @brief Send command with string data
 * @param cmd Command to send
 * @param data String data (null-terminated)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send_data(ipc_cmd_t cmd, const char *data);

/*******************************************************************************
 * Receive Functions
 ******************************************************************************/

/**
 * @brief Check if message received
 * @return true if message pending
 */
bool cm55_ipc_msg_pending(void);

/**
 * @brief Get received message (copies to provided buffer)
 * @param msg Pointer to store received message
 * @return true if message retrieved
 */
bool cm55_ipc_get_msg(ipc_msg_t *msg);

/**
 * @brief Register callback for received messages
 * @param callback Function to call on message receive
 * @param user_data User data passed to callback
 */
void cm55_ipc_register_callback(cm55_ipc_rx_callback_t callback, void *user_data);

/**
 * @brief Process pending IPC messages (call from main loop or task)
 *
 * This function should be called periodically to process received messages.
 * It invokes registered callbacks for each pending message.
 */
void cm55_ipc_process(void);

/*******************************************************************************
 * Logging Functions (via IPC to CM33 console)
 ******************************************************************************/

/**
 * @brief Send log message to CM33 console
 * @param fmt Printf-style format string
 */
void cm55_ipc_log(const char *fmt, ...);

/**
 * @brief Send log message with level
 * @param level Log level (IPC_CMD_LOG_ERROR, IPC_CMD_LOG_WARN, etc.)
 * @param fmt Printf-style format string
 */
void cm55_ipc_log_level(ipc_cmd_t level, const char *fmt, ...);

/*******************************************************************************
 * Convenience Macros
 ******************************************************************************/

#define CM55_LOG(fmt, ...)    cm55_ipc_log(fmt, ##__VA_ARGS__)
#define CM55_LOGE(fmt, ...)   cm55_ipc_log_level(IPC_CMD_LOG_ERROR, fmt, ##__VA_ARGS__)
#define CM55_LOGW(fmt, ...)   cm55_ipc_log_level(IPC_CMD_LOG_WARN, fmt, ##__VA_ARGS__)
#define CM55_LOGI(fmt, ...)   cm55_ipc_log_level(IPC_CMD_LOG_INFO, fmt, ##__VA_ARGS__)
#define CM55_LOGD(fmt, ...)   cm55_ipc_log_level(IPC_CMD_LOG_DEBUG, fmt, ##__VA_ARGS__)

/*******************************************************************************
 * Sensor Data Functions
 ******************************************************************************/

/**
 * @brief Send IMU data to CM33
 * @param data Pointer to IMU data structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send_imu(const ipc_imu_data_t *data);

/**
 * @brief Send ADC data to CM33
 * @param data Pointer to ADC data structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_send_adc(const ipc_adc_data_t *data);

/**
 * @brief Request sensor data from CM33
 * @param sensor_type Sensor type (IPC_CMD_IMU_DATA, IPC_CMD_ADC_DATA, etc.)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_request_sensor(ipc_cmd_t sensor_type);

/*******************************************************************************
 * GPIO Functions (via IPC)
 ******************************************************************************/

/**
 * @brief Send LED control command
 * @param led_id LED identifier
 * @param state LED state (0 = off, 1 = on)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_set_led(uint8_t led_id, bool state);

/**
 * @brief Send LED brightness command
 * @param led_id LED identifier
 * @param brightness Brightness (0-100 percent)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_set_led_brightness(uint8_t led_id, uint8_t brightness);

/**
 * @brief Request button state from CM33
 * @param button_id Button identifier
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm55_ipc_request_button(uint8_t button_id);

/*******************************************************************************
 * FreeRTOS Task
 ******************************************************************************/

/**
 * @brief Create IPC receive task (FreeRTOS)
 *
 * Creates a FreeRTOS task that continuously processes IPC messages.
 * The task runs at priority 3 and uses configMINIMAL_STACK_SIZE * 2.
 */
void cm55_ipc_create_task(void);

/**
 * @brief Delete IPC task
 */
void cm55_ipc_delete_task(void);

/*******************************************************************************
 * Statistics and Debugging
 ******************************************************************************/

/**
 * @brief Get IPC statistics
 * @param tx_count Pointer to store TX count (can be NULL)
 * @param rx_count Pointer to store RX count (can be NULL)
 * @param error_count Pointer to store error count (can be NULL)
 */
void cm55_ipc_get_stats(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count);

/**
 * @brief Reset IPC statistics
 */
void cm55_ipc_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* CM55_IPC_PIPE_H */
