/*******************************************************************************
 * File Name:   ma_filter.h
 *
 * Description: Moving Average Filter for sensor data smoothing
 *              Used to reduce noise in accelerometer/gyroscope readings
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *              Embedded Systems Engineering, Burapha University
 *
 * Target:      PSoC Edge E84 Evaluation Kit
 *
 * Usage:
 *   // Create filter instance
 *   ma_filter_t my_filter;
 *   ma_filter_init(&my_filter);
 *
 *   // Update filter with new sample and get filtered value
 *   float filtered_value = ma_filter_update(&my_filter, raw_value);
 *
 ******************************************************************************/

#ifndef MA_FILTER_H
#define MA_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*******************************************************************************
 * Configuration
 ******************************************************************************/

/* Default filter window size (number of samples to average)
 * IMU updates every 50ms, so:
 *   - Size 5 = 250ms lag (too slow)
 *   - Size 3 = 150ms lag (balanced)
 *   - Size 2 = 100ms lag (fast, more noise)
 */
#define MA_FILTER_DEFAULT_SIZE      (3U)

/* Maximum supported filter size */
#define MA_FILTER_MAX_SIZE          (20U)

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief Moving Average Filter structure
 *
 * Uses circular buffer for efficient O(1) update operations.
 * Maintains running sum to avoid recalculating average each time.
 */
typedef struct {
    float buffer[MA_FILTER_MAX_SIZE];   /* Circular buffer for samples */
    float sum;                           /* Running sum of all samples */
    uint32_t head;                       /* Next write position */
    uint32_t count;                      /* Number of valid samples */
    uint32_t size;                       /* Filter window size */
} ma_filter_t;

/**
 * @brief 3-axis Moving Average Filter for IMU data
 *
 * Combines three filters for X, Y, Z axes.
 */
typedef struct {
    ma_filter_t x;
    ma_filter_t y;
    ma_filter_t z;
} ma_filter_3axis_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize a moving average filter
 *
 * @param filter    Pointer to filter structure
 * @param size      Window size (1 to MA_FILTER_MAX_SIZE)
 */
static inline void ma_filter_init(ma_filter_t *filter, uint32_t size)
{
    if (filter == NULL) return;

    memset(filter, 0, sizeof(ma_filter_t));

    /* Clamp size to valid range */
    if (size < 1) size = 1;
    if (size > MA_FILTER_MAX_SIZE) size = MA_FILTER_MAX_SIZE;

    filter->size = size;
}

/**
 * @brief Initialize filter with default size
 *
 * @param filter    Pointer to filter structure
 */
static inline void ma_filter_init_default(ma_filter_t *filter)
{
    ma_filter_init(filter, MA_FILTER_DEFAULT_SIZE);
}

/**
 * @brief Reset filter to initial state (clear all samples)
 *
 * @param filter    Pointer to filter structure
 */
static inline void ma_filter_reset(ma_filter_t *filter)
{
    if (filter == NULL) return;

    uint32_t saved_size = filter->size;
    memset(filter, 0, sizeof(ma_filter_t));
    filter->size = saved_size;
}

/**
 * @brief Add new sample to filter and get filtered output
 *
 * Algorithm:
 * 1. Remove oldest sample from running sum (if buffer full)
 * 2. Add new sample to buffer and running sum
 * 3. Update head pointer (circular)
 * 4. Return average = sum / count
 *
 * @param filter        Pointer to filter structure
 * @param new_value     New sample value
 * @return float        Filtered (averaged) value
 */
static inline float ma_filter_update(ma_filter_t *filter, float new_value)
{
    if (filter == NULL || filter->size == 0) return new_value;

    /* Remove old value from sum if buffer is full */
    if (filter->count >= filter->size) {
        filter->sum -= filter->buffer[filter->head];
    } else {
        filter->count++;
    }

    /* Add new value to buffer and sum */
    filter->buffer[filter->head] = new_value;
    filter->sum += new_value;

    /* Move head pointer (circular) */
    filter->head = (filter->head + 1) % filter->size;

    /* Return average */
    return filter->sum / (float)filter->count;
}

/**
 * @brief Get current average without adding new sample
 *
 * @param filter    Pointer to filter structure
 * @return float    Current filtered value
 */
static inline float ma_filter_get_average(const ma_filter_t *filter)
{
    if (filter == NULL || filter->count == 0) return 0.0f;
    return filter->sum / (float)filter->count;
}

/**
 * @brief Check if filter buffer is full
 *
 * @param filter    Pointer to filter structure
 * @return bool     true if buffer contains 'size' samples
 */
static inline bool ma_filter_is_full(const ma_filter_t *filter)
{
    if (filter == NULL) return false;
    return filter->count >= filter->size;
}

/**
 * @brief Get number of samples currently in filter
 *
 * @param filter    Pointer to filter structure
 * @return uint32_t Number of samples
 */
static inline uint32_t ma_filter_get_count(const ma_filter_t *filter)
{
    if (filter == NULL) return 0;
    return filter->count;
}

/*******************************************************************************
 * 3-Axis Filter Functions (for IMU data)
 ******************************************************************************/

/**
 * @brief Initialize 3-axis filter for IMU data
 *
 * @param filter    Pointer to 3-axis filter structure
 * @param size      Window size for all axes
 */
static inline void ma_filter_3axis_init(ma_filter_3axis_t *filter, uint32_t size)
{
    if (filter == NULL) return;

    ma_filter_init(&filter->x, size);
    ma_filter_init(&filter->y, size);
    ma_filter_init(&filter->z, size);
}

/**
 * @brief Initialize 3-axis filter with default size
 *
 * @param filter    Pointer to 3-axis filter structure
 */
static inline void ma_filter_3axis_init_default(ma_filter_3axis_t *filter)
{
    ma_filter_3axis_init(filter, MA_FILTER_DEFAULT_SIZE);
}

/**
 * @brief Reset all 3 axes
 *
 * @param filter    Pointer to 3-axis filter structure
 */
static inline void ma_filter_3axis_reset(ma_filter_3axis_t *filter)
{
    if (filter == NULL) return;

    ma_filter_reset(&filter->x);
    ma_filter_reset(&filter->y);
    ma_filter_reset(&filter->z);
}

/**
 * @brief Update all 3 axes with new samples
 *
 * @param filter    Pointer to 3-axis filter structure
 * @param x_raw     Raw X value
 * @param y_raw     Raw Y value
 * @param z_raw     Raw Z value
 * @param x_out     Pointer to store filtered X value
 * @param y_out     Pointer to store filtered Y value
 * @param z_out     Pointer to store filtered Z value
 */
static inline void ma_filter_3axis_update(ma_filter_3axis_t *filter,
                                           float x_raw, float y_raw, float z_raw,
                                           float *x_out, float *y_out, float *z_out)
{
    if (filter == NULL) return;

    if (x_out) *x_out = ma_filter_update(&filter->x, x_raw);
    if (y_out) *y_out = ma_filter_update(&filter->y, y_raw);
    if (z_out) *z_out = ma_filter_update(&filter->z, z_raw);
}

#endif /* MA_FILTER_H */
