# AIC-EEC SDK

> **Hardware Abstraction Layer for PSoC Edge E84 Evaluation Kit**
>
> Developed by **Assoc. Prof. Wiroon Sriborrirux** (รศ.วิรุฬห์ ศรีบริรักษ์)
>
> Department of Electrical Engineering, Faculty of Engineering, Burapha University
>
> Contact: wiroon@eng.buu.ac.th

---

## Overview

AIC-EEC SDK เป็น Hardware Abstraction Layer (HAL) ที่พัฒนาขึ้นสำหรับ **Embedded Systems and IoT Development** โดยมีวัตถุประสงค์เพื่อ:

1. **Abstraction** - ซ่อนความซับซ้อนของ Hardware Driver และ HAL ของ Infineon
2. **Consistency** - ใช้ API รูปแบบเดียวกันทั้งโปรเจค
3. **Testability** - สามารถ mock/simulate ได้ง่ายสำหรับ UI development
4. **Reusability** - ใช้ซ้ำได้หลาย examples โดยไม่ต้องเขียนโค้ดใหม่

---

## Files Structure

```ini
aic-eec/
├── aic-eec.h          # Main header - UI Components (Footer, Header, Logo)
├── aic-eec.c          # Common utilities implementation
├── aic_layout.h       # Layout Helpers - Flexbox row/col, card, gauge, progress bar
├── aic_layout.c       # Layout implementation + color palette + dark theme
├── aic_event.h        # Event Bus - Publish-subscribe for sensor/button/system events
├── aic_event.c        # Event bus implementation (FreeRTOS queue)
├── aic_log.h          # Logging System - Levels, printf + IPC output
├── aic_log.c          # Logging implementation (FreeRTOS queue)
├── gpio.h             # GPIO API - LED, Button, PWM
├── gpio.c             # GPIO implementation
├── sensors.h          # Sensor API - ADC, IMU, CAPSENSE
├── sensors.c          # Sensor implementation (reads from shared memory)
├── tilt.h             # Tilt Analysis - Complementary Filter
├── tilt.c             # Tilt calculation (Roll, Pitch from IMU)
├── scope.h            # Oscilloscope API - Waveform, Audio, FFT
├── scope.c            # Signal processing implementation
├── ma_filter.h        # Moving Average Filter (header-only)
└── README.md          # This file

```

---

## Quick Start

### Basic Usage

```c
#include "aic-eec.h"      /* Common UI components */
#include "gpio.h"         /* GPIO control */
#include "sensors.h"      /* Sensor reading */
#include "tilt.h"         /* Tilt analysis */
#include "scope.h"        /* Signal processing */

void app_init(void)
{
    /* Initialize hardware */
    aic_gpio_init();
    aic_sensors_init();
    aic_tilt_init(NULL);  /* Use default config */

    /* Create footer on screen */
    aic_create_footer(lv_screen_active());
}

```

---

## Module 1: aic-eec.h - Common UI Components

### Description

Provides common UI elements for all examples including footer, header, and styling utilities.

### Functions

| Function | Description | Return |
|----------|-------------|--------|
| `aic_create_footer(parent)` | Create footer with copyright text | `lv_obj_t*` |
| `aic_create_footer_custom(parent, text, color)` | Create footer with custom text | `lv_obj_t*` |
| `aic_create_header(parent)` | Create header with course name | `lv_obj_t*` |
| `aic_create_header_custom(parent, text, color)` | Create header with custom text | `lv_obj_t*` |
| `aic_apply_theme(screen, bg_color)` | Apply AIC-EEC theme to screen | void |
| `aic_create_container(parent, w, h)` | Create styled container | `lv_obj_t*` |

### Configuration

```c
#define AIC_COPYRIGHT_TEXT   "(C) 2026 AIC-EEC.com and BiiL Centre, Burapha University"
#define AIC_COPYRIGHT_COLOR  0x666666
#define AIC_FOOTER_Y_OFFSET  (-5)

```

### Example

```c
/* Add footer to screen */
aic_create_footer(lv_screen_active());

/* Custom footer */
aic_create_footer_custom(screen, "My Custom Footer", 0x00FF00);

```

