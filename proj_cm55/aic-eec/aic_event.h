/*******************************************************************************
 * File: aic_event.h
 * Description: AIC-EEC Event Bus System
 *
 * Publish-Subscribe event system for decoupling sensor updates from UI.
 * Thread-safe event delivery with callback-based subscription.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef AIC_EVENT_H
#define AIC_EVENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Event Types
 ******************************************************************************/

typedef enum {
    AIC_EVENT_NONE = 0,

    /* Sensor Events (1-20) */
    AIC_EVENT_IMU_UPDATE,       /**< IMU data updated */
    AIC_EVENT_ADC_UPDATE,       /**< ADC value updated */
    AIC_EVENT_TEMP_UPDATE,      /**< Temperature updated */
    AIC_EVENT_HUMIDITY_UPDATE,  /**< Humidity updated */
    AIC_EVENT_PRESSURE_UPDATE,  /**< Pressure updated */

    /* Input Events (21-40) */
    AIC_EVENT_BUTTON_PRESS = 21,    /**< Button pressed */
    AIC_EVENT_BUTTON_RELEASE,       /**< Button released */
    AIC_EVENT_BUTTON_LONG_PRESS,    /**< Long press detected */
    AIC_EVENT_CAPSENSE_UPDATE,      /**< CapSense slider/button updated */
    AIC_EVENT_TOUCH_UPDATE,         /**< Touch screen event */

    /* System Events (41-60) */
    AIC_EVENT_IPC_CONNECTED = 41,   /**< IPC connection established */
    AIC_EVENT_IPC_DISCONNECTED,     /**< IPC connection lost */
    AIC_EVENT_IPC_MESSAGE,          /**< IPC message received */
    AIC_EVENT_ERROR,                /**< Error occurred */
    AIC_EVENT_WARNING,              /**< Warning */
    AIC_EVENT_TIMER,                /**< Timer expired */

    /* Application Events (61-80) */
    AIC_EVENT_MODE_CHANGE = 61,     /**< Application mode changed */
    AIC_EVENT_SETTING_CHANGE,       /**< Setting value changed */
    AIC_EVENT_UI_UPDATE,            /**< UI needs refresh */
    AIC_EVENT_DATA_READY,           /**< Data processing complete */

    /* Custom Events (81-100) */
    AIC_EVENT_CUSTOM_1 = 81,
    AIC_EVENT_CUSTOM_2,
    AIC_EVENT_CUSTOM_3,
    AIC_EVENT_CUSTOM_4,
    AIC_EVENT_CUSTOM_5,

    AIC_EVENT_MAX = 100
} aic_event_t;

/*******************************************************************************
 * Event Data Structures
 ******************************************************************************/

/**
 * @brief IMU event data
 */
typedef struct {
    int16_t ax, ay, az;     /**< Accelerometer (mg or raw) */
    int16_t gx, gy, gz;     /**< Gyroscope (mdps or raw) */
    uint32_t timestamp;     /**< Timestamp in ms */
} aic_event_imu_t;

/**
 * @brief ADC event data
 */
typedef struct {
    uint8_t channel;        /**< ADC channel */
    uint16_t raw_value;     /**< Raw ADC value */
    uint16_t voltage_mv;    /**< Voltage in millivolts */
} aic_event_adc_t;

/**
 * @brief Button event data
 */
typedef struct {
    uint8_t button_id;      /**< Button identifier */
    bool pressed;           /**< true = pressed, false = released */
    uint32_t duration_ms;   /**< Duration for long press */
} aic_event_button_t;

/**
 * @brief Temperature event data
 */
typedef struct {
    int16_t value;          /**< Temperature in 0.01°C */
    int8_t integer;         /**< Integer part in °C */
    uint8_t decimal;        /**< Decimal part (0-99) */
} aic_event_temp_t;

/**
 * @brief Generic event data (for custom events)
 */
typedef struct {
    uint32_t param1;
    uint32_t param2;
    void *data;
} aic_event_generic_t;

/**
 * @brief Union of all event data types
 */
typedef union {
    aic_event_imu_t imu;
    aic_event_adc_t adc;
    aic_event_button_t button;
    aic_event_temp_t temp;
    aic_event_generic_t generic;
} aic_event_data_t;

/*******************************************************************************
 * Callback Type
 ******************************************************************************/

/**
 * @brief Event callback function type
 * @param event Event type
 * @param data Pointer to event data (type depends on event)
 * @param user_data User-defined data passed during subscription
 */
