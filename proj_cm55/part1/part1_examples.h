/*******************************************************************************
 * File Name:   part1_examples.h
 *
 * Description: Header file for Part 1 LVGL examples - GPIO Integration
 *              Part of Embedded C for IoT Course
 *
 * Part I Examples (UI Only - Simulated):
 *   1. Hello World Label
 *   2. Button with Event
 *   3. LED Widget Control
 *   4. Switch for GPIO
 *   5. GPIO Dashboard
 *
 * Part II Examples (Hardware Integration - aic-eec API):
 *   6. Hardware LED Control (3 LEDs)
 *   7. Hardware Button Status
 *   8. Hardware ADC Display
 *   9. Hardware GPIO Dashboard
 *  10. CAPSENSE UI Mockup
 *  11. CAPSENSE Hardware (I2C)
 *
 ******************************************************************************/

#ifndef PART1_EXAMPLES_H
#define PART1_EXAMPLES_H

#include "lvgl.h"

/*******************************************************************************
 * Part I: UI Only Examples (1-5)
 ******************************************************************************/

/**
 * @brief Example 1: Hello World - Basic Label
 * Learning: lv_label_create, lv_obj_align, lv_obj_set_style_*
 */
void part1_ex1_hello_world(void);

/**
 * @brief Example 2: Button with Click Counter
 * Learning: lv_button_create, lv_obj_add_event_cb, LV_EVENT_CLICKED
 */
void part1_ex2_button_counter(void);

/**
 * @brief Example 3: LED Widget Control
 * Learning: lv_led_create, lv_led_on/off, lv_led_set_brightness
 */
void part1_ex3_led_control(void);

/**
 * @brief Example 4: Switch Widget (ON/OFF Toggle)
 * Learning: lv_switch_create, LV_STATE_CHECKED, LV_EVENT_VALUE_CHANGED
 */
void part1_ex4_switch_toggle(void);

/**
 * @brief Example 5: GPIO Dashboard (Multiple LEDs and Switches)
 * Learning: Layout, Multiple widgets interaction
 */
void part1_ex5_gpio_dashboard(void);

/*******************************************************************************
 * Part II: Hardware Integration Examples (6-11)
 * Uses aic-eec/gpio.h API for real hardware control
 ******************************************************************************/

/**
 * @brief Example 6: Hardware LED Control
 * Control 3 on-board LEDs (Red, Green, Blue) using aic_gpio_led_set()
 * Based on Ex3, but with real hardware
 */
void part1_ex6_hw_led_control(void);

/**
 * @brief Example 7: Hardware Button Status
 * Display real button state using aic_gpio_button_read()
 * Shows pressed/released status with LED indicator
 */
void part1_ex7_hw_button_status(void);

/**
 * @brief Example 8: Hardware ADC Display
 * Read potentiometer via aic_adc_read() and display on Slider/Bar
 * Based on Week4 Ex1 pattern, but with real ADC
 */
void part1_ex8_hw_adc_display(void);

/**
 * @brief Example 9: Hardware GPIO Dashboard
 * Combined dashboard with real LEDs, Button, and ADC
 * Based on Ex5, but with real hardware integration
 */
void part1_ex9_hw_gpio_dashboard(void);

/**
 * @brief Example 10: CAPSENSE UI Mockup
 * UI design for CAPSENSE buttons and slider (no hardware)
 * Prepare UI before hardware integration
 */
void part1_ex10_capsense_mockup(void);

/**
 * @brief Example 11: CAPSENSE Hardware Integration
 * Real CAPSENSE buttons and slider via I2C
 * Based on Ex10 with hardware connection
 */
void part1_ex11_capsense_hardware(void);

#endif /* PART1_EXAMPLES_H */
