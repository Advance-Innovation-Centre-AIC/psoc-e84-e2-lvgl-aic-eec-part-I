/*******************************************************************************
 * File Name:   tilt.c
 *
 * Description: Tilt Analysis Module Implementation
 *              Uses Complementary Filter to fuse Accelerometer + Gyroscope
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Algorithm Details:
 * =================
 *
 * 1. Accelerometer-based angle (static):
 *    - Roll  = atan2(ay, az) * 180/PI
 *    - Pitch = atan2(-ax, sqrt(ay^2 + az^2)) * 180/PI
 *
 *    Pros: No drift, gives absolute angle relative to gravity
 *    Cons: Noisy, affected by linear acceleration (vibration, movement)
 *
 * 2. Gyroscope-based angle (dynamic):
 *    - Roll  += gx * dt * 180/PI
 *    - Pitch += gy * dt * 180/PI
 *
 *    Pros: Fast response, smooth, not affected by linear acceleration
 *    Cons: Drifts over time due to integration of bias/noise
 *
 * 3. Complementary Filter (fusion):
 *    angle = alpha * (angle + gyro_rate * dt) + (1 - alpha) * accel_angle
 *
 *    - High-pass filter on gyroscope (removes DC drift)
 *    - Low-pass filter on accelerometer (removes high-freq noise)
 *    - alpha = 0.98 means: trust gyro 98% for fast changes,
 *                          trust accel 2% for long-term correction
 *
 ******************************************************************************/

#include "tilt.h"
#include "sensors.h"
#include <math.h>
#include <stdio.h>

/*******************************************************************************
 * Constants
 ******************************************************************************/

#define RAD_TO_DEG      (57.2957795f)   /* 180 / PI */
#define DEG_TO_RAD      (0.0174533f)    /* PI / 180 */
#define GRAVITY_NORM    (9.80665f)      /* Standard gravity m/s^2 */

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Filter configuration */
static float filter_alpha = AIC_TILT_DEFAULT_ALPHA;
static float filter_dt = AIC_TILT_DEFAULT_DT;

/* Current tilt state */
static aic_tilt_state_t tilt_state = {
    .roll = 0.0f,
    .pitch = 0.0f,
    .roll_rate = 0.0f,
    .pitch_rate = 0.0f,
    .initialized = false
};

/* Module initialization flag */
static bool module_initialized = false;

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Calculate roll angle from accelerometer (gravity vector)
 *
 * Roll = atan2(ay, az)
 * When board is level: ay=0, az=+g -> roll=0
 * When tilted right:   ay=+g, az=0 -> roll=+90
 * When tilted left:    ay=-g, az=0 -> roll=-90
 */
static float calc_accel_roll(float ay, float az)
{
    return atan2f(ay, az) * RAD_TO_DEG;
}

/**
 * @brief Calculate pitch angle from accelerometer (gravity vector)
 *
 * Pitch = atan2(-ax, sqrt(ay^2 + az^2))
 * When board is level:    ax=0  -> pitch=0
 * When tilted forward:    ax=-g -> pitch=+90
 * When tilted backward:   ax=+g -> pitch=-90
 */
static float calc_accel_pitch(float ax, float ay, float az)
{
    float yz_mag = sqrtf(ay * ay + az * az);
    return atan2f(-ax, yz_mag) * RAD_TO_DEG;
}

/**
 * @brief Normalize angle to -180 to +180 range
 */
static float normalize_angle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

/**
 * @brief Clamp value to range
 */
