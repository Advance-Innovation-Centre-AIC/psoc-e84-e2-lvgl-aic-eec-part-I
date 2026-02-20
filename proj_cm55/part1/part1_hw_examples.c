/*******************************************************************************
 * File Name:   part1_hw_examples.c
 *
 * Description: Part 1 Part II Examples - Hardware Integration
 *              Embedded C for IoT Course
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * IMPORTANT:   Part II examples REUSE the UI code from Part I and only add
 *              hardware API calls. This ensures consistency between
 *              simulated and real hardware examples.
 *
 * Part II Examples (Hardware Integration):
 *   6.  Hardware LED Control      -> Based on Ex3 (LED Widget Control)
 *   7.  Hardware Button Status    -> New example (physical button input)
 *   8.  Hardware ADC Display      -> Similar to Week4 Ex1 (ADC visualization)
 *   9.  Hardware GPIO Dashboard   -> Based on Ex5 (GPIO Dashboard)
 *  10.  CAPSENSE UI Mockup        -> New UI design for CAPSENSE
 *  11.  CAPSENSE Hardware         -> Shared Memory IPC (CM33->CM55)
 *
 * =============================================================================
 * LVGL 9.2 Widget References
 * =============================================================================
 *   LED Widget:    https://docs.lvgl.io/9.2/widgets/led.html
 *   Button:        https://docs.lvgl.io/9.2/widgets/button.html
 *   Slider:        https://docs.lvgl.io/9.2/widgets/slider.html
 *   Bar:           https://docs.lvgl.io/9.2/widgets/bar.html
 *   Switch:        https://docs.lvgl.io/9.2/widgets/switch.html
 *   Label:         https://docs.lvgl.io/9.2/widgets/label.html
 *   Timer:         https://docs.lvgl.io/9.2/others/timer.html
 *   Styles:        https://docs.lvgl.io/9.2/overview/style.html
 *
 * Dependencies:
 *   - aic-eec/gpio.h   (LED, Button, PWM control)
 *   - aic-eec/sensors.h (ADC reading)
 *
 * =============================================================================
 * SCREEN & COORDINATE SYSTEM (PSoC Edge E84 Eval Kit)
 * =============================================================================
 *       X=0           X=240          X=479
 *   Y=0  +-----------------------------+
 *        | (0,0)                       |
 *        |         X+ -------->        |
 *   Y=160|         | Y+    * CENTER    |
 *        |         v     (240,160)     |
 *   Y=319+-----------------------------+
 *
 *   Physical Display: 480 x 800 pixels (Vertical/Portrait)
 *   Usable Area:      480 x 320 pixels (for this course)
 *
 * =============================================================================
 * COMMON ALIGNMENT PATTERNS IN PART II (reusing Part I layouts)
 * =============================================================================
 *
 *   Ex6: LED Control (Based on Ex3)
 *   +------------------------------------------+
 *   |           "HW LED Control" (title)       | TOP_MID, y=20
 *   |                                          |
 *   |              [LED Widget]                | CENTER, y=-70
 *   |           "Brightness: 150"              | CENTER, y=0
 *   |          [ON]     [OFF]                  | CENTER, x=±60, y=50
 *   |        ========= Slider =========        | CENTER, y=110
 *   |              "59%"                       | align_to: OUT_BOTTOM_MID
 *   |    "[Part II] Using aic_gpio_*"          | BOTTOM_MID, y=-30
 *   +------------------------------------------+
 *
 *   Ex7: Button Status (2 Buttons Side-by-Side)
 *   +------------------------------------------+
 *   |         "HW Button Status" (title)       | TOP_MID, y=20
 *   |                                          |
 *   |    [LED1]        [LED2]                  | CENTER, x=-90 / x=90
 *   |   "USER Btn1"   "USER Btn2"              | align_to: OUT_BOTTOM_MID
 *   |   "Released"    "Released"               | align_to: OUT_BOTTOM_MID
 *   |                                          |
 *   |    "[Part II] Press SW2 or SW4"          | BOTTOM_MID, y=-45
 *   +------------------------------------------+
 *
 *   Ex9: GPIO Dashboard (3 LEDs + 2 Panels)
 *   +------------------------------------------+
 *   |         "HW GPIO Dashboard"              | TOP_MID, y=8
 *   |  [Red LED]    Red    [Switch]            | TOP_MID, y=65
 *   |  [Green LED]  Green  [Switch]            | TOP_MID, y=115
 *   |  [Blue LED]   Blue   (POT ctrl)          | TOP_MID, y=165
 *   |   [All ON]   [All OFF]                   | BOTTOM_MID, y=-160
 *   | +---------+           +---------+        |
 *   | | BTN2    |           |  ADC    |        | BOTTOM_LEFT/RIGHT
 *   | | Status  |           | POT->LED|        | y=-25
 *   | +---------+           +---------+        |
 *   +------------------------------------------+
 *
 *   Ex10/11: CAPSENSE (Slider on TOP, Buttons on BOTTOM)
 *   +------------------------------------------+
 *   |         "CAPSENSE UI Mockup"             | TOP_MID, y=8
 *   |         "Mode: Auto Demo"                | TOP_MID, y=30
 *   | +--------------------------------------+ | TOP_MID, y=85
 *   | | SLIDER (CSS1)              0%   [o] | | Slider Panel: 420x80
 *   | +--------------------------------------+ |
 *   |                                          |
 *   |   +--------+          +--------+         | BOTTOM_MID, x=-110/110
 *   |   |  BTN0  |          |  BTN1  |         | y=-85, size 140x150
 *   |   | (CSB1) |          | (CSB2) |         |
 *   |   | [LED]  |          | [LED]  |         |
 *   |   | Ready  |          | Ready  |         |
 *   |   +--------+          +--------+         |
 *   |      "Touch to switch to Manual"         | BOTTOM_MID, y=-25
 *   +------------------------------------------+
 *
 * =============================================================================
 * HARDWARE API USED IN PART II
 * =============================================================================
 *   GPIO:
 *     aic_gpio_init()                    - Initialize GPIO subsystem
 *     aic_gpio_led_set(led, state)       - Set LED ON/OFF
 *     aic_gpio_pwm_init(led)             - Initialize PWM for LED
 *     aic_gpio_pwm_set_brightness(led,%) - Set LED brightness 0-100%
 *     aic_gpio_button_read(btn)          - Read button state
 *
 *   Sensors:
 *     aic_sensors_init()                 - Initialize sensor subsystem
 *     aic_adc_read(channel)              - Read ADC raw value (0-4095)
 *     aic_adc_read_percent(channel)      - Read ADC as percentage
 *
 *   CAPSENSE (Shared Memory IPC):
 *     CAPSENSE_SHARED_IS_VALID()         - Check if shared memory valid
 *     CAPSENSE_SHARED_GET_COUNT()        - Get update count
 *     capsense_shared_read(...)          - Read button/slider data
 *
 ******************************************************************************/

#include "part1_examples.h"
#include "../aic-eec/aic-eec.h"
#include "../aic-eec/gpio.h"
#include "../aic-eec/sensors.h"
#include "../lv_port_indev.h"    /* For touch disable/enable (I2C bus sharing) */
#include <stdio.h>

/*******************************************************************************
 * Example 6: Hardware LED Control
 *
 * Based on: part1_ex3_led_control() - Same UI, but controls REAL LEDs
 *
 * Changes from Part I:
 *   1. Add aic_gpio_init() at start
 *   2. ON button also calls aic_gpio_led_set(AIC_LED_GREEN, true)
 *   3. OFF button also calls aic_gpio_led_set(AIC_LED_GREEN, false)
 *   4. Slider brightness also updates real LED via PWM
 *
 ******************************************************************************/

/* Static variables - same as Ex3 */
static lv_obj_t * ex6_led;
static lv_obj_t * ex6_brightness_label;
static lv_obj_t * ex6_slider_label;