typedef void (*aic_event_cb_t)(aic_event_t event, const aic_event_data_t *data, void *user_data);

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define AIC_EVENT_MAX_SUBSCRIBERS   (8U)    /**< Max subscribers per event */
#define AIC_EVENT_QUEUE_SIZE        (16U)   /**< Event queue size */
#define AIC_EVENT_TASK_STACK        (256U)  /**< Event task stack size */
#define AIC_EVENT_TASK_PRIORITY     (3U)    /**< Event task priority */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize event bus
 * @return true on success
 */
bool aic_event_init(void);

/**
 * @brief Deinitialize event bus
 */
void aic_event_deinit(void);

/**
 * @brief Check if event bus is initialized
 * @return true if initialized
 */
bool aic_event_is_init(void);

/**
 * @brief Subscribe to an event
 * @param event Event type to subscribe to
 * @param callback Function to call when event occurs
 * @param user_data User data passed to callback
 * @return true on success, false if max subscribers reached
 */
bool aic_event_subscribe(aic_event_t event, aic_event_cb_t callback, void *user_data);

/**
 * @brief Unsubscribe from an event
 * @param event Event type
 * @param callback Callback to remove
 * @return true if callback was found and removed
 */
bool aic_event_unsubscribe(aic_event_t event, aic_event_cb_t callback);

/**
 * @brief Unsubscribe all callbacks for an event
 * @param event Event type
 */
void aic_event_unsubscribe_all(aic_event_t event);

/**
 * @brief Publish an event (queued, non-blocking)
 * @param event Event type
 * @param data Event data (can be NULL)
 * @return true if queued successfully
 */
bool aic_event_publish(aic_event_t event, const aic_event_data_t *data);

/**
 * @brief Publish an event immediately (blocking)
 * @param event Event type
 * @param data Event data (can be NULL)
 *
 * Callbacks are called directly in the caller's context.
 * Use with caution - may cause issues if called from ISR.
 */
void aic_event_publish_immediate(aic_event_t event, const aic_event_data_t *data);

/**
 * @brief Get number of subscribers for an event
 * @param event Event type
 * @return Number of subscribers
 */
uint8_t aic_event_subscriber_count(aic_event_t event);

/**
 * @brief Get event queue count
 * @return Number of pending events
 */
uint32_t aic_event_queue_count(void);

/*******************************************************************************
 * Helper Functions for Common Events
 ******************************************************************************/

/**
 * @brief Publish IMU update event
 */
static inline bool aic_event_publish_imu(int16_t ax, int16_t ay, int16_t az,
                                         int16_t gx, int16_t gy, int16_t gz)
{
    aic_event_data_t data = {
        .imu = { .ax = ax, .ay = ay, .az = az,
                 .gx = gx, .gy = gy, .gz = gz,
                 .timestamp = 0 }
    };
    return aic_event_publish(AIC_EVENT_IMU_UPDATE, &data);
}

/**
 * @brief Publish ADC update event
 */
static inline bool aic_event_publish_adc(uint8_t channel, uint16_t raw_value, uint16_t voltage_mv)
{
    aic_event_data_t data = {
        .adc = { .channel = channel, .raw_value = raw_value, .voltage_mv = voltage_mv }
    };
    return aic_event_publish(AIC_EVENT_ADC_UPDATE, &data);
}

/**
 * @brief Publish button event
 */
static inline bool aic_event_publish_button(uint8_t button_id, bool pressed)
{
    aic_event_data_t data = {
        .button = { .button_id = button_id, .pressed = pressed, .duration_ms = 0 }
    };
    return aic_event_publish(pressed ? AIC_EVENT_BUTTON_PRESS : AIC_EVENT_BUTTON_RELEASE, &data);
}

/**
 * @brief Publish temperature event
 */
static inline bool aic_event_publish_temp(int16_t temp_centi)
{
    aic_event_data_t data = {
        .temp = {
            .value = temp_centi,
            .integer = (int8_t)(temp_centi / 100),
            .decimal = (uint8_t)((temp_centi < 0 ? -temp_centi : temp_centi) % 100)
        }
    };
    return aic_event_publish(AIC_EVENT_TEMP_UPDATE, &data);
}

/*******************************************************************************
 * FreeRTOS Task (Internal)
 ******************************************************************************/

/**
 * @brief Create event processing task
 */
void aic_event_create_task(void);

/**
 * @brief Delete event processing task
 */
void aic_event_delete_task(void);

/**
 * @brief Process event queue (for non-FreeRTOS or manual processing)
 */
void aic_event_process(void);

#ifdef __cplusplus
}
#endif

#endif /* AIC_EVENT_H */
