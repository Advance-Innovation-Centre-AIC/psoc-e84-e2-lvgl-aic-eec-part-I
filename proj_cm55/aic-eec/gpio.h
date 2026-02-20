/*******************************************************************************
 * File Name:   gpio.h
 *
 * Description: AIC-EEC GPIO Hardware Abstraction Layer
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Components:
 *   - LED Control (On-board LEDs)
 *   - Button Input (User buttons)
 *   - PWM LED Control (Brightness)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 * Usage:
 *   #include "gpio.h"
 *   aic_gpio_init();
 *   aic_gpio_led_set(AIC_LED_RED, true);
 *
 ******************************************************************************/

#ifndef AIC_GPIO_H
#define AIC_GPIO_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * LED Definitions - PSoC Edge E84 Kit
 ******************************************************************************/

/** LED identifiers for on-board LEDs */
typedef enum {
    AIC_LED_RED = 0,    /**< User LED 1 (Red) - P19_2 */
    AIC_LED_GREEN,      /**< User LED 2 (Green) - P19_3 */
    AIC_LED_BLUE,       /**< User LED 3 (Blue) - P16_5 (PWM capable) */
    AIC_LED_COUNT       /**< Total number of LEDs */
} aic_led_t;

/*******************************************************************************
 * Button Definitions
 ******************************************************************************/

/** Button identifiers for on-board buttons */
typedef enum {
    AIC_BTN_USER = 0,   /**< User Button 1 (SW2) - P8_3 */
    AIC_BTN_USER2,      /**< User Button 2 (SW4) - P8_7 */
    AIC_BTN_COUNT       /**< Total number of buttons */
} aic_button_t;

/** Button callback function type */
typedef void (*aic_button_callback_t)(void);

/*******************************************************************************
 * GPIO Initialization
 ******************************************************************************/

/**
 * @brief Initialize all GPIO pins (LEDs and Buttons)
 * @return true if initialization successful, false otherwise
 * @note Call this function once at startup before using other GPIO functions
 */
bool aic_gpio_init(void);

/**
 * @brief Deinitialize GPIO pins and release resources
 */
void aic_gpio_deinit(void);

/*******************************************************************************
 * LED Control Functions
 ******************************************************************************/

/**
 * @brief Set LED state (on/off)
 * @param led LED identifier (AIC_LED_RED, AIC_LED_GREEN, AIC_LED_BLUE)
 * @param state true = ON, false = OFF
 */
void aic_gpio_led_set(aic_led_t led, bool state);

/**
 * @brief Toggle LED state
 * @param led LED identifier
 */
void aic_gpio_led_toggle(aic_led_t led);

/**
 * @brief Get current LED state
 * @param led LED identifier
 * @return true if LED is ON, false if OFF
 */
bool aic_gpio_led_get(aic_led_t led);

/**
 * @brief Turn all LEDs on
 */
void aic_gpio_led_all_on(void);

/**
 * @brief Turn all LEDs off
 */
void aic_gpio_led_all_off(void);

/*******************************************************************************
 * PWM LED Control Functions (Brightness)
 ******************************************************************************/

/**
 * @brief Initialize PWM for LED brightness control
 * @param led LED identifier (only AIC_LED_BLUE supports PWM)
 * @return true if PWM initialization successful, false otherwise
 * @note Only LED3 (Blue) on P16_5 supports hardware PWM on this kit
 */
bool aic_gpio_pwm_init(aic_led_t led);

/**
 * @brief Set LED brightness using PWM
 * @param led LED identifier
 * @param brightness Brightness level (0-100%)
 */
void aic_gpio_pwm_set_brightness(aic_led_t led, uint8_t brightness);

/**
 * @brief Get current PWM brightness setting
 * @param led LED identifier
 * @return Current brightness level (0-100%)
 */
uint8_t aic_gpio_pwm_get_brightness(aic_led_t led);

/**
 * @brief Set PWM frequency
 * @param led LED identifier
 * @param freq_hz PWM frequency in Hz (default 1000 Hz)
 */
void aic_gpio_pwm_set_frequency(aic_led_t led, uint32_t freq_hz);

/**
 * @brief Disable PWM and restore GPIO control
 * @param led LED identifier
 */
void aic_gpio_pwm_deinit(aic_led_t led);

/*******************************************************************************
 * Button Input Functions
 ******************************************************************************/

/**
 * @brief Read button state (with debouncing)
 * @param btn Button identifier
 * @return true if button is pressed, false otherwise
 */
bool aic_gpio_button_read(aic_button_t btn);

/**
 * @brief Read raw button state (no debouncing)
 * @param btn Button identifier
 * @return true if button is pressed, false otherwise
 */
bool aic_gpio_button_read_raw(aic_button_t btn);

/**
 * @brief Register callback for button press interrupt
 * @param btn Button identifier
 * @param callback Function to call when button is pressed
 * @return true if callback registered successfully
 */
bool aic_gpio_button_set_callback(aic_button_t btn, aic_button_callback_t callback);

/**
 * @brief Clear button callback
 * @param btn Button identifier
 */
void aic_gpio_button_clear_callback(aic_button_t btn);

/**
 * @brief Check if button was pressed since last check (edge detection)
 * @param btn Button identifier
 * @return true if button was pressed, false otherwise
 * @note This function clears the pressed flag after reading
 */
bool aic_gpio_button_was_pressed(aic_button_t btn);

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get LED name string for display
 * @param led LED identifier
 * @return LED name string (e.g., "Red", "Green", "Blue")
 */
const char* aic_gpio_led_name(aic_led_t led);

/**
 * @brief Get button name string for display
 * @param btn Button identifier
 * @return Button name string (e.g., "User")
 */
const char* aic_gpio_button_name(aic_button_t btn);

/*******************************************************************************
 * Demo/Test Functions
 ******************************************************************************/

/**
 * @brief Run LED blink demo (for testing)
 * @param cycles Number of blink cycles
 * @param delay_ms Delay between toggles in milliseconds
 */
void aic_gpio_led_demo(uint8_t cycles, uint32_t delay_ms);

#endif /* AIC_GPIO_H */