---

## Module 2: gpio.h - GPIO Control

### Description

Hardware abstraction for LEDs, Buttons, and PWM on PSoC Edge E84 kit.

### Hardware Mapping

| LED | GPIO Pin | Feature |
|-----|----------|---------|
| `AIC_LED_RED` | P19_2 | GPIO only (ON/OFF) |
| `AIC_LED_GREEN` | P19_3 | GPIO only (ON/OFF) |
| `AIC_LED_BLUE` | P16_5 | PWM capable (dimming) |

| Button | GPIO Pin | Description |
|--------|----------|-------------|
| `AIC_BTN_USER` | P8_3 (SW2) | User Button 1 |
| `AIC_BTN_USER2` | P8_7 (SW4) | User Button 2 |

### Functions

#### Initialization

| Function | Description |
|----------|-------------|
| `aic_gpio_init()` | Initialize all GPIO pins |
| `aic_gpio_deinit()` | Deinitialize GPIO |

#### LED Control

| Function | Description |
|----------|-------------|
| `aic_gpio_led_set(led, state)` | Set LED ON/OFF |
| `aic_gpio_led_toggle(led)` | Toggle LED state |
| `aic_gpio_led_get(led)` | Get current LED state |
| `aic_gpio_led_all_on()` | Turn all LEDs on |
| `aic_gpio_led_all_off()` | Turn all LEDs off |

#### PWM Control (Blue LED only)

| Function | Description |
|----------|-------------|
| `aic_gpio_pwm_init(led)` | Initialize PWM for LED |
| `aic_gpio_pwm_set_brightness(led, %)` | Set brightness 0-100% |
| `aic_gpio_pwm_get_brightness(led)` | Get current brightness |
| `aic_gpio_pwm_set_frequency(led, hz)` | Set PWM frequency |

#### Button Input

| Function | Description |
|----------|-------------|
| `aic_gpio_button_read(btn)` | Read button with debounce |
| `aic_gpio_button_read_raw(btn)` | Read raw button state |
| `aic_gpio_button_was_pressed(btn)` | Edge detection (clear on read) |
| `aic_gpio_button_set_callback(btn, cb)` | Register interrupt callback |

### Example

```c
/* Initialize GPIO */
aic_gpio_init();
aic_gpio_pwm_init(AIC_LED_BLUE);

/* Control LEDs */
aic_gpio_led_set(AIC_LED_RED, true);      /* LED on */
aic_gpio_led_toggle(AIC_LED_GREEN);        /* Toggle */
aic_gpio_pwm_set_brightness(AIC_LED_BLUE, 50);  /* 50% brightness */

/* Read button */
if (aic_gpio_button_read(AIC_BTN_USER)) {
    printf("Button pressed!\n");
}

```

---

## Module 3: sensors.h - Sensor Reading

### Description

Hardware abstraction for ADC (Potentiometer), IMU (BMI270), and CAPSENSE.

### ADC Channels

| Channel | Description |
|---------|-------------|
| `AIC_ADC_CH0` | Potentiometer |
| `AIC_ADC_CH1-3` | General purpose |
| `AIC_ADC_TEMP` | Internal temperature |

### Functions

#### Initialization

| Function | Description |
|----------|-------------|
| `aic_sensors_init()` | Initialize all sensors |
| `aic_sensors_deinit()` | Deinitialize |

#### ADC Functions

| Function | Return | Description |
|----------|--------|-------------|
| `aic_adc_read(channel)` | 0-4095 | Raw ADC value (12-bit) |
| `aic_adc_read_voltage(channel)` | 0.0-3.3V | Voltage reading |
| `aic_adc_read_percent(channel)` | 0-100% | Percentage |
| `aic_adc_read_temperature()` | °C | Internal temperature |

#### IMU Functions (BMI270)

| Function | Return | Description |
|----------|--------|-------------|
| `aic_imu_read_accel(&ax, &ay, &az)` | bool | Read accelerometer (m/s²) |
| `aic_imu_read_gyro(&gx, &gy, &gz)` | bool | Read gyroscope (rad/s) |
| `aic_imu_read_all(&data)` | bool | Read all IMU data |
| `aic_imu_get_orientation()` | enum | Get device orientation |
| `aic_imu_is_available()` | bool | Check if IMU ready |
| `aic_imu_calibrate()` | bool | Zero offset calibration |

