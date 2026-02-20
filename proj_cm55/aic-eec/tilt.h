/*******************************************************************************
 * File Name:   tilt.h
 *
 * Description: Tilt Analysis Module using Complementary Filter
 *              Combines Accelerometer + Gyroscope for accurate tilt estimation
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Algorithm:   Complementary Filter
 *              - Accelerometer: Good for static tilt (gravity vector)
 *              - Gyroscope: Good for dynamic changes (fast response)
 *              - Combined: Best of both worlds
 *
 * Formula:
 *   angle = alpha * (angle + gyro * dt) + (1-alpha) * accel_angle
 *   where alpha = 0.98 (trust gyro for high-freq, accel for low-freq)
 *
 * Coordinate System (PSoC Edge E84 Board):
 *   - Roll:  Rotation around X-axis (tilt left/right)
 *   - Pitch: Rotation around Y-axis (tilt forward/backward)
 *   - Yaw:   Rotation around Z-axis (heading, needs magnetometer)
 *
 ******************************************************************************/

#ifndef AIC_TILT_H
#define AIC_TILT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

/**
 * @brief Tilt state structure
 *
 * Stores the current estimated tilt angles and filter state
 */
typedef struct {
    float roll;         /**< Roll angle in degrees (-180 to +180) */
    float pitch;        /**< Pitch angle in degrees (-90 to +90) */
    float roll_rate;    /**< Roll angular velocity (deg/s) */
    float pitch_rate;   /**< Pitch angular velocity (deg/s) */
    bool initialized;   /**< True after first update */
} aic_tilt_state_t;

/**
 * @brief Tilt filter configuration
 */
typedef struct {
    float alpha;        /**< Complementary filter coefficient (0.0-1.0) */
    float dt;           /**< Sample period in seconds */
} aic_tilt_config_t;

/*******************************************************************************
 * Default Configuration
 ******************************************************************************/

/**
 * Default filter coefficient (0.98 = 98% gyro, 2% accelerometer)
 * Higher alpha = faster response but more drift
 * Lower alpha = slower response but more stable
 */
#define AIC_TILT_DEFAULT_ALPHA      (0.98f)

/**
 * Default sample period (100ms = 10 Hz update rate)
 */
#define AIC_TILT_DEFAULT_DT         (0.1f)

/*******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Initialize the tilt analysis module
 *
 * @param config  Pointer to configuration (NULL for defaults)
 * @return true if successful
 */
bool aic_tilt_init(const aic_tilt_config_t *config);

/**
 * @brief Update tilt estimation with new sensor data
 *
 * Call this function periodically (e.g., every 100ms) with fresh
 * accelerometer and gyroscope readings.
 *
 * @param ax  Accelerometer X (m/s^2)
 * @param ay  Accelerometer Y (m/s^2)
 * @param az  Accelerometer Z (m/s^2)
 * @param gx  Gyroscope X - Roll rate (rad/s)
 * @param gy  Gyroscope Y - Pitch rate (rad/s)
 * @param gz  Gyroscope Z - Yaw rate (rad/s)
 * @return true if update successful
 */
bool aic_tilt_update(float ax, float ay, float az,
                     float gx, float gy, float gz);

/**
 * @brief Get current tilt state
 *
 * @param state  Pointer to receive tilt state
 * @return true if state is valid
 */
bool aic_tilt_get_state(aic_tilt_state_t *state);

/**
 * @brief Get roll angle (X-axis tilt)
 *
 * @return Roll angle in degrees (-180 to +180)
 *         0 = level, +90 = tilted right, -90 = tilted left
 */
float aic_tilt_get_roll(void);

/**
 * @brief Get pitch angle (Y-axis tilt)
 *
 * @return Pitch angle in degrees (-90 to +90)
 *         0 = level, +90 = tilted forward, -90 = tilted backward
 */
float aic_tilt_get_pitch(void);

/**
 * @brief Get roll angle as percentage
 *
 * Maps roll angle to 0-100%:
 *   -90 degrees = 0%
 *     0 degrees = 50%
 *   +90 degrees = 100%
 *
 * @return Roll percentage (0-100)
 */
uint8_t aic_tilt_get_roll_percent(void);

/**
 * @brief Get pitch angle as percentage
 *
 * Maps pitch angle to 0-100%:
 *   -90 degrees = 0%
 *     0 degrees = 50%
 *   +90 degrees = 100%
 *
 * @return Pitch percentage (0-100)
 */
uint8_t aic_tilt_get_pitch_percent(void);

/**
 * @brief Reset tilt estimation to zero
 *
 * Call this to reset the filter state. The next update will
 * initialize from accelerometer readings.
 */
void aic_tilt_reset(void);

/**
 * @brief Set filter coefficient
 *
 * @param alpha  New alpha value (0.0 to 1.0)
 *               Higher = faster response, more drift
 *               Lower = slower response, more stable
 */
void aic_tilt_set_alpha(float alpha);

/**
 * @brief Set sample period
 *
 * @param dt  Sample period in seconds
 */
void aic_tilt_set_dt(float dt);

/**
 * @brief Convenience function: Update from IMU shared memory
 *
 * Reads accelerometer and gyroscope from the sensors module
 * and updates the tilt estimation automatically.
 *
 * @return true if update successful
 */
bool aic_tilt_update_from_imu(void);

#ifdef __cplusplus
}
#endif

#endif /* AIC_TILT_H */