static float clamp_f(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

bool aic_tilt_init(const aic_tilt_config_t *config)
{
    /* Apply configuration or use defaults */
    if (config != NULL) {
        filter_alpha = clamp_f(config->alpha, 0.0f, 1.0f);
        filter_dt = config->dt;
    } else {
        filter_alpha = AIC_TILT_DEFAULT_ALPHA;
        filter_dt = AIC_TILT_DEFAULT_DT;
    }

    /* Reset state */
    tilt_state.roll = 0.0f;
    tilt_state.pitch = 0.0f;
    tilt_state.roll_rate = 0.0f;
    tilt_state.pitch_rate = 0.0f;
    tilt_state.initialized = false;

    module_initialized = true;

    printf("[Tilt] Initialized: alpha=%.2f, dt=%.3fs\r\n",
           (double)filter_alpha, (double)filter_dt);

    return true;
}

bool aic_tilt_update(float ax, float ay, float az,
                     float gx, float gy, float gz)
{
    if (!module_initialized) {
        aic_tilt_init(NULL);
    }

    /* Convert gyroscope from rad/s to deg/s */
    float gyro_roll_rate = gx * RAD_TO_DEG;   /* Roll rate (deg/s) */
    float gyro_pitch_rate = gy * RAD_TO_DEG;  /* Pitch rate (deg/s) */

    /* Store angular rates */
    tilt_state.roll_rate = gyro_roll_rate;
    tilt_state.pitch_rate = gyro_pitch_rate;

    /* Calculate accelerometer-based angles */
    float accel_roll = calc_accel_roll(ay, az);
    float accel_pitch = calc_accel_pitch(ax, ay, az);

    /* First update: Initialize from accelerometer */
    if (!tilt_state.initialized) {
        tilt_state.roll = accel_roll;
        tilt_state.pitch = accel_pitch;
        tilt_state.initialized = true;
        printf("[Tilt] First reading: roll=%.1f, pitch=%.1f\r\n",
               (double)tilt_state.roll, (double)tilt_state.pitch);
        return true;
    }

    /* Complementary Filter:
     * angle = alpha * (angle + gyro_rate * dt) + (1-alpha) * accel_angle
     *
     * The gyro integration (angle + gyro_rate * dt) provides fast response
     * The accel angle provides long-term stability (corrects drift)
     */
    float gyro_roll = tilt_state.roll + gyro_roll_rate * filter_dt;
    float gyro_pitch = tilt_state.pitch + gyro_pitch_rate * filter_dt;

    /* Apply complementary filter */
    tilt_state.roll = filter_alpha * gyro_roll + (1.0f - filter_alpha) * accel_roll;
    tilt_state.pitch = filter_alpha * gyro_pitch + (1.0f - filter_alpha) * accel_pitch;

    /* Normalize angles */
    tilt_state.roll = normalize_angle(tilt_state.roll);
    tilt_state.pitch = clamp_f(tilt_state.pitch, -90.0f, 90.0f);

    return true;
}

bool aic_tilt_get_state(aic_tilt_state_t *state)
{
    if (state == NULL) {
        return false;
    }

    *state = tilt_state;
    return tilt_state.initialized;
}

float aic_tilt_get_roll(void)
{
    return tilt_state.roll;
}

float aic_tilt_get_pitch(void)
{
    return tilt_state.pitch;
}

uint8_t aic_tilt_get_roll_percent(void)
{
    /* Map -90..+90 degrees to 0..100% */
    /* -90 = 0%, 0 = 50%, +90 = 100% */
    float clamped = clamp_f(tilt_state.roll, -90.0f, 90.0f);
    float percent = (clamped + 90.0f) / 180.0f * 100.0f;
    return (uint8_t)clamp_f(percent, 0.0f, 100.0f);
}

uint8_t aic_tilt_get_pitch_percent(void)
{
    /* Map -90..+90 degrees to 0..100% */
    /* -90 = 0%, 0 = 50%, +90 = 100% */
    float clamped = clamp_f(tilt_state.pitch, -90.0f, 90.0f);
    float percent = (clamped + 90.0f) / 180.0f * 100.0f;
    return (uint8_t)clamp_f(percent, 0.0f, 100.0f);
}

void aic_tilt_reset(void)
{
    tilt_state.roll = 0.0f;
    tilt_state.pitch = 0.0f;
    tilt_state.roll_rate = 0.0f;
    tilt_state.pitch_rate = 0.0f;
    tilt_state.initialized = false;
    printf("[Tilt] Reset\r\n");
}

void aic_tilt_set_alpha(float alpha)
{
    filter_alpha = clamp_f(alpha, 0.0f, 1.0f);
    printf("[Tilt] Alpha set to %.2f\r\n", (double)filter_alpha);
}

void aic_tilt_set_dt(float dt)
{
    filter_dt = dt;
    printf("[Tilt] dt set to %.3fs\r\n", (double)filter_dt);
}

bool aic_tilt_update_from_imu(void)
{
    float ax, ay, az;
    float gx, gy, gz;

    /* Read accelerometer */
    if (!aic_imu_read_accel(&ax, &ay, &az)) {
        return false;
    }

    /* Read gyroscope */
    if (!aic_imu_read_gyro(&gx, &gy, &gz)) {
        return false;
    }

    /* Update tilt estimation */
    return aic_tilt_update(ax, ay, az, gx, gy, gz);
}

/* [] END OF FILE */