static void ex6_on_btn_cb(lv_event_t * e)
{
    (void)e;
    /* Update virtual LED */
    lv_led_on(ex6_led);
    lv_label_set_text(ex6_brightness_label, "Brightness: 255 (ON)");

    /* Control REAL hardware LED */
    aic_gpio_led_set(AIC_LED_GREEN, true);
    printf("[HW] LED GREEN: ON\r\n");
}

static void ex6_off_btn_cb(lv_event_t * e)
{
    (void)e;
    /* Update virtual LED */
    lv_led_off(ex6_led);
    lv_label_set_text(ex6_brightness_label, "Brightness: 0 (OFF)");

    /* Control REAL hardware LED */
    aic_gpio_led_set(AIC_LED_GREEN, false);
    printf("[HW] LED GREEN: OFF\r\n");
}

static void ex6_slider_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    /* Update percentage label (0-255 -> 0-100%) */
    int percent = (value * 100) / 255;
    lv_label_set_text_fmt(ex6_slider_label, "%d%%", percent);

    /* Update virtual LED brightness */
    lv_led_set_brightness(ex6_led, (uint8_t)value);
    lv_label_set_text_fmt(ex6_brightness_label, "Brightness: %d", (int)value);

    /* Control REAL Blue LED brightness via PWM
     * Note: Only Blue LED (P16_5) supports PWM dimming on this board
     * Green/Red LEDs are GPIO only (ON/OFF)
     */
    aic_gpio_pwm_set_brightness(AIC_LED_BLUE, (uint8_t)percent);
    printf("[HW] LED BLUE PWM: %d%%\r\n", percent);
}

void part1_ex6_hw_led_control(void)
{
    /* ===== HARDWARE INITIALIZATION (Part II addition) ===== */
    aic_gpio_init();
    aic_gpio_pwm_init(AIC_LED_BLUE);  /* Initialize PWM for Blue LED dimming */

    /* ===== UI CODE FROM Ex3 (part1_ex3_led_control) ===== */

    /* Background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x0f0f23),
                              LV_PART_MAIN);

    /* Title - Updated for Part II */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 Ex6: HW LED Control (Based on Ex3)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Create LED widget - same as Ex3 */
    ex6_led = lv_led_create(lv_screen_active());
    lv_obj_set_size(ex6_led, 80, 80);
    lv_obj_align(ex6_led, LV_ALIGN_CENTER, 0, -70);
    lv_led_set_color(ex6_led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_brightness(ex6_led, 150);

    /* Brightness label - same as Ex3 */
    ex6_brightness_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ex6_brightness_label, "Brightness: 150");
    lv_obj_set_style_text_color(ex6_brightness_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ex6_brightness_label, LV_ALIGN_CENTER, 0, 0);

    /* ON Button - same pattern as Ex3 */
    lv_obj_t * btn_on = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_on, ex6_on_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, -60, 50);
    lv_obj_set_style_bg_color(btn_on, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_pad_hor(btn_on, 30, 0);
    lv_obj_set_style_pad_ver(btn_on, 15, 0);

    lv_obj_t * label_on = lv_label_create(btn_on);
    lv_label_set_text(label_on, "ON");
    lv_obj_center(label_on);

    /* OFF Button - same pattern as Ex3 */
    lv_obj_t * btn_off = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_off, ex6_off_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 60, 50);
    lv_obj_set_style_bg_color(btn_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_pad_hor(btn_off, 30, 0);
    lv_obj_set_style_pad_ver(btn_off, 15, 0);

    lv_obj_t * label_off = lv_label_create(btn_off);
    lv_label_set_text(label_off, "OFF");
    lv_obj_center(label_off);

    /* Brightness Slider - same as Ex3 */
    lv_obj_t * slider = lv_slider_create(lv_screen_active());
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 110);
    lv_slider_set_range(slider, 0, 255);
    lv_slider_set_value(slider, 150, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, ex6_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Slider percentage label - same as Ex3 */
    ex6_slider_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ex6_slider_label, "59%");
    lv_obj_set_style_text_color(ex6_slider_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(ex6_slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /* Description - Updated for Part II */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "[Part II] Using aic_gpio_led_set() + aic_gpio_pwm_set_brightness()");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -30);

    /* Footer */
    aic_create_footer(lv_screen_active());

    printf("[Week3] Ex6: Hardware LED Control started (based on Ex3 UI)\r\n");
}

/*******************************************************************************
 * Example 7: Hardware Button Status (2 Buttons)
 *
 * Shows status of 2 hardware buttons on the PSoC Edge E84 board:
 *   - USER Button 1 (SW2) - P8_3 - displayed on LEFT side
 *   - USER Button 2 (SW4) - P8_7 - displayed on RIGHT side
 *
 * UI Features:
 *   - Two LED widgets (Cyan/Orange) showing button state visually
 *   - Status labels showing "PRESSED" (green) or "Released" (red)
 *   - Timer polls button states every 50ms with debouncing
 *
 * Hardware API used:
 *   - aic_gpio_button_read(AIC_BTN_USER)   - Read Button 1
 *   - aic_gpio_button_read(AIC_BTN_USER2)  - Read Button 2
 *
 * Note: This is a NEW example - no corresponding Part I example
 *       (Part I doesn't have physical button input)
 *
 ******************************************************************************/

/* UI elements for 2 buttons */
static lv_obj_t * ex7_led1;           /* LED indicator for Button 1 */
static lv_obj_t * ex7_led2;           /* LED indicator for Button 2 */
static lv_obj_t * ex7_status_label1;  /* Status for Button 1 */
static lv_obj_t * ex7_status_label2;  /* Status for Button 2 */
static lv_timer_t * ex7_timer;

static void ex7_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* Read REAL hardware buttons */
    bool pressed1 = aic_gpio_button_read(AIC_BTN_USER);
    bool pressed2 = aic_gpio_button_read(AIC_BTN_USER2);

    /* Update Button 1 UI */
    if(pressed1) {
        lv_label_set_text(ex7_status_label1, "PRESSED");
        lv_obj_set_style_text_color(ex7_status_label1, lv_color_hex(0x00FF00), 0);
        lv_led_on(ex7_led1);
    } else {
        lv_label_set_text(ex7_status_label1, "Released");
        lv_obj_set_style_text_color(ex7_status_label1, lv_color_hex(0xFF6666), 0);
        lv_led_off(ex7_led1);
    }

    /* Update Button 2 UI */
    if(pressed2) {
        lv_label_set_text(ex7_status_label2, "PRESSED");
        lv_obj_set_style_text_color(ex7_status_label2, lv_color_hex(0x00FF00), 0);
        lv_led_on(ex7_led2);
    } else {
        lv_label_set_text(ex7_status_label2, "Released");
        lv_obj_set_style_text_color(ex7_status_label2, lv_color_hex(0xFF6666), 0);
        lv_led_off(ex7_led2);
    }
}