#### CAPSENSE Functions (PSoC 4000T via I2C)

| Function | Return | Description |
|----------|--------|-------------|
| `aic_capsense_init()` | bool | Initialize I2C |
| `aic_capsense_read(&data)` | bool | Read buttons + slider |
| `aic_capsense_is_available()` | bool | Check if available |

### Example

```c
/* Initialize sensors */
aic_sensors_init();

/* Read ADC */
uint16_t raw = aic_adc_read(AIC_ADC_CH0);
float voltage = aic_adc_read_voltage(AIC_ADC_CH0);
uint8_t percent = aic_adc_read_percent(AIC_ADC_CH0);

/* Read IMU */
float ax, ay, az;
if (aic_imu_read_accel(&ax, &ay, &az)) {
    printf("Accel: X=%.2f Y=%.2f Z=%.2f m/s²\n", ax, ay, az);
}

/* Read CAPSENSE */
aic_capsense_data_t caps;
if (aic_capsense_read(&caps)) {
    printf("Slider: %d%%, BTN0: %d, BTN1: %d\n",
           caps.slider_pos, caps.btn0_pressed, caps.btn1_pressed);
}

```

---

## Module 4: tilt.h - Tilt Analysis (Complementary Filter)

### Description

Calculates Roll and Pitch angles by fusing Accelerometer and Gyroscope data using a Complementary Filter.

### Algorithm

```hs
angle = alpha * (angle + gyro * dt) + (1-alpha) * accel_angle

where:
  alpha = 0.98    → Trust gyro 98% (fast response)
  (1-alpha) = 0.02 → Trust accel 2% (drift correction)
  dt = 0.1s       → Sample period (100ms timer)

```

### Why Complementary Filter?

| Sensor | Advantage | Disadvantage |
|--------|-----------|--------------|
| **Accelerometer** | Measures gravity direction accurately | Noisy, affected by vibration |
| **Gyroscope** | Fast response, no noise | Drifts over time (integration error) |
| **Complementary** | Best of both worlds | Requires alpha tuning |

### Coordinate System

```sh
PSoC Edge E84 Board Orientation:
  - Roll:  Rotation around X-axis (tilt left/right)
  - Pitch: Rotation around Y-axis (tilt forward/backward)
  - Yaw:   Rotation around Z-axis (heading, needs magnetometer)

```

### Functions

| Function | Return | Description |
|----------|--------|-------------|
| `aic_tilt_init(config)` | bool | Initialize (NULL for defaults) |
| `aic_tilt_update(ax,ay,az,gx,gy,gz)` | bool | Update with raw data |
| `aic_tilt_update_from_imu()` | bool | Auto-read IMU and update |
| `aic_tilt_get_roll()` | float | Roll angle (-180 to +180°) |
| `aic_tilt_get_pitch()` | float | Pitch angle (-90 to +90°) |
| `aic_tilt_get_roll_percent()` | uint8_t | Roll as 0-100% |
| `aic_tilt_get_pitch_percent()` | uint8_t | Pitch as 0-100% |
| `aic_tilt_reset()` | void | Reset filter state |
| `aic_tilt_set_alpha(alpha)` | void | Adjust filter coefficient |

### Percentage Mapping

```ini
-90° → 0%
  0° → 50%
+90° → 100%

```

### Example

```c
/* Initialize */
aic_sensors_init();
aic_tilt_init(NULL);  /* Use defaults: alpha=0.98, dt=0.1s */

/* In timer callback (100ms) */
void timer_cb(lv_timer_t *t) {
    aic_tilt_update_from_imu();  /* Auto-read IMU */

    float roll = aic_tilt_get_roll();
    float pitch = aic_tilt_get_pitch();
    uint8_t roll_pct = aic_tilt_get_roll_percent();

    lv_arc_set_value(arc, roll_pct);
    lv_label_set_text_fmt(label, "Roll: %.1f°", roll);
}

```

---

