/*******************************************************************************
 * File Name:   imu_shared.h
 *
 * Description: Shared memory structure for IMU (BMI270) data between CM33 and CM55
 *              Used for Inter-Processor Communication (IPC)
 *
 *              CM33 writes IMU data (from BMI270 via I2C) to shared memory
 *              CM55 reads from shared memory and displays on LVGL
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *              Embedded Systems Engineering, Burapha University
 *
 * Target:      PSoC Edge E84 Evaluation Kit
 *
 * Memory Map:
 *   - m33_m55_shared region: 0x261C0000, size 256KB
 *   - CAPSENSE data: offset 0x00 (64 bytes)
 *   - IMU data: offset 0x40 (64 bytes)
 *
 * Usage:
 *   CM33 (writer):
 *     #include "imu_shared.h"
 *     imu_shared_t *imu = IMU_SHARED_PTR;
 *     imu->accel_x = ax;
 *     imu->valid = true;
 *
 *   CM55 (reader):
 *     #include "imu_shared.h"
 *     imu_shared_t *imu = IMU_SHARED_PTR;
 *     if (imu->valid) { float ax = imu->accel_x; }
 *
 ******************************************************************************/

#ifndef IMU_SHARED_H
#define IMU_SHARED_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Shared Memory Configuration
 ******************************************************************************/

/* Shared memory base address (from linker script) */
#define SHARED_MEM_BASE_ADDR        (0x261C0000UL)

/* IMU data offset in shared memory (after CAPSENSE 64 bytes) */
#define IMU_SHARED_OFFSET           (0x00000040UL)

/* Magic number to verify valid data (0x1AACC00D = "IMU ACCEL GOOD") */
#define IMU_SHARED_MAGIC            (0x1AACC00DUL)

/*******************************************************************************
 * IMU Shared Data Structure
 *
 * Layout (64 bytes):
 *   Offset 0:  magic (4 bytes) - Must be IMU_SHARED_MAGIC
 *   Offset 4:  version (4 bytes)
 *   Offset 8:  valid (4 bytes) - Set true when data is valid
 *   Offset 12: update_count (4 bytes) - Incremented on each update
 *   Offset 16: accel_x (4 bytes, float, m/s^2)
 *   Offset 20: accel_y (4 bytes, float, m/s^2)
 *   Offset 24: accel_z (4 bytes, float, m/s^2)
 *   Offset 28: gyro_x (4 bytes, float, rad/s)
 *   Offset 32: gyro_y (4 bytes, float, rad/s)
 *   Offset 36: gyro_z (4 bytes, float, rad/s)
 *   Offset 40-63: reserved
 *
 ******************************************************************************/

typedef struct __attribute__((packed, aligned(4))) {
    /* Header - verification and versioning */
    uint32_t magic;             /* Must be IMU_SHARED_MAGIC */
    uint32_t version;           /* Structure version (currently 1) */
    uint32_t valid;             /* 1 = data valid, 0 = not yet written */
    uint32_t update_count;      /* Incremented each time CM33 updates */
    uint32_t write_lock;        /* Odd = writing, Even = done (for sync) */

    /* Accelerometer data (m/s^2) */
    float accel_x;              /* X-axis acceleration */
    float accel_y;              /* Y-axis acceleration */
    float accel_z;              /* Z-axis acceleration */

    /* Gyroscope data (rad/s) */
    float gyro_x;               /* X-axis angular velocity */
    float gyro_y;               /* Y-axis angular velocity */
    float gyro_z;               /* Z-axis angular velocity */

    /* Raw sensor data (for debugging) */
    int16_t accel_raw_x;        /* Raw accel X */
    int16_t accel_raw_y;        /* Raw accel Y */
    int16_t accel_raw_z;        /* Raw accel Z */
    int16_t gyro_raw_x;         /* Raw gyro X */
    int16_t gyro_raw_y;         /* Raw gyro Y */
    int16_t gyro_raw_z;         /* Raw gyro Z */

    /* Timestamps and status */
    uint32_t last_read_time_ms; /* Time of last I2C read (from CM33) */
    uint32_t error_count;       /* I2C/sensor error counter */
    uint32_t wdt_reset_count;   /* Watchdog reset counter (incremented on WDT reset) */

} imu_shared_t;

