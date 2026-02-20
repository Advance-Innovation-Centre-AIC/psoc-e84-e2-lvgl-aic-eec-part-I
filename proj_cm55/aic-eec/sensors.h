/*******************************************************************************
 * File Name:   sensors.h
 *
 * Description: AIC-EEC Sensor Hardware Abstraction Layer
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Components:
 *   - ADC (Analog-to-Digital Converter)
 *   - IMU (Inertial Measurement Unit - BMI270)
 *   - Temperature Sensor (internal)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 * Usage:
 *   #include "sensors.h"
 *   aic_sensors_init();
 *   float voltage = aic_adc_read_voltage(AIC_ADC_CH0);
 *
 ******************************************************************************/

#ifndef AIC_SENSORS_H
#define AIC_SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * ADC Definitions
 ******************************************************************************/

/** ADC channel identifiers */
typedef enum {
    AIC_ADC_CH0 = 0,        /**< ADC Channel 0 (Potentiometer) */
    AIC_ADC_CH1,            /**< ADC Channel 1 */
    AIC_ADC_CH2,            /**< ADC Channel 2 */
    AIC_ADC_CH3,            /**< ADC Channel 3 */
    AIC_ADC_TEMP,           /**< Internal temperature sensor */
    AIC_ADC_CH_COUNT        /**< Total number of ADC channels */
} aic_adc_channel_t;

/** ADC resolution */
typedef enum {
    AIC_ADC_RES_8BIT = 8,   /**< 8-bit resolution (0-255) */
    AIC_ADC_RES_10BIT = 10, /**< 10-bit resolution (0-1023) */
    AIC_ADC_RES_12BIT = 12  /**< 12-bit resolution (0-4095) */
} aic_adc_resolution_t;

/*******************************************************************************
 * IMU Definitions
 ******************************************************************************/

/** IMU axis identifiers */
typedef enum {
    AIC_AXIS_X = 0,
    AIC_AXIS_Y,
    AIC_AXIS_Z,
    AIC_AXIS_COUNT
} aic_axis_t;

/** IMU orientation states */
typedef enum {
    AIC_ORIENT_UNKNOWN = 0,
    AIC_ORIENT_FLAT_UP,         /**< Screen facing up */
    AIC_ORIENT_FLAT_DOWN,       /**< Screen facing down */
    AIC_ORIENT_PORTRAIT,        /**< Portrait orientation */
    AIC_ORIENT_PORTRAIT_INV,    /**< Portrait inverted */
    AIC_ORIENT_LANDSCAPE,       /**< Landscape orientation */
    AIC_ORIENT_LANDSCAPE_INV    /**< Landscape inverted */
} aic_orientation_t;

/** IMU data structure */
typedef struct {
    float accel_x;      /**< Accelerometer X-axis (g) */
    float accel_y;      /**< Accelerometer Y-axis (g) */
    float accel_z;      /**< Accelerometer Z-axis (g) */
    float gyro_x;       /**< Gyroscope X-axis (deg/s) */
    float gyro_y;       /**< Gyroscope Y-axis (deg/s) */
    float gyro_z;       /**< Gyroscope Z-axis (deg/s) */
    int32_t accel_raw_x; /**< Accelerometer raw X */
    int32_t accel_raw_y; /**< Accelerometer raw Y */
    int32_t accel_raw_z; /**< Accelerometer raw Z */
    int32_t gyro_raw_x;  /**< Gyroscope raw X */
    int32_t gyro_raw_y;  /**< Gyroscope raw Y */
    int32_t gyro_raw_z;  /**< Gyroscope raw Z */
} aic_imu_data_t;

/*******************************************************************************
 * Sensor Initialization
 ******************************************************************************/

/**
 * @brief Initialize all sensors
 * @return true if initialization successful, false otherwise
 */
bool aic_sensors_init(void);

/**
 * @brief Deinitialize all sensors
 */
void aic_sensors_deinit(void);

/*******************************************************************************
 * ADC Functions
 ******************************************************************************/

/**
 * @brief Initialize ADC subsystem
 * @param resolution ADC resolution (8, 10, or 12 bit)
 * @return true if initialization successful
 */
bool aic_adc_init(aic_adc_resolution_t resolution);

/**
 * @brief Read raw ADC value from channel
 * @param channel ADC channel to read
 * @return Raw ADC value (0 to max based on resolution)
 */
uint16_t aic_adc_read(aic_adc_channel_t channel);

/**
 * @brief Read ADC value as voltage
 * @param channel ADC channel to read
 * @return Voltage in volts (0 to VREF)
 */
float aic_adc_read_voltage(aic_adc_channel_t channel);

/**
 * @brief Read ADC value as percentage
 * @param channel ADC channel to read
 * @return Percentage (0-100)
 */
uint8_t aic_adc_read_percent(aic_adc_channel_t channel);

/**
 * @brief Set ADC reference voltage
 * @param vref_mv Reference voltage in millivolts (default 3300)
 */
void aic_adc_set_vref(uint16_t vref_mv);