## Module 5: scope.h - Oscilloscope & Signal Processing

### Description

Waveform generation, audio input/output, and FFT spectrum analysis.

### Waveform Types

| Type | Description |
|------|-------------|
| `AIC_WAVE_SQUARE` | Square wave (50% duty) |
| `AIC_WAVE_SINE` | Sine wave |
| `AIC_WAVE_TRIANGLE` | Triangle wave |
| `AIC_WAVE_SAWTOOTH` | Sawtooth wave |
| `AIC_WAVE_NOISE` | White noise |
| `AIC_WAVE_PULSE` | Adjustable duty cycle |

### Functions

#### Initialization

| Function | Description |
|----------|-------------|
| `aic_scope_init()` | Initialize scope subsystem |
| `aic_scope_is_ready()` | Check if initialized |

#### Waveform Generation

| Function | Description |
|----------|-------------|
| `aic_scope_generate_square(buf, n, freq, sr, amp)` | Generate square wave |
| `aic_scope_generate_sine(buf, n, freq, sr, amp)` | Generate sine wave |
| `aic_scope_generate_triangle(buf, n, freq, sr, amp)` | Generate triangle wave |
| `aic_scope_generate_sawtooth(buf, n, freq, sr, amp)` | Generate sawtooth |
| `aic_scope_generate_noise(buf, n, amp)` | Generate noise |
| `aic_scope_generate_wave(buf, n, config)` | Generate using config |

#### Audio Input (PDM Microphone)

| Function | Description |
|----------|-------------|
| `aic_audio_in_init(sample_rate)` | Initialize mic |
| `aic_audio_in_start()` | Start capture |
| `aic_audio_in_stop()` | Stop capture |
| `aic_audio_in_get_samples(buf, n)` | Get captured samples |
| `aic_audio_in_get_level()` | Get RMS level (0-100) |

#### Audio Output (I2S DAC)

| Function | Description |
|----------|-------------|
| `aic_audio_out_init(sample_rate)` | Initialize DAC |
| `aic_audio_out_start(buf, n, loop)` | Start playback |
| `aic_audio_out_stop()` | Stop playback |
| `aic_audio_out_set_volume(vol)` | Set volume (0-100) |

#### Signal Processing

| Function | Return | Description |
|----------|--------|-------------|
| `aic_signal_rms(buf, n)` | int16_t | Calculate RMS |
| `aic_signal_peak_to_peak(buf, n)` | int32_t | Peak-to-peak amplitude |
| `aic_signal_find_trigger(buf, n, level, rising)` | int32_t | Find trigger point |
| `aic_signal_frequency(buf, n, sr)` | uint32_t | Estimate frequency |
| `aic_signal_remove_dc(buf, n)` | void | Remove DC offset |

#### FFT (Spectrum Analyzer)

| Function | Description |
|----------|-------------|
| `aic_fft_init(size)` | Initialize FFT (64/128/256/512/1024) |
| `aic_fft_calculate(input, output)` | Calculate magnitude spectrum |
| `aic_fft_bin_frequency(bin, size, sr)` | Get frequency for bin |
| `aic_fft_dominant_frequency(spectrum, bins, sr)` | Find dominant freq |

### Example

```c
int16_t buffer[256];

/* Generate 1kHz sine wave at 48kHz sample rate */
aic_scope_generate_sine(buffer, 256, 1000, 48000, 32767);

/* Calculate signal stats */
int16_t rms = aic_signal_rms(buffer, 256);
int32_t p2p = aic_signal_peak_to_peak(buffer, 256);

/* FFT analysis */
uint16_t spectrum[128];
aic_fft_init(256);
aic_fft_calculate(buffer, spectrum);
uint32_t dominant = aic_fft_dominant_frequency(spectrum, 128, 48000);

```

---

## Module 6: ma_filter.h - Moving Average Filter

### Description

Header-only implementation of Moving Average Filter for smoothing sensor data.

### Configuration

```c
#define MA_FILTER_DEFAULT_SIZE  3    /* 150ms lag at 50ms update */
#define MA_FILTER_MAX_SIZE      20   /* Maximum window size */

```

### Functions