/*******************************************************************************
 * Pointer to shared IMU data
 ******************************************************************************/

#define IMU_SHARED_PTR  ((volatile imu_shared_t *)(SHARED_MEM_BASE_ADDR + IMU_SHARED_OFFSET))

/*******************************************************************************
 * Helper Macros
 ******************************************************************************/

/* Check if IMU data is valid */
#define IMU_SHARED_IS_VALID() \
    (IMU_SHARED_PTR->magic == IMU_SHARED_MAGIC && IMU_SHARED_PTR->valid != 0)

/* Get update count (to detect new data) */
#define IMU_SHARED_GET_COUNT() \
    (IMU_SHARED_PTR->update_count)

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize shared IMU structure (called by CM33 at startup)
 * @param preserve_wdt_count If true, preserve wdt_reset_count (for WDT reset detection)
 */
static inline void imu_shared_init_ex(bool preserve_wdt_count)
{
    volatile imu_shared_t *imu = IMU_SHARED_PTR;
    uint32_t saved_wdt_count = imu->wdt_reset_count;

    imu->magic = IMU_SHARED_MAGIC;
    imu->version = 1;
    imu->valid = 0;
    imu->update_count = 0;
    imu->write_lock = 0;  /* Even = not writing */
    imu->accel_x = 0.0f;
    imu->accel_y = 0.0f;
    imu->accel_z = 1.0f;  /* Default 1g on Z when flat */
    imu->gyro_x = 0.0f;
    imu->gyro_y = 0.0f;
    imu->gyro_z = 0.0f;
    imu->accel_raw_x = 0;
    imu->accel_raw_y = 0;
    imu->accel_raw_z = 0;
    imu->gyro_raw_x = 0;
    imu->gyro_raw_y = 0;
    imu->gyro_raw_z = 0;
    imu->last_read_time_ms = 0;
    imu->error_count = 0;

    /* Preserve or reset WDT counter */
    if (preserve_wdt_count && saved_wdt_count < 1000) {
        imu->wdt_reset_count = saved_wdt_count;
    } else {
        imu->wdt_reset_count = 0;
    }
}

/**
 * @brief Initialize shared IMU structure (called by CM33 at startup)
 */
static inline void imu_shared_init(void)
{
    imu_shared_init_ex(false);
}

/**
 * @brief Increment WDT reset counter (called after WDT reset detected)
 */
static inline void imu_shared_wdt_reset(void)
{
    IMU_SHARED_PTR->wdt_reset_count++;
}

/**
 * @brief Update IMU data in shared memory (called by CM33 after BMI270 read)
 *
 * Uses write_lock for synchronization:
 *   - Odd value = writing in progress
 *   - Even value = write complete, data consistent
 *
 * @param ax Accelerometer X (m/s^2)
 * @param ay Accelerometer Y (m/s^2)
 * @param az Accelerometer Z (m/s^2)
 * @param gx Gyroscope X (rad/s)
 * @param gy Gyroscope Y (rad/s)
 * @param gz Gyroscope Z (rad/s)
 * @param time_ms Current time in milliseconds
 */
static inline void imu_shared_update(float ax, float ay, float az,
                                     float gx, float gy, float gz,
                                     uint32_t time_ms)
{
    volatile imu_shared_t *imu = IMU_SHARED_PTR;

    /* Begin write - make write_lock odd */
    imu->write_lock++;
    __DSB();  /* Data Synchronization Barrier - ensure write_lock is visible */

    /* Write data */
    imu->accel_x = ax;
    imu->accel_y = ay;
    imu->accel_z = az;
    imu->gyro_x = gx;
    imu->gyro_y = gy;
    imu->gyro_z = gz;
    imu->last_read_time_ms = time_ms;
    imu->update_count++;
    imu->valid = 1;

    /* End write - make write_lock even */
    __DSB();  /* Ensure all data written before write_lock update */
    imu->write_lock++;
}

/**
 * @brief Update IMU raw data (called by CM33 for debugging)
 */