void part1_ex7_hw_button_status(void)
{
    /* ===== HARDWARE INITIALIZATION ===== */
    aic_gpio_init();

    /* ===== UI with 2 buttons side by side ===== */

    /* Background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x16213e),
                              LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 Ex7: Hardware Button Status");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* ===== Button 1 (LEFT) - SW2 ===== */
    /* LED indicator 1 */
    ex7_led1 = lv_led_create(lv_screen_active());
    lv_obj_set_size(ex7_led1, 70, 70);
    lv_obj_align(ex7_led1, LV_ALIGN_CENTER, -90, -30);
    lv_led_set_color(ex7_led1, lv_palette_main(LV_PALETTE_CYAN));
    lv_led_off(ex7_led1);

    /* Label for Button 1 */
    lv_obj_t * btn1_label = lv_label_create(lv_screen_active());
    lv_label_set_text(btn1_label, "USER Button 1\n(SW2)");
    lv_obj_set_style_text_color(btn1_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(btn1_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(btn1_label, ex7_led1, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Status label 1 */
    ex7_status_label1 = lv_label_create(lv_screen_active());
    lv_label_set_text(ex7_status_label1, "Released");
    lv_obj_set_style_text_color(ex7_status_label1, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_text_font(ex7_status_label1, &lv_font_montserrat_16, 0);
    lv_obj_align_to(ex7_status_label1, btn1_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /* ===== Button 2 (RIGHT) - SW4 ===== */
    /* LED indicator 2 */
    ex7_led2 = lv_led_create(lv_screen_active());
    lv_obj_set_size(ex7_led2, 70, 70);
    lv_obj_align(ex7_led2, LV_ALIGN_CENTER, 90, -30);
    lv_led_set_color(ex7_led2, lv_palette_main(LV_PALETTE_ORANGE));
    lv_led_off(ex7_led2);

    /* Label for Button 2 */
    lv_obj_t * btn2_label = lv_label_create(lv_screen_active());
    lv_label_set_text(btn2_label, "USER Button 2\n(SW4)");
    lv_obj_set_style_text_color(btn2_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(btn2_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(btn2_label, ex7_led2, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Status label 2 */
    ex7_status_label2 = lv_label_create(lv_screen_active());
    lv_label_set_text(ex7_status_label2, "Released");
    lv_obj_set_style_text_color(ex7_status_label2, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_text_font(ex7_status_label2, &lv_font_montserrat_16, 0);
    lv_obj_align_to(ex7_status_label2, btn2_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /* Description */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "[Part II] Press SW2 or SW4 on the board");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -45);

    /* Footer */
    aic_create_footer(lv_screen_active());

    /* Timer to poll button states */
    ex7_timer = lv_timer_create(ex7_timer_cb, 50, NULL);

    printf("[Week3] Ex7: Hardware Button Status (2 buttons) started\r\n");
}

/*******************************************************************************
 * Example 8: Hardware ADC Display
 *
 * Similar to Part 2 Ex1 (Slider & Bar for ADC) but with real hardware
 *
 ******************************************************************************/

static lv_obj_t * ex8_slider;
static lv_obj_t * ex8_bar;
static lv_obj_t * ex8_raw_label;
static lv_obj_t * ex8_volt_label;
static lv_obj_t * ex8_pct_label;
static lv_timer_t * ex8_timer;

#define ADC_MAX 4095
#define ADC_VREF_MV 3300

static void ex8_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* Read REAL ADC value */
    uint16_t adc_value = aic_adc_read(AIC_ADC_CH0);

    /* Update slider */
    lv_slider_set_value(ex8_slider, adc_value, LV_ANIM_ON);

    /* Update bar */
    int32_t pct = (adc_value * 100) / ADC_MAX;
    lv_bar_set_value(ex8_bar, pct, LV_ANIM_ON);

    /* Update labels */
    lv_label_set_text_fmt(ex8_raw_label, "Raw: %d", (int)adc_value);
    lv_label_set_text_fmt(ex8_pct_label, "%d%%", (int)pct);

    float voltage = ((float)adc_value / ADC_MAX) * (ADC_VREF_MV / 1000.0f);
    lv_label_set_text_fmt(ex8_volt_label, "Voltage: %.3f V", (double)voltage);
}

void part1_ex8_hw_adc_display(void)
{
    /* ===== HARDWARE INITIALIZATION ===== */
    aic_sensors_init();

    /* ===== UI - Balanced Layout with Color Coding ===== */
    lv_obj_t * scr = lv_screen_active();

    /* Background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Part 1 Ex8: Hardware ADC Display");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* ===== ROW 1: ADC Raw Value (GREEN theme) - CENTERED ===== */
    lv_obj_t * slider_label = lv_label_create(scr);
    lv_label_set_text(slider_label, "ADC Raw Value (0-4095)");
    lv_obj_set_style_text_color(slider_label, lv_color_hex(0x00FF00), 0);  /* Green */
    lv_obj_align(slider_label, LV_ALIGN_CENTER, -55, -70);

    /* Slider - centered */
    ex8_slider = lv_slider_create(scr);
    lv_obj_set_width(ex8_slider, 200);
    lv_obj_align(ex8_slider, LV_ALIGN_CENTER, -55, -40);
    lv_slider_set_range(ex8_slider, 0, ADC_MAX);
    lv_slider_set_value(ex8_slider, 2048, LV_ANIM_OFF);
    lv_obj_remove_flag(ex8_slider, LV_OBJ_FLAG_CLICKABLE);
    /* Green slider style */
    lv_obj_set_style_bg_color(ex8_slider, lv_color_hex(0x004400), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ex8_slider, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ex8_slider, lv_color_hex(0x00CC00), LV_PART_KNOB);

    /* Raw value label - after slider */
    ex8_raw_label = lv_label_create(scr);
    lv_label_set_text(ex8_raw_label, "Raw: 2048");
    lv_obj_set_style_text_color(ex8_raw_label, lv_color_hex(0x00FF00), 0);  /* Green */
    lv_obj_align_to(ex8_raw_label, ex8_slider, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    /* ===== ROW 2: Percentage (CYAN theme) - CENTERED ===== */
    lv_obj_t * bar_label = lv_label_create(scr);
    lv_label_set_text(bar_label, "Percentage");
    lv_obj_set_style_text_color(bar_label, lv_color_hex(0x00FFFF), 0);  /* Cyan */
    lv_obj_align(bar_label, LV_ALIGN_CENTER, -55, 0);

    ex8_bar = lv_bar_create(scr);
    lv_obj_set_size(ex8_bar, 200, 18);
    lv_obj_align(ex8_bar, LV_ALIGN_CENTER, -55, 25);
    lv_bar_set_range(ex8_bar, 0, 100);
    lv_bar_set_value(ex8_bar, 50, LV_ANIM_OFF);
    /* Cyan bar style */
    lv_obj_set_style_bg_color(ex8_bar, lv_color_hex(0x003344), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ex8_bar, lv_color_hex(0x00FFFF), LV_PART_INDICATOR);

    /* Percent label - after bar */
    ex8_pct_label = lv_label_create(scr);
    lv_label_set_text(ex8_pct_label, "50%");
    lv_obj_set_style_text_color(ex8_pct_label, lv_color_hex(0x00FFFF), 0);  /* Cyan */
    lv_obj_align_to(ex8_pct_label, ex8_bar, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    /* ===== ROW 3: Voltage (YELLOW theme, prominent) ===== */
    ex8_volt_label = lv_label_create(scr);
    lv_label_set_text(ex8_volt_label, "Voltage: 1.650 V");
    lv_obj_set_style_text_color(ex8_volt_label, lv_color_hex(0xFFFF00), 0);  /* Yellow */
    lv_obj_set_style_text_font(ex8_volt_label, &lv_font_montserrat_24, 0);
    lv_obj_align(ex8_volt_label, LV_ALIGN_CENTER, 0, 80);

    /* ===== Description ===== */
    lv_obj_t * desc = lv_label_create(scr);
    lv_label_set_text(desc, "[Part II] Turn the potentiometer on the board");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -30);

    /* Footer */
    aic_create_footer(scr);

    /* Timer for auto-update with REAL ADC data */
    ex8_timer = lv_timer_create(ex8_timer_cb, 100, NULL);

    printf("[Week3] Ex8: Hardware ADC Display started\r\n");
}

/*******************************************************************************
 * Example 9: Hardware GPIO Dashboard
 *
 * Hardware on PSoC Edge E84:
 *   - LED1 (Red)   - P19_2 - GPIO only (Toggle ON/OFF)
 *   - LED2 (Green) - P19_3 - GPIO only (Toggle ON/OFF)
 *   - LED3 (Blue)  - P16_5 - PWM (Controlled by Potentiometer)
 *   - USER Button  - P8_3 (SW2)
 *   - Potentiometer - ADC CH0 -> Controls Blue LED brightness
 *
 * Layout: LEDs arranged VERTICALLY to match board layout
 *
 ******************************************************************************/

#define EX9_NUM_TOGGLE_LEDS 2  /* Red and Green with toggle switches */

typedef struct {
    lv_obj_t *led;
    lv_obj_t *sw;
    lv_obj_t *label;
    const char *name;
    bool state;
} ex9_gpio_item_t;

static ex9_gpio_item_t ex9_gpios[EX9_NUM_TOGGLE_LEDS];
static lv_obj_t * ex9_blue_led;  /* Blue LED controlled by POT */
static lv_timer_t * ex9_timer;

/* Button/ADC UI elements */
static lv_obj_t * ex9_btn_status_led;
static lv_obj_t * ex9_btn_status_label;
static lv_obj_t * ex9_adc_bar;
static lv_obj_t * ex9_adc_label;

static void ex9_switch_cb(lv_event_t * e)
{
    ex9_gpio_item_t * gpio = (ex9_gpio_item_t *)lv_event_get_user_data(e);
    lv_obj_t * sw = lv_event_get_target(e);
    int idx = (int)(gpio - ex9_gpios);

    gpio->state = lv_obj_has_state(sw, LV_STATE_CHECKED);

    /* Update virtual LED */
    if(gpio->state) {
        lv_led_on(gpio->led);
    } else {
        lv_led_off(gpio->led);
    }

    /* Control REAL hardware LED (Red or Green only) */
    aic_gpio_led_set((aic_led_t)idx, gpio->state);
    printf("[HW] %s: %s\r\n", gpio->name, gpio->state ? "ON" : "OFF");
}

/* All ON button callback */
static void ex9_all_on_cb(lv_event_t * e)
{
    (void)e;
    /* Turn on Red and Green LEDs */
    for(int i = 0; i < EX9_NUM_TOGGLE_LEDS; i++) {
        ex9_gpios[i].state = true;
        lv_led_on(ex9_gpios[i].led);
        lv_obj_add_state(ex9_gpios[i].sw, LV_STATE_CHECKED);
        aic_gpio_led_set((aic_led_t)i, true);
    }
    /* Blue LED follows POT, but set to 100% on All ON */
    lv_led_on(ex9_blue_led);
    aic_gpio_pwm_set_brightness(AIC_LED_BLUE, 100);
    printf("[HW] All LEDs: ON\r\n");
}

/* All OFF button callback */
static void ex9_all_off_cb(lv_event_t * e)
{
    (void)e;
    /* Turn off Red and Green LEDs */
    for(int i = 0; i < EX9_NUM_TOGGLE_LEDS; i++) {
        ex9_gpios[i].state = false;
        lv_led_off(ex9_gpios[i].led);
        lv_obj_clear_state(ex9_gpios[i].sw, LV_STATE_CHECKED);
        aic_gpio_led_set((aic_led_t)i, false);
    }
    /* Turn off Blue LED */
    lv_led_off(ex9_blue_led);
    aic_gpio_pwm_set_brightness(AIC_LED_BLUE, 0);
    printf("[HW] All LEDs: OFF\r\n");
}

/* Timer to update button, ADC, and Blue LED */
static void ex9_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* Read REAL button state - USER BTN 2 (SW4) */
    bool pressed = aic_gpio_button_read(AIC_BTN_USER2);
    if(ex9_btn_status_led != NULL) {
        if(pressed) {
            lv_led_on(ex9_btn_status_led);
            lv_label_set_text(ex9_btn_status_label, "PRESSED");
            lv_obj_set_style_text_color(ex9_btn_status_label, lv_color_hex(0x00FF00), 0);
        } else {
            lv_led_off(ex9_btn_status_led);
            lv_label_set_text(ex9_btn_status_label, "Released");
            lv_obj_set_style_text_color(ex9_btn_status_label, lv_color_hex(0xFF6666), 0);
        }
    }

    /* Read REAL ADC value and control Blue LED */
    uint8_t percent = aic_adc_read_percent(AIC_ADC_CH0);

    /* Update ADC display */
    if(ex9_adc_label != NULL) {
        lv_label_set_text_fmt(ex9_adc_label, "%u%%", percent);
    }
    if(ex9_adc_bar != NULL) {
        lv_bar_set_value(ex9_adc_bar, percent, LV_ANIM_ON);
    }

    /* Update Blue LED widget brightness from POT (dimming effect 0-255) */
    if(ex9_blue_led != NULL) {
        /* Map 0-100% to LED brightness 0-255 */
        uint8_t brightness = (uint8_t)((percent * 255) / 100);
        /* Always turn ON first, then set brightness for proper dimming */
        lv_led_on(ex9_blue_led);
        lv_led_set_brightness(ex9_blue_led, brightness);
    }
    /* Update REAL Blue LED via PWM */
    aic_gpio_pwm_set_brightness(AIC_LED_BLUE, percent);
}

void part1_ex9_hw_gpio_dashboard(void)
{
    /* ===== HARDWARE INITIALIZATION ===== */
    aic_gpio_init();
    aic_gpio_pwm_init(AIC_LED_BLUE);  /* Initialize PWM for Blue LED */
    aic_sensors_init();

    /* LED names and colors for Red/Green (toggle control) */
    const char *gpio_names[] = {"Red", "Green"};
    lv_color_t gpio_colors[] = {
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_GREEN)
    };

    lv_obj_t * scr = lv_screen_active();

    /* Background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Part 1 Ex9: HW GPIO Dashboard");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* ===== CENTER: LEDs (VERTICAL layout, TRUE CENTER) ===== */
    /* Fixed X position for toggle switches to align on same axis */
    const int toggle_x = 45;  /* Fixed X for all toggles */

    /* Create Red and Green LED items with toggle switches */
    for(int i = 0; i < EX9_NUM_TOGGLE_LEDS; i++) {
        ex9_gpios[i].name = gpio_names[i];
        ex9_gpios[i].state = false;

        int y_pos = 65 + i * 50;  /* Start from y=65, spacing 50px */

        /* LED widget (left of center) */
        ex9_gpios[i].led = lv_led_create(scr);
        lv_obj_set_size(ex9_gpios[i].led, 45, 45);
        lv_obj_align(ex9_gpios[i].led, LV_ALIGN_TOP_MID, -100, y_pos);
        lv_led_set_color(ex9_gpios[i].led, gpio_colors[i]);
        lv_led_off(ex9_gpios[i].led);

        /* Label (next to LED) */
        ex9_gpios[i].label = lv_label_create(scr);
        lv_label_set_text(ex9_gpios[i].label, gpio_names[i]);
        lv_obj_set_style_text_color(ex9_gpios[i].label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(ex9_gpios[i].label, LV_ALIGN_TOP_MID, -35, y_pos + 12);

        /* Switch (fixed X position for alignment) */
        ex9_gpios[i].sw = lv_switch_create(scr);
        lv_obj_set_size(ex9_gpios[i].sw, 70, 38);
        lv_obj_align(ex9_gpios[i].sw, LV_ALIGN_TOP_MID, toggle_x, y_pos + 3);
        lv_obj_add_event_cb(ex9_gpios[i].sw, ex9_switch_cb,
                           LV_EVENT_VALUE_CHANGED, &ex9_gpios[i]);
    }

    /* Blue LED (controlled by POT, no toggle switch) */
    int blue_y = 65 + 2 * 50; 
    ex9_blue_led = lv_led_create(scr);
    lv_obj_set_size(ex9_blue_led, 45, 45);
    lv_obj_align(ex9_blue_led, LV_ALIGN_TOP_MID, -100, blue_y);
    lv_led_set_color(ex9_blue_led, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_off(ex9_blue_led);

    lv_obj_t * blue_label = lv_label_create(scr);
    lv_label_set_text(blue_label, "Blue");
    lv_obj_set_style_text_color(blue_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(blue_label, LV_ALIGN_TOP_MID, -35, blue_y + 5);

    lv_obj_t * blue_note = lv_label_create(scr);
    lv_label_set_text(blue_note, "(POT ctrl)");
    lv_obj_set_style_text_color(blue_note, lv_color_hex(0x00AAFF), 0);
    lv_obj_align(blue_note, LV_ALIGN_TOP_MID, toggle_x, blue_y + 12);

    /* ===== LEFT BOTTOM: Button Status Panel (1.5x size: 225x128) ===== */
    lv_obj_t * btn_panel = lv_obj_create(scr);
    lv_obj_set_size(btn_panel, 225, 128);
    lv_obj_align(btn_panel, LV_ALIGN_BOTTOM_LEFT, 5, -25);
    lv_obj_set_style_bg_color(btn_panel, lv_color_hex(0x0f0f23), 0);
    lv_obj_set_style_border_width(btn_panel, 2, 0);
    lv_obj_set_style_border_color(btn_panel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_pad_all(btn_panel, 10, 0);
    lv_obj_clear_flag(btn_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_title = lv_label_create(btn_panel);
    lv_label_set_text(btn_title, "USER BTN2 (SW4)");
    lv_obj_set_style_text_color(btn_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(btn_title, LV_ALIGN_TOP_MID, 0, 0);

    ex9_btn_status_led = lv_led_create(btn_panel);
    lv_obj_set_size(ex9_btn_status_led, 60, 60);
    lv_obj_align(ex9_btn_status_led, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_led_set_color(ex9_btn_status_led, lv_palette_main(LV_PALETTE_ORANGE));
    lv_led_off(ex9_btn_status_led);

    ex9_btn_status_label = lv_label_create(btn_panel);
    lv_label_set_text(ex9_btn_status_label, "Released");
    lv_obj_set_style_text_color(ex9_btn_status_label, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_text_font(ex9_btn_status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(ex9_btn_status_label, LV_ALIGN_BOTTOM_RIGHT, -10, -20);

    /* ===== RIGHT BOTTOM: ADC Panel (1.5x size: 225x128) ===== */
    lv_obj_t * adc_panel = lv_obj_create(scr);
    lv_obj_set_size(adc_panel, 225, 128);
    lv_obj_align(adc_panel, LV_ALIGN_BOTTOM_RIGHT, -5, -25);
    lv_obj_set_style_bg_color(adc_panel, lv_color_hex(0x0f0f23), 0);
    lv_obj_set_style_border_width(adc_panel, 2, 0);
    lv_obj_set_style_border_color(adc_panel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_pad_all(adc_panel, 10, 0);
    lv_obj_clear_flag(adc_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * adc_title = lv_label_create(adc_panel);
    lv_label_set_text(adc_title, "POT -> Blue LED");
    lv_obj_set_style_text_color(adc_title, lv_color_hex(0x00AAFF), 0);
    lv_obj_align(adc_title, LV_ALIGN_TOP_MID, 0, 0);

    ex9_adc_bar = lv_bar_create(adc_panel);
    lv_obj_set_size(ex9_adc_bar, 180, 25);
    lv_obj_align(ex9_adc_bar, LV_ALIGN_CENTER, 0, -5);
    lv_bar_set_range(ex9_adc_bar, 0, 100);
    lv_bar_set_value(ex9_adc_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ex9_adc_bar, lv_color_hex(0x001133), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ex9_adc_bar, lv_color_hex(0x0088FF), LV_PART_INDICATOR);

    ex9_adc_label = lv_label_create(adc_panel);
    lv_label_set_text(ex9_adc_label, "0%");
    lv_obj_set_style_text_color(ex9_adc_label, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_text_font(ex9_adc_label, &lv_font_montserrat_24, 0);
    lv_obj_align(ex9_adc_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    /* ===== BOTTOM CENTER: All ON / All OFF buttons (moved up more) ===== */
    lv_obj_t * btn_all_on = lv_button_create(scr);
    lv_obj_add_event_cb(btn_all_on, ex9_all_on_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn_all_on, 100, 40);
    lv_obj_align(btn_all_on, LV_ALIGN_BOTTOM_MID, -60, -160);  /* Bottom center, moved up */
    lv_obj_set_style_bg_color(btn_all_on, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t * label_on = lv_label_create(btn_all_on);
    lv_label_set_text(label_on, "All ON");
    lv_obj_center(label_on);

    lv_obj_t * btn_all_off = lv_button_create(scr);
    lv_obj_add_event_cb(btn_all_off, ex9_all_off_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn_all_off, 100, 40);
    lv_obj_align(btn_all_off, LV_ALIGN_BOTTOM_MID, 60, -160);  /* Bottom center, moved up */
    lv_obj_set_style_bg_color(btn_all_off, lv_palette_main(LV_PALETTE_RED), 0);

    lv_obj_t * label_off = lv_label_create(btn_all_off);
    lv_label_set_text(label_off, "All OFF");
    lv_obj_center(label_off);

    /* Footer */
    aic_create_footer(scr);

    /* Timer to update button, ADC, and Blue LED */
    ex9_timer = lv_timer_create(ex9_timer_cb, 100, NULL);

    printf("[Week3] Ex9: Hardware GPIO Dashboard started\r\n");
    printf("  - Red/Green: Toggle ON/OFF\r\n");
    printf("  - Blue: Controlled by Potentiometer (PWM)\r\n");
}

/*******************************************************************************
 * Example 10: CAPSENSE UI Mockup
 *
 * Same UI as Ex11 but WITHOUT hardware - for students to understand
 * the CAPSENSE UI design before hardware integration
 *
 * Layout (matches actual board):
 *   - SLIDER (CSS1) on TOP
 *   - BTN0 (CSB1) bottom LEFT, BTN1 (CSB2) bottom RIGHT
 *
 ******************************************************************************/

#define EX10_NUM_BUTTONS 2  /* CAPSENSE has 2 buttons */

/* Ex10 UI elements */
static lv_obj_t * ex10_btn_panels[EX10_NUM_BUTTONS];
static lv_obj_t * ex10_btn_leds[EX10_NUM_BUTTONS];
static lv_obj_t * ex10_btn_status[EX10_NUM_BUTTONS];
static lv_obj_t * ex10_slider;
static lv_obj_t * ex10_slider_value;
static lv_obj_t * ex10_output_led;
static lv_obj_t * ex10_mode_label;
static lv_timer_t * ex10_demo_timer;
static bool ex10_demo_mode = true;
static int ex10_demo_step = 0;

/* Button touch callback (UI only, no hardware) */
static void ex10_btn_touch_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        /* Turn off demo mode when user interacts */
        ex10_demo_mode = false;
        lv_label_set_text(ex10_mode_label, "Mode: Manual");

        /* Touch down - animate button press */
        lv_obj_set_style_bg_color(ex10_btn_panels[idx], lv_color_hex(0x00AA00), 0);
        lv_led_on(ex10_btn_leds[idx]);
        lv_label_set_text(ex10_btn_status[idx], "TOUCHED");
        lv_obj_set_style_text_color(ex10_btn_status[idx], lv_color_hex(0x00FF00), 0);
        printf("[MOCKUP] BTN%d: TOUCHED\r\n", (int)idx);
    }
    else if(code == LV_EVENT_RELEASED) {
        /* Touch release - restore button */
        lv_obj_set_style_bg_color(ex10_btn_panels[idx], lv_color_hex(0x333355), 0);
        lv_led_off(ex10_btn_leds[idx]);
        lv_label_set_text(ex10_btn_status[idx], "Ready");
        lv_obj_set_style_text_color(ex10_btn_status[idx], lv_color_hex(0x888888), 0);
        printf("[MOCKUP] BTN%d: Released\r\n", (int)idx);
    }
}

/* Slider callback (UI only, no hardware) */
static void ex10_slider_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    /* Turn off demo mode when user interacts */
    ex10_demo_mode = false;
    lv_label_set_text(ex10_mode_label, "Mode: Manual");

    /* Update value label */
    lv_label_set_text_fmt(ex10_slider_value, "%d%%", (int)value);

    /* Update output LED brightness */
    uint8_t brightness = (uint8_t)((value * 255) / 100);
    lv_led_on(ex10_output_led);
    lv_led_set_brightness(ex10_output_led, brightness);

    printf("[MOCKUP] Slider: %d%%\r\n", (int)value);
}

/* Auto-demo timer callback (UI only, no hardware) */
static void ex10_demo_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    if(!ex10_demo_mode) return;

    /* Demo sequence: Press 2 buttons and move slider */
    switch(ex10_demo_step % 8) {
        case 0:
            /* Press BTN0 */
            lv_obj_set_style_bg_color(ex10_btn_panels[0], lv_color_hex(0x00AA00), 0);
            lv_led_on(ex10_btn_leds[0]);
            lv_label_set_text(ex10_btn_status[0], "TOUCHED");
            lv_obj_set_style_text_color(ex10_btn_status[0], lv_color_hex(0x00FF00), 0);
            break;
        case 1:
            /* Release BTN0 */
            lv_obj_set_style_bg_color(ex10_btn_panels[0], lv_color_hex(0x333355), 0);
            lv_led_off(ex10_btn_leds[0]);
            lv_label_set_text(ex10_btn_status[0], "Ready");
            lv_obj_set_style_text_color(ex10_btn_status[0], lv_color_hex(0x888888), 0);
            break;
        case 2:
            /* Press BTN1 */
            lv_obj_set_style_bg_color(ex10_btn_panels[1], lv_color_hex(0x00AA00), 0);
            lv_led_on(ex10_btn_leds[1]);
            lv_label_set_text(ex10_btn_status[1], "TOUCHED");
            lv_obj_set_style_text_color(ex10_btn_status[1], lv_color_hex(0x00FF00), 0);
            break;
        case 3:
            /* Release BTN1 */
            lv_obj_set_style_bg_color(ex10_btn_panels[1], lv_color_hex(0x333355), 0);
            lv_led_off(ex10_btn_leds[1]);
            lv_label_set_text(ex10_btn_status[1], "Ready");
            lv_obj_set_style_text_color(ex10_btn_status[1], lv_color_hex(0x888888), 0);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            /* Slider animation */
            {
                int slider_val = ((ex10_demo_step % 8) - 4) * 33;
                if(slider_val > 100) slider_val = 100;
                lv_slider_set_value(ex10_slider, slider_val, LV_ANIM_ON);
                lv_label_set_text_fmt(ex10_slider_value, "%d%%", slider_val);
                uint8_t br = (uint8_t)((slider_val * 255) / 100);
                lv_led_on(ex10_output_led);
                lv_led_set_brightness(ex10_output_led, br);
            }
            break;
    }
    ex10_demo_step++;
}

void part1_ex10_capsense_mockup(void)
{
    lv_obj_t * scr = lv_screen_active();

    /* Background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Part 1 Ex10: CAPSENSE UI Mockup");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* Mode indicator (replaces subtitle) */
    ex10_mode_label = lv_label_create(scr);
    lv_label_set_text(ex10_mode_label, "Mode: Auto Demo");
    lv_obj_set_style_text_color(ex10_mode_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(ex10_mode_label, LV_ALIGN_TOP_MID, 0, 30);

    /* Reset demo state */
    ex10_demo_mode = true;
    ex10_demo_step = 0;

    /* ===== CAPSENSE SLIDER (TOP) - Matches board layout ===== */
    lv_obj_t * slider_panel = lv_obj_create(scr);
    lv_obj_set_size(slider_panel, 420, 80);
    lv_obj_align(slider_panel, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_set_style_bg_color(slider_panel, lv_color_hex(0x0f0f23), 0);
    lv_obj_set_style_pad_all(slider_panel, 8, 0);
    lv_obj_clear_flag(slider_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Title + Value on same line */
    lv_obj_t * slider_title = lv_label_create(slider_panel);
    lv_label_set_text(slider_title, "SLIDER (CSS1)");
    lv_obj_set_style_text_color(slider_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(slider_title, LV_ALIGN_TOP_LEFT, 10, 0);

    ex10_slider_value = lv_label_create(slider_panel);
    lv_label_set_text(ex10_slider_value, "0%");
    lv_obj_set_style_text_color(ex10_slider_value, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_text_font(ex10_slider_value, &lv_font_montserrat_16, 0);
    lv_obj_align(ex10_slider_value, LV_ALIGN_TOP_RIGHT, -10, 0);

    /* Slider below title */
    ex10_slider = lv_slider_create(slider_panel);
    lv_obj_set_width(ex10_slider, 340);
    lv_obj_set_height(ex10_slider, 25);
    lv_obj_align(ex10_slider, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_slider_set_range(ex10_slider, 0, 100);
    lv_slider_set_value(ex10_slider, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ex10_slider, lv_color_hex(0x333355), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ex10_slider, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
    lv_obj_add_event_cb(ex10_slider, ex10_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Output LED next to slider */
    ex10_output_led = lv_led_create(slider_panel);
    lv_obj_set_size(ex10_output_led, 25, 25);
    lv_obj_align(ex10_output_led, LV_ALIGN_BOTTOM_RIGHT, -5, -8);
    lv_led_set_color(ex10_output_led, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_led_off(ex10_output_led);

    /* ===== CAPSENSE BUTTONS (BOTTOM) - BTN0 left, BTN1 right ===== */
    const char * btn_names[] = {"BTN0", "BTN1"};
    const char * btn_ids[] = {"(CSB1)", "(CSB2)"};
    uint32_t led_colors[] = {0xff0000, 0x00ff00};
    int btn_x_pos[] = {-110, 110};  /* Left and Right */

    for(int i = 0; i < EX10_NUM_BUTTONS; i++) {
        /* Touch panel - taller for better spacing */
        ex10_btn_panels[i] = lv_obj_create(scr);
        lv_obj_set_size(ex10_btn_panels[i], 140, 150);
        lv_obj_align(ex10_btn_panels[i], LV_ALIGN_BOTTOM_MID, btn_x_pos[i], -85);
        lv_obj_set_style_bg_color(ex10_btn_panels[i], lv_color_hex(0x333355), 0);
        lv_obj_set_style_border_width(ex10_btn_panels[i], 3, 0);
        lv_obj_set_style_border_color(ex10_btn_panels[i], lv_color_hex(0x666699), 0);
        lv_obj_set_style_radius(ex10_btn_panels[i], 10, 0);
        lv_obj_set_style_pad_all(ex10_btn_panels[i], 5, 0);
        lv_obj_add_flag(ex10_btn_panels[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(ex10_btn_panels[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(ex10_btn_panels[i], ex10_btn_touch_cb, LV_EVENT_PRESSED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(ex10_btn_panels[i], ex10_btn_touch_cb, LV_EVENT_RELEASED, (void*)(intptr_t)i);

        /* Button name at top */
        lv_obj_t * name = lv_label_create(ex10_btn_panels[i]);
        lv_label_set_text(name, btn_names[i]);
        lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 2);

        /* Button ID below name */
        lv_obj_t * id_lbl = lv_label_create(ex10_btn_panels[i]);
        lv_label_set_text(id_lbl, btn_ids[i]);
        lv_obj_set_style_text_color(id_lbl, lv_color_hex(0x888888), 0);
        lv_obj_align(id_lbl, LV_ALIGN_TOP_MID, 0, 22);

        /* LED indicator in center */
        ex10_btn_leds[i] = lv_led_create(ex10_btn_panels[i]);
        lv_obj_set_size(ex10_btn_leds[i], 50, 50);
        lv_obj_align(ex10_btn_leds[i], LV_ALIGN_CENTER, 0, 8);
        lv_led_set_color(ex10_btn_leds[i], lv_color_hex(led_colors[i]));
        lv_led_off(ex10_btn_leds[i]);

        /* Status label at bottom */
        ex10_btn_status[i] = lv_label_create(ex10_btn_panels[i]);
        lv_label_set_text(ex10_btn_status[i], "Ready");
        lv_obj_set_style_text_color(ex10_btn_status[i], lv_color_hex(0x888888), 0);
        lv_obj_align(ex10_btn_status[i], LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    /* Description */
    lv_obj_t * desc = lv_label_create(scr);
    lv_label_set_text(desc, "Touch to switch to Manual. Ex11 reads real CAPSENSE.");
    lv_obj_set_style_text_color(desc, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -25);

    /* Footer */
    aic_create_footer(scr);

    /* Start demo timer */
    ex10_demo_timer = lv_timer_create(ex10_demo_timer_cb, 500, NULL);

    printf("[Week3] Ex10: CAPSENSE UI Mockup (Auto-Demo) started\r\n");
    printf("  - Touch screen to switch to Manual mode\r\n");
}

/*******************************************************************************
 * Example 11: CAPSENSE Hardware (Direct I2C on CM55)
 *
 * Reads CAPSENSE data directly from PSoC 4000T via I2C:
 *   - 2 Touch Buttons (CSB1, CSB2) with LED feedback
 *   - Slider (CSS1) with Blue LED PWM control
 *
 * Architecture:
 *   - CM55 reads PSoC 4000T directly via I2C (address 0x08)
 *   - Shares I2C bus (SCB0) with display touch controller
 *   - No shared memory or CM33 involvement needed!
 *
 * I2C Communication:
 *   - Slave Address: 0x08 (PSoC 4000T)
 *   - Data Format: 3 bytes [BTN0, BTN1, Slider]
 *   - Uses aic_capsense_read() from sensors.h
 *
 * LED Control:
 *   - BTN0 (CSB1) -> Red LED
 *   - BTN1 (CSB2) -> Green LED
 *   - Slider (CSS1) -> Blue LED (PWM brightness)
 *
 ******************************************************************************/

/* Include CAPSENSE API from sensors.h */
#include "../aic-eec/sensors.h"

#define EX11_NUM_BUTTONS 2  /* CAPSENSE has 2 buttons */

/* Ex11 UI elements */
static lv_obj_t * ex11_btn_panels[EX11_NUM_BUTTONS];   /* Button display panels */
static lv_obj_t * ex11_btn_leds[EX11_NUM_BUTTONS];     /* LED indicators */
static lv_obj_t * ex11_btn_status[EX11_NUM_BUTTONS];   /* Status labels */
static lv_obj_t * ex11_slider;             /* Slider bar */
static lv_obj_t * ex11_slider_value;       /* Slider value label */
static lv_obj_t * ex11_output_led;         /* Blue LED indicator */
static lv_obj_t * ex11_status_label;       /* Connection status */
static lv_timer_t * ex11_poll_timer;

/* Previous CAPSENSE state for edge detection */
static bool ex11_prev_btn0 = false;
static bool ex11_prev_btn1 = false;
static uint8_t ex11_prev_slider = 0;

/* Helper function to update UI with CAPSENSE data (real or simulated) */
static void ex11_update_ui(bool btn0, bool btn1, uint8_t slider_pos, bool slider_active)
{
    /* === Button 0 (CSB1) === */
    if (btn0 != ex11_prev_btn0) {
        if (btn0) {
            lv_obj_set_style_bg_color(ex11_btn_panels[0], lv_color_hex(0x00AA00), 0);
            lv_led_on(ex11_btn_leds[0]);
            lv_label_set_text(ex11_btn_status[0], "TOUCHED");
            lv_obj_set_style_text_color(ex11_btn_status[0], lv_color_hex(0x00FF00), 0);
            aic_gpio_led_set(AIC_LED_RED, true);
            printf("[CAPS] BTN0: TOUCHED\r\n");
        } else {
            lv_obj_set_style_bg_color(ex11_btn_panels[0], lv_color_hex(0x333355), 0);
            lv_led_off(ex11_btn_leds[0]);
            lv_label_set_text(ex11_btn_status[0], "Ready");
            lv_obj_set_style_text_color(ex11_btn_status[0], lv_color_hex(0x888888), 0);
            aic_gpio_led_set(AIC_LED_RED, false);
        }
        ex11_prev_btn0 = btn0;
    }

    /* === Button 1 (CSB2) === */
    if (btn1 != ex11_prev_btn1) {
        if (btn1) {
            lv_obj_set_style_bg_color(ex11_btn_panels[1], lv_color_hex(0x00AA00), 0);
            lv_led_on(ex11_btn_leds[1]);
            lv_label_set_text(ex11_btn_status[1], "TOUCHED");
            lv_obj_set_style_text_color(ex11_btn_status[1], lv_color_hex(0x00FF00), 0);
            aic_gpio_led_set(AIC_LED_GREEN, true);
            printf("[CAPS] BTN1: TOUCHED\r\n");
        } else {
            lv_obj_set_style_bg_color(ex11_btn_panels[1], lv_color_hex(0x333355), 0);
            lv_led_off(ex11_btn_leds[1]);
            lv_label_set_text(ex11_btn_status[1], "Ready");
            lv_obj_set_style_text_color(ex11_btn_status[1], lv_color_hex(0x888888), 0);
            aic_gpio_led_set(AIC_LED_GREEN, false);
        }
        ex11_prev_btn1 = btn1;
    }

    /* === Slider (CSS1) === */
    if (slider_pos != ex11_prev_slider) {
        lv_slider_set_value(ex11_slider, slider_pos, LV_ANIM_ON);
        lv_label_set_text_fmt(ex11_slider_value, "%d%%", slider_pos);

        uint8_t brightness = (uint8_t)((slider_pos * 255) / 100);
        if (slider_active) {
            lv_led_on(ex11_output_led);
            lv_led_set_brightness(ex11_output_led, brightness);
        } else {
            lv_led_off(ex11_output_led);
        }

        aic_gpio_pwm_set_brightness(AIC_LED_BLUE, slider_pos);
        ex11_prev_slider = slider_pos;
    }
}

/* Track read count for status display */
static uint32_t ex11_read_count = 0;
static uint32_t ex11_error_count = 0;
static bool ex11_connected = false;  /* True when I2C read succeeds */

/* Timer callback - reads CAPSENSE data directly via I2C */
static void ex11_poll_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* Read CAPSENSE data directly from PSoC 4000T via I2C */
    aic_capsense_data_t caps_data;

    if (aic_capsense_read(&caps_data)) {
        /* I2C read successful */
        ex11_read_count++;

        /* First time connected */
        if (!ex11_connected) {
            ex11_connected = true;
            printf("[Ex11] CAPSENSE I2C connected!\r\n");
        }

        /* DEBUG: Print raw CAPSENSE values every 20 reads (~1s at 50ms) */
        if ((ex11_read_count % 20) == 0) {
            printf("[CAPS] cnt=%u BTN0=%d BTN1=%d Slider=%d Active=%d\r\n",
                   (unsigned int)ex11_read_count,
                   caps_data.btn0_pressed ? 1 : 0,
                   caps_data.btn1_pressed ? 1 : 0,
                   caps_data.slider_pos,
                   caps_data.slider_active ? 1 : 0);
        }

        /* Show status with read count */
        lv_label_set_text_fmt(ex11_status_label, "I2C: cnt=%u OK",
                              (unsigned int)ex11_read_count);
        lv_obj_set_style_text_color(ex11_status_label, lv_color_hex(0x00FF00), 0);

        /* Update UI with CAPSENSE data */
        ex11_update_ui(caps_data.btn0_pressed,
                       caps_data.btn1_pressed,
                       caps_data.slider_pos,
                       caps_data.slider_active);
    } else {
        /* I2C read failed */
        ex11_error_count++;

        if (ex11_connected) {
            /* Was connected, now failed */
            lv_label_set_text_fmt(ex11_status_label, "I2C Error (err=%u)",
                                  (unsigned int)ex11_error_count);
            lv_obj_set_style_text_color(ex11_status_label, lv_color_hex(0xFF0000), 0);
        } else {
            /* Never connected */
            lv_label_set_text(ex11_status_label, "I2C: No response");
            lv_obj_set_style_text_color(ex11_status_label, lv_color_hex(0xFFFF00), 0);
        }
    }
}

void part1_ex11_capsense_hardware(void)
{
    /* ===== DISABLE DISPLAY TOUCH FOR I2C BUS SHARING ===== */
    /* CAPSENSE and Display Touch share the same I2C bus (SCB0).
     * Disable touch reads so CAPSENSE has exclusive bus access.
     * CAPSENSE becomes the primary touch input for this example. */
    lv_port_indev_disable_touch();

    /* ===== HARDWARE INITIALIZATION ===== */
    aic_gpio_init();
    aic_gpio_pwm_init(AIC_LED_BLUE);

    /* Initialize CAPSENSE I2C (now has exclusive I2C bus access) */
    aic_capsense_init();

    /* Reset state for direct I2C */
    ex11_prev_btn0 = false;
    ex11_prev_btn1 = false;
    ex11_prev_slider = 0;
    ex11_read_count = 0;
    ex11_error_count = 0;
    ex11_connected = false;

    lv_obj_t * scr = lv_screen_active();

    /* Background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Title - updated for direct I2C mode */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Part 1 Ex11: CAPSENSE (I2C)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* Connection status indicator */
    ex11_status_label = lv_label_create(scr);
    /* Check if CAPSENSE is available */
    if (aic_capsense_is_available()) {
        lv_label_set_text(ex11_status_label, "Mode: Direct I2C (CM55)");
        lv_obj_set_style_text_color(ex11_status_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(ex11_status_label, "Mode: I2C not available");
        lv_obj_set_style_text_color(ex11_status_label, lv_color_hex(0xFFFF00), 0);
    }
    lv_obj_align(ex11_status_label, LV_ALIGN_TOP_MID, 0, 30);

    /* ===== CAPSENSE SLIDER (TOP) - Matches Ex10 layout ===== */
    lv_obj_t * slider_panel = lv_obj_create(scr);
    lv_obj_set_size(slider_panel, 420, 80);
    lv_obj_align(slider_panel, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_color(slider_panel, lv_color_hex(0x0f0f23), 0);
    lv_obj_set_style_pad_all(slider_panel, 8, 0);
    lv_obj_clear_flag(slider_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Title + Value on same line (like Ex10) */
    lv_obj_t * slider_title = lv_label_create(slider_panel);
    lv_label_set_text(slider_title, "SLIDER (CSS1)");
    lv_obj_set_style_text_color(slider_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(slider_title, LV_ALIGN_TOP_LEFT, 10, 0);

    ex11_slider_value = lv_label_create(slider_panel);
    lv_label_set_text(ex11_slider_value, "0%");
    lv_obj_set_style_text_color(ex11_slider_value, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_text_font(ex11_slider_value, &lv_font_montserrat_16, 0);
    lv_obj_align(ex11_slider_value, LV_ALIGN_TOP_RIGHT, -10, 0);

    /* Slider below title (read-only, controlled by I2C CAPSENSE) */
    ex11_slider = lv_slider_create(slider_panel);
    lv_obj_set_width(ex11_slider, 340);
    lv_obj_set_height(ex11_slider, 25);
    lv_obj_align(ex11_slider, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_slider_set_range(ex11_slider, 0, 100);
    lv_slider_set_value(ex11_slider, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ex11_slider, lv_color_hex(0x333355), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ex11_slider, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
    lv_obj_remove_flag(ex11_slider, LV_OBJ_FLAG_CLICKABLE);  /* Read-only */

    /* Output LED next to slider (like Ex10) */
    ex11_output_led = lv_led_create(slider_panel);
    lv_obj_set_size(ex11_output_led, 25, 25);
    lv_obj_align(ex11_output_led, LV_ALIGN_BOTTOM_RIGHT, -5, -8);
    lv_led_set_color(ex11_output_led, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_led_off(ex11_output_led);

    /* ===== CAPSENSE BUTTONS (BOTTOM) - BTN0 left, BTN1 right (like Ex10) ===== */
    const char * btn_names[] = {"BTN0", "BTN1"};
    const char * btn_ids[] = {"(CSB1)", "(CSB2)"};
    uint32_t led_colors[] = {0xff0000, 0x00ff00};
    int btn_x_pos[] = {-110, 110};  /* Left and Right */

    for(int i = 0; i < EX11_NUM_BUTTONS; i++) {
        /* Touch panel - same size as Ex10 */
        ex11_btn_panels[i] = lv_obj_create(scr);
        lv_obj_set_size(ex11_btn_panels[i], 140, 150);
        lv_obj_align(ex11_btn_panels[i], LV_ALIGN_BOTTOM_MID, btn_x_pos[i], -55);
        lv_obj_set_style_bg_color(ex11_btn_panels[i], lv_color_hex(0x333355), 0);
        lv_obj_set_style_border_width(ex11_btn_panels[i], 3, 0);
        lv_obj_set_style_border_color(ex11_btn_panels[i], lv_color_hex(0x666699), 0);
        lv_obj_set_style_radius(ex11_btn_panels[i], 10, 0);
        lv_obj_set_style_pad_all(ex11_btn_panels[i], 5, 0);
        lv_obj_clear_flag(ex11_btn_panels[i], LV_OBJ_FLAG_SCROLLABLE);
        /* No touch callbacks - controlled by I2C polling */

        /* Button name at top */
        lv_obj_t * name = lv_label_create(ex11_btn_panels[i]);
        lv_label_set_text(name, btn_names[i]);
        lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 2);

        /* Button ID below name */
        lv_obj_t * id_lbl = lv_label_create(ex11_btn_panels[i]);
        lv_label_set_text(id_lbl, btn_ids[i]);
        lv_obj_set_style_text_color(id_lbl, lv_color_hex(0x888888), 0);
        lv_obj_align(id_lbl, LV_ALIGN_TOP_MID, 0, 22);

        /* LED indicator in center */
        ex11_btn_leds[i] = lv_led_create(ex11_btn_panels[i]);
        lv_obj_set_size(ex11_btn_leds[i], 50, 50);
        lv_obj_align(ex11_btn_leds[i], LV_ALIGN_CENTER, 0, 8);
        lv_led_set_color(ex11_btn_leds[i], lv_color_hex(led_colors[i]));
        lv_led_off(ex11_btn_leds[i]);

        /* Status label at bottom */
        ex11_btn_status[i] = lv_label_create(ex11_btn_panels[i]);
        lv_label_set_text(ex11_btn_status[i], "Ready");
        lv_obj_set_style_text_color(ex11_btn_status[i], lv_color_hex(0x888888), 0);
        lv_obj_align(ex11_btn_status[i], LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    /* Description */
    lv_obj_t * desc = lv_label_create(scr);
    lv_label_set_text(desc, "CM55 reads PSoC 4000T directly via I2C (0x08)");
    lv_obj_set_style_text_color(desc, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -25);

    /* Footer */
    aic_create_footer(scr);

    /* Start I2C polling timer (50ms = 20Hz for responsive CAPSENSE input) */
    ex11_poll_timer = lv_timer_create(ex11_poll_timer_cb, 50, NULL);

    printf("[Week3] Ex11: CAPSENSE Hardware (Direct I2C on CM55)\r\n");
    printf("  - I2C Bus: SCB0 (shared with display touch)\r\n");
    printf("  - PSoC 4000T Address: 0x08\r\n");
    printf("  - BTN0 (CSB1) -> Red LED\r\n");
    printf("  - BTN1 (CSB2) -> Green LED\r\n");
    printf("  - Slider (CSS1) -> Blue LED (PWM)\r\n");
}

/* [] END OF FILE */