| Function | Description |
|----------|-------------|
| `ma_filter_init(&f, size)` | Initialize with window size |
| `ma_filter_init_default(&f)` | Initialize with default size (3) |
| `ma_filter_reset(&f)` | Clear all samples |
| `ma_filter_update(&f, value)` | Add sample, get filtered output |
| `ma_filter_get_average(&f)` | Get current average |
| `ma_filter_is_full(&f)` | Check if buffer full |

### 3-Axis Filter (for IMU)

| Function | Description |
|----------|-------------|
| `ma_filter_3axis_init(&f, size)` | Initialize X, Y, Z filters |
| `ma_filter_3axis_reset(&f)` | Reset all axes |
| `ma_filter_3axis_update(&f, x, y, z, &xo, &yo, &zo)` | Update all axes |

### Example

```c
/* Single axis filter */
ma_filter_t filter;
ma_filter_init(&filter, 5);  /* 5-sample window */

float filtered = ma_filter_update(&filter, raw_value);

/* 3-axis filter for IMU */
ma_filter_3axis_t imu_filter;
ma_filter_3axis_init(&imu_filter, 3);

float ax_f, ay_f, az_f;
ma_filter_3axis_update(&imu_filter, ax, ay, az, &ax_f, &ay_f, &az_f);

```

---

## Module 7: aic_layout.h - UI Layout Helpers

### Description

Flexbox-style layout helpers for LVGL. Reduces boilerplate code and provides consistent spacing, alignment, and common UI patterns.

### Color Palette

| Macro | Hex | Usage |
|-------|-----|-------|
| `AIC_COLOR_BG_DARK` | `#16213e` | Screen background |
| `AIC_COLOR_BG_CARD` | `#1f4068` | Card background |
| `AIC_COLOR_PRIMARY` | `#00d4ff` | Primary accent |
| `AIC_COLOR_SECONDARY` | `#ff6b6b` | Secondary accent |
| `AIC_COLOR_SUCCESS` | `#4ade80` | Success state |
| `AIC_COLOR_WARNING` | `#fbbf24` | Warning state |
| `AIC_COLOR_ERROR` | `#ef4444` | Error state |
| `AIC_COLOR_TEXT` | `#ffffff` | Normal text |
| `AIC_COLOR_TEXT_DIM` | `#94a3b8` | Dimmed text |

### Layout Containers

| Function | Description |
|----------|-------------|
| `aic_row_create(parent)` | Horizontal flex row |
| `aic_col_create(parent)` | Vertical flex column |
| `aic_spacer_create(parent)` | Flexible spacer (fills space) |
| `aic_center_create(parent)` | Centered container |

### Common Components

| Function | Description |
|----------|-------------|
| `aic_card_create(parent, title)` | Card with title and content area |
| `aic_value_display_create(parent, label)` | Label + value pair |
| `aic_status_indicator_create(parent, label, state)` | LED status indicator |
| `aic_icon_button_create(parent, icon, text)` | Button with icon |
| `aic_section_create(parent, title)` | Section with title |
| `aic_divider_create(parent)` | Horizontal divider |

### Data Display

| Function | Description |
|----------|-------------|
| `aic_gauge_create(parent, label, min, max, initial)` | Arc gauge |
| `aic_progress_bar_create(parent, label)` | Progress bar |
| `aic_xyz_display_create(parent, title, labels)` | XYZ display for IMU |

### Screen Helpers

| Function | Description |
|----------|-------------|
| `aic_apply_dark_theme(scr)` | Apply dark theme (NULL = active screen) |
| `aic_create_footer(parent)` | Default copyright footer |
| `aic_create_footer_custom(parent, text, color)` | Custom footer |
| `aic_create_header(parent, title)` | Header with title |

### Layout Properties

| Function | Description |
|----------|-------------|
| `aic_flex_grow(obj, grow)` | Set flex grow factor |
| `aic_pad(obj, pad)` | Set padding (all sides) |
| `aic_pad_all(obj, t, r, b, l)` | Set padding (individual) |
| `aic_gap(obj, gap)` | Set gap between children |
| `aic_full_width(obj)` | Full width of parent |
| `aic_full_size(obj)` | Full size of parent |