/**
 * @brief Get current ADC reference voltage
 * @return Reference voltage in millivolts
 */
uint16_t aic_adc_get_vref(void);

/**
 * @brief Read internal temperature sensor
 * @return Temperature in degrees Celsius
 */
float aic_adc_read_temperature(void);

/*******************************************************************************
 * IMU (BMI270) Functions
 ******************************************************************************/

/**
 * @brief Initialize IMU sensor
 * @return true if initialization successful
 */
bool aic_imu_init(void);

/**
 * @brief Read accelerometer data
 * @param ax Pointer to store X-axis acceleration (g)
 * @param ay Pointer to store Y-axis acceleration (g)
 * @param az Pointer to store Z-axis acceleration (g)
 * @return true if read successful
 */
bool aic_imu_read_accel(float *ax, float *ay, float *az);

/**
 * @brief Read gyroscope data
 * @param gx Pointer to store X-axis angular velocity (deg/s)
 * @param gy Pointer to store Y-axis angular velocity (deg/s)
 * @param gz Pointer to store Z-axis angular velocity (deg/s)
 * @return true if read successful
 */
bool aic_imu_read_gyro(float *gx, float *gy, float *gz);

/**
 * @brief Read all IMU data (accelerometer + gyroscope)
 * @param data Pointer to IMU data structure
 * @return true if read successful
 */
bool aic_imu_read_all(aic_imu_data_t *data);

/**
 * @brief Get device orientation based on accelerometer
 * @return Current orientation state
 */
aic_orientation_t aic_imu_get_orientation(void);

/**
 * @brief Get orientation name string
 * @param orient Orientation value
 * @return Orientation name string
 */
const char* aic_imu_orientation_name(aic_orientation_t orient);

/**
 * @brief Calibrate IMU (zero offset)
 * @return true if calibration successful
 * @note Device should be stationary during calibration
 */
bool aic_imu_calibrate(void);

/**
 * @brief Check if IMU is available
 * @return true if IMU is detected and ready
 */
bool aic_imu_is_available(void);

/*******************************************************************************
 * CAPSENSE (I2C) Definitions - PSoC 4000T Touch Controller
 ******************************************************************************/

/** CAPSENSE I2C address (PSoC 4000T) */
#define AIC_CAPSENSE_I2C_ADDR  0x08

/** CAPSENSE data structure */
typedef struct {
    bool btn0_pressed;      /**< Button 0 (CSB1) state */
    bool btn1_pressed;      /**< Button 1 (CSB2) state */
    uint8_t slider_pos;     /**< Slider position (0-100, 0=no touch) */
    bool slider_active;     /**< Slider is being touched */
} aic_capsense_data_t;

/*******************************************************************************
 * CAPSENSE Functions
 ******************************************************************************/

/**
 * @brief Initialize CAPSENSE I2C communication
 * @return true if initialization successful
 */
bool aic_capsense_init(void);

/**
 * @brief Read CAPSENSE data from PSoC 4000T via I2C
 * @param data Pointer to CAPSENSE data structure
 * @return true if read successful
 */
bool aic_capsense_read(aic_capsense_data_t *data);

/**
 * @brief Check if CAPSENSE is available
 * @return true if CAPSENSE responds on I2C
 */
bool aic_capsense_is_available(void);

/*******************************************************************************
 * Simulation Functions (for UI development without hardware)
 ******************************************************************************/

/**
 * @brief Enable simulation mode
 * @param enable true to enable simulation, false to use hardware
 */
void aic_sensors_set_simulation(bool enable);

/**
 * @brief Check if simulation mode is enabled
 * @return true if in simulation mode
 */
bool aic_sensors_is_simulation(void);

/**
 * @brief Set simulated ADC value
 * @param channel ADC channel
 * @param value Simulated value (0 to max resolution)
 */
void aic_adc_set_simulated(aic_adc_channel_t channel, uint16_t value);

/**
 * @brief Set simulated IMU accelerometer data
 * @param ax X-axis acceleration (g)
 * @param ay Y-axis acceleration (g)
 * @param az Z-axis acceleration (g)
 */
void aic_imu_set_simulated_accel(float ax, float ay, float az);

/**
 * @brief Set simulated IMU gyroscope data
 * @param gx X-axis angular velocity (deg/s)
 * @param gy Y-axis angular velocity (deg/s)
 * @param gz Z-axis angular velocity (deg/s)
 */
void aic_imu_set_simulated_gyro(float gx, float gy, float gz);

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get ADC channel name string
 * @param channel ADC channel
 * @return Channel name string
 */
const char* aic_adc_channel_name(aic_adc_channel_t channel);

/**
 * @brief Convert raw ADC value to voltage
 * @param raw Raw ADC value
 * @param resolution ADC resolution used
 * @param vref_mv Reference voltage in millivolts
 * @return Voltage in volts
 */
float aic_adc_raw_to_voltage(uint16_t raw, aic_adc_resolution_t resolution, uint16_t vref_mv);

/**
 * @brief Print sensor status information
 */
void aic_sensors_print_status(void);

#endif /* AIC_SENSORS_H */
