/*******************************************************************************
 * File: cm33_ipc_pipe.h
 * Description: CM33-NS IPC Pipe interface for inter-processor communication
 *
 * This module provides IPC communication from CM33-NS to CM55 core.
 * Uses Infineon IPC Pipe library.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef CM33_IPC_PIPE_H
#define CM33_IPC_PIPE_H

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
typedef void (*cm33_ipc_rx_callback_t)(const ipc_msg_t *msg, void *user_data);

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize IPC pipe for CM33-NS
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_init(void);

/**
 * @brief Deinitialize IPC pipe
 */
void cm33_ipc_deinit(void);

/**
 * @brief Check if IPC is initialized
 * @return true if initialized
 */
bool cm33_ipc_is_init(void);

/*******************************************************************************
 * Send Functions
 ******************************************************************************/

/**
 * @brief Send message to CM55
 * @param msg Pointer to message structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send(const ipc_msg_t *msg);

/**
 * @brief Send message with retry on busy
 * @param msg Pointer to message structure
 * @param max_retries Maximum retry attempts (0 = use default)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_retry(const ipc_msg_t *msg, uint32_t max_retries);

/**
 * @brief Send command with value
 * @param cmd Command to send
 * @param value Numeric value
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_cmd(ipc_cmd_t cmd, uint32_t value);

/**
 * @brief Send command with string data
 * @param cmd Command to send
 * @param data String data (null-terminated)
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_data(ipc_cmd_t cmd, const char *data);

/*******************************************************************************
 * Receive Functions
 ******************************************************************************/

/**
 * @brief Check if message received
 * @return true if message pending
 */
bool cm33_ipc_msg_pending(void);

/**
 * @brief Get received message (copies to provided buffer)
 * @param msg Pointer to store received message
 * @return true if message retrieved
 */
bool cm33_ipc_get_msg(ipc_msg_t *msg);

/**
 * @brief Register callback for received messages
 * @param callback Function to call on message receive
 * @param user_data User data passed to callback
 */
void cm33_ipc_register_callback(cm33_ipc_rx_callback_t callback, void *user_data);

/**
 * @brief Process pending IPC messages (call from main loop)
 */
void cm33_ipc_process(void);

/*******************************************************************************
 * Sensor Functions
 ******************************************************************************/

/**
 * @brief Send IMU data to CM55
 * @param data Pointer to IMU data structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_imu(const ipc_imu_data_t *data);

/**
 * @brief Send ADC data to CM55
 * @param data Pointer to ADC data structure
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_adc(const ipc_adc_data_t *data);

/*******************************************************************************
 * GPIO Functions
 ******************************************************************************/

/**
 * @brief Send button event to CM55
 * @param button_id Button identifier
 * @param pressed true if pressed, false if released
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_button_event(uint8_t button_id, bool pressed);

/**
 * @brief Send LED state to CM55 (for confirmation)
 * @param led_id LED identifier
 * @param state LED state
 * @return CY_IPC_PIPE_SUCCESS on success
 */
cy_en_ipc_pipe_status_t cm33_ipc_send_led_state(uint8_t led_id, bool state);

/*******************************************************************************
 * Logging Functions
 ******************************************************************************/

/**
 * @brief Handle log message from CM55 (print to console)
 * @param msg Log message
 */
void cm33_ipc_handle_log(const ipc_msg_t *msg);

/*******************************************************************************
 * Statistics
 ******************************************************************************/

/**
 * @brief Get IPC statistics
 * @param tx_count Pointer to store TX count (can be NULL)
 * @param rx_count Pointer to store RX count (can be NULL)
 * @param error_count Pointer to store error count (can be NULL)
 */
void cm33_ipc_get_stats(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count);

/**
 * @brief Reset IPC statistics
 */
void cm33_ipc_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* CM33_IPC_PIPE_H */