### Example

```c
#include "aic_layout.h"

aic_apply_dark_theme(NULL);
lv_obj_t *col = aic_col_create(lv_screen_active());
aic_create_header(col, "Sensor Dashboard");

lv_obj_t *row = aic_row_create(col);
lv_obj_t *card1 = aic_card_create(row, "Temperature");
lv_obj_t *val = aic_value_display_create(card1, "Temp:");
lv_label_set_text(val, "25.3 C");

lv_obj_t *bar = aic_progress_bar_create(col, "CPU Load");
lv_bar_set_value(bar, 65, LV_ANIM_ON);

aic_create_footer(col);
```

---

## Module 8: aic_event.h - Event Bus

### Description

Publish-subscribe event system for decoupling sensor updates from UI. Uses FreeRTOS queue for thread-safe event delivery.

### Event Types

| Group | Events |
|-------|--------|
| **Sensor** (1-20) | `AIC_EVENT_IMU_UPDATE`, `ADC_UPDATE`, `TEMP_UPDATE`, `HUMIDITY_UPDATE`, `PRESSURE_UPDATE` |
| **Input** (21-40) | `AIC_EVENT_BUTTON_PRESS`, `BUTTON_RELEASE`, `BUTTON_LONG_PRESS`, `CAPSENSE_UPDATE` |
| **System** (41-60) | `AIC_EVENT_IPC_CONNECTED`, `IPC_DISCONNECTED`, `IPC_MESSAGE`, `ERROR`, `WARNING`, `TIMER` |
| **App** (61-80) | `AIC_EVENT_MODE_CHANGE`, `SETTING_CHANGE`, `UI_UPDATE`, `DATA_READY` |
| **Custom** (81-100) | `AIC_EVENT_CUSTOM_1` to `CUSTOM_5` |

### Functions

| Function | Description |
|----------|-------------|
| `aic_event_init()` | Initialize event bus |
| `aic_event_subscribe(event, cb, user_data)` | Subscribe to event |
| `aic_event_unsubscribe(event, cb)` | Unsubscribe |
| `aic_event_publish(event, data)` | Publish (queued, non-blocking) |
| `aic_event_publish_immediate(event, data)` | Publish (blocking) |
| `aic_event_process()` | Process queue (non-FreeRTOS) |

### Publish Helpers

| Function | Description |
|----------|-------------|
| `aic_event_publish_imu(ax,ay,az,gx,gy,gz)` | Publish IMU data |
| `aic_event_publish_adc(ch, raw, mv)` | Publish ADC data |
| `aic_event_publish_button(id, pressed)` | Publish button event |
| `aic_event_publish_temp(centi)` | Publish temperature |

### Example

```c
#include "aic_event.h"

void on_button(aic_event_t ev, const aic_event_data_t *d, void *u) {
    printf("Button %d: %s\n", d->button.button_id,
           d->button.pressed ? "PRESS" : "RELEASE");
}

aic_event_init();
aic_event_subscribe(AIC_EVENT_BUTTON_PRESS, on_button, NULL);
aic_event_publish_button(AIC_BTN_USER, true);
aic_event_process();  /* or use FreeRTOS task */
```

### Configuration

| Define | Default | Description |
|--------|---------|-------------|
| `AIC_EVENT_MAX_SUBSCRIBERS` | 8 | Max subscribers per event |
| `AIC_EVENT_QUEUE_SIZE` | 16 | Event queue depth |

---

## Module 9: aic_log.h - Logging System

### Description

Thread-safe, non-blocking logging system with queue-based output. Supports multiple output targets: printf (UART), IPC to CM33, and LVGL label.

### Log Levels

| Level | Macro | Description |
|-------|-------|-------------|
| `AIC_LOG_ERROR` | `AIC_LOGE()` | Error messages |
| `AIC_LOG_WARN` | `AIC_LOGW()` | Warnings |
| `AIC_LOG_INFO` | `AIC_LOGI()` | Informational |
| `AIC_LOG_DEBUG` | `AIC_LOGD()` | Debug details |
| `AIC_LOG_VERBOSE` | `AIC_LOGV()` | All details |

### Output Targets