static inline void imu_shared_update_raw(int16_t ax, int16_t ay, int16_t az,
                                          int16_t gx, int16_t gy, int16_t gz)
{
    volatile imu_shared_t *imu = IMU_SHARED_PTR;

    imu->accel_raw_x = ax;
    imu->accel_raw_y = ay;
    imu->accel_raw_z = az;
    imu->gyro_raw_x = gx;
    imu->gyro_raw_y = gy;
    imu->gyro_raw_z = gz;
}

/**
 * @brief Increment error count (called by CM33 on I2C/sensor error)
 */
static inline void imu_shared_error(void)
{
    IMU_SHARED_PTR->error_count++;
}

/**
 * @brief Read accelerometer data from shared memory (called by CM55)
 *
 * Uses write_lock for synchronization to avoid reading partial data.
 * Returns false if CM33 is currently writing (caller should retry or use cached value).
 *
 * @param ax Pointer to store X acceleration
 * @param ay Pointer to store Y acceleration
 * @param az Pointer to store Z acceleration
 * @return true if data is valid and consistent, false otherwise
 */
static inline bool imu_shared_read_accel(float *ax, float *ay, float *az)
{
    volatile imu_shared_t *imu = IMU_SHARED_PTR;

    /* Data Synchronization Barrier - ensure cache coherency */
    __DSB();

    /* Check if data is valid */
    if (imu->magic != IMU_SHARED_MAGIC || imu->valid == 0) {
        return false;
    }

    /* Read write_lock before reading data */
    uint32_t lock1 = imu->write_lock;
    __DSB();

    /* If odd, CM33 is writing - skip this read */
    if (lock1 & 1) {
        return false;
    }

    /* Read data */
    float tmp_ax = imu->accel_x;
    float tmp_ay = imu->accel_y;
    float tmp_az = imu->accel_z;

    /* Read write_lock again to verify data consistency */
    __DSB();
    uint32_t lock2 = imu->write_lock;

    /* If lock changed, data may be inconsistent - skip */
    if (lock1 != lock2) {
        return false;
    }

    /* Data is consistent - copy to output */
    if (ax) *ax = tmp_ax;
    if (ay) *ay = tmp_ay;
    if (az) *az = tmp_az;

    return true;
}

/**
 * @brief Read gyroscope data from shared memory (called by CM55)
 *
 * Uses write_lock for synchronization to avoid reading partial data.
 * Returns false if CM33 is currently writing (caller should retry or use cached value).
 *
 * @param gx Pointer to store X angular velocity
 * @param gy Pointer to store Y angular velocity
 * @param gz Pointer to store Z angular velocity
 * @return true if data is valid and consistent, false otherwise
 */
static inline bool imu_shared_read_gyro(float *gx, float *gy, float *gz)
{
    volatile imu_shared_t *imu = IMU_SHARED_PTR;

    /* Data Synchronization Barrier - ensure cache coherency */
    __DSB();

    /* Check if data is valid */
    if (imu->magic != IMU_SHARED_MAGIC || imu->valid == 0) {
        return false;
    }

    /* Read write_lock before reading data */
    uint32_t lock1 = imu->write_lock;
    __DSB();

    /* If odd, CM33 is writing - skip this read */
    if (lock1 & 1) {
        return false;
    }

    /* Read data */
    float tmp_gx = imu->gyro_x;
    float tmp_gy = imu->gyro_y;
    float tmp_gz = imu->gyro_z;

    /* Read write_lock again to verify data consistency */
    __DSB();
    uint32_t lock2 = imu->write_lock;

    /* If lock changed, data may be inconsistent - skip */
    if (lock1 != lock2) {
        return false;
    }

    /* Data is consistent - copy to output */
    if (gx) *gx = tmp_gx;
    if (gy) *gy = tmp_gy;
    if (gz) *gz = tmp_gz;

    return true;
}

/**
 * @brief Read all IMU data from shared memory (called by CM55)
 * @param ax, ay, az Pointers to store acceleration
 * @param gx, gy, gz Pointers to store angular velocity
 * @return true if data is valid, false otherwise
 */
static inline bool imu_shared_read_all(float *ax, float *ay, float *az,
                                        float *gx, float *gy, float *gz)
{
    bool accel_ok = imu_shared_read_accel(ax, ay, az);
    bool gyro_ok = imu_shared_read_gyro(gx, gy, gz);
    return accel_ok && gyro_ok;
}

#endif /* IMU_SHARED_H */