| Target | Flag | Description |
|--------|------|-------------|
| printf | `AIC_LOG_TARGET_PRINTF` | Output to UART console |
| IPC | `AIC_LOG_TARGET_IPC` | Forward to CM33 via IPC |
| LVGL | `AIC_LOG_TARGET_LVGL` | Display on LVGL label |

### Functions

| Function | Description |
|----------|-------------|
| `aic_log_init()` | Initialize logging |
| `aic_log_set_level(level)` | Set minimum log level |
| `aic_log_set_targets(mask)` | Set output targets |
| `aic_log(level, fmt, ...)` | Log message |
| `aic_log_tag(level, tag, fmt, ...)` | Log with module tag |
| `aic_log_flush()` | Flush queue (blocking) |
| `aic_log_set_lvgl_label(label, max_lines)` | Set LVGL output label |

### Tagged Logging

```c
/* In .c file */
AIC_LOG_TAG("SENSOR");   /* Define module tag */

LOGI("Init complete");           /* → [I][SENSOR] Init complete */
LOGE("Read failed: %d", err);   /* → [E][SENSOR] Read failed: -1 */
LOGD("ADC: %d mV", voltage);    /* → [D][SENSOR] ADC: 1650 mV */
```

### Example

```c
#include "aic_log.h"

aic_log_init();
aic_log_set_level(AIC_LOG_DEBUG);
aic_log_set_targets(AIC_LOG_TARGET_PRINTF | AIC_LOG_TARGET_IPC);

AIC_LOGI("System started");
AIC_LOGD("ADC CH0 = %d", raw_value);
AIC_LOGE("Sensor timeout!");

aic_log_flush();  /* Wait for all messages to be output */
```

### Configuration

| Define | Default | Description |
|--------|---------|-------------|
| `AIC_LOG_QUEUE_SIZE` | 16 | Queue depth |
| `AIC_LOG_MSG_MAX_LEN` | 128 | Max message length |

---

## Simulation Mode

For UI development without hardware, enable simulation mode:

```c
/* Enable simulation */
aic_sensors_set_simulation(true);

/* Set simulated values */
aic_adc_set_simulated(AIC_ADC_CH0, 2048);      /* Set ADC value */
aic_imu_set_simulated_accel(0.0f, 0.0f, 9.81f); /* Set accel */
aic_imu_set_simulated_gyro(0.0f, 0.0f, 0.0f);   /* Set gyro */

```

---

## Architecture: CM33-CM55 IPC

PSoC Edge E84 has dual-core architecture:

- **CM33**: Handles I2C communication (IMU, CAPSENSE)
- **CM55**: Runs LVGL UI and reads sensor data from shared memory

```ini
┌─────────────┐     Shared Memory     ┌─────────────┐
│    CM33     │ ──────────────────────│    CM55     │
│             │     0x261C0000        │             │
│  I2C Master │ → imu_shared_t        │  LVGL UI    │
│  BMI270 IMU │ → capsense_shared_t   │  aic-eec    │
│  CAPSENSE   │                       │  Examples   │
└─────────────┘                       └─────────────┘

```

The `aic-eec` API automatically reads from shared memory on CM55.

---

## Used in Examples

### Part 1: LVGL Basics + GPIO (11 examples)

- Ex1-5: UI widgets only (`aic-eec.h`, `aic_layout.h`)
- Ex6-11: Uses `gpio.h` for LED/Button/PWM control

---

## License

This SDK is developed for educational purposes.
Department of Electrical Engineering, Faculty of Engineering, Burapha University.

---

## References

- [LVGL v9.2 Documentation](https://docs.lvgl.io/9.2/)
- [PSoC Edge E84 Kit Guide](https://www.infineon.com/cms/en/product/evaluation-boards/)
- [BMI270 Datasheet](https://www.bosch-sensortec.com/products/motion-sensors/imus/bmi270/)

---

> **(C) 2026 AIC-EEC.com**
> **Embedded Systems and IoT Development, Department of Electrical Engineering, Faculty of Engineering, Burapha University**
> **Assoc. Prof. Wiroon Sriborrirux** (wiroon@eng.buu.ac.th)
