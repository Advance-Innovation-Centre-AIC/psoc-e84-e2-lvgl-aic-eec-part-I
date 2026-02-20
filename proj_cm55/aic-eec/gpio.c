/*******************************************************************************
 * File Name:   gpio.c
 *
 * Description: AIC-EEC GPIO Hardware Abstraction Layer Implementation
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 * Note: This implementation provides both hardware GPIO control and
 *       software simulation for UI development without hardware.
 *
 ******************************************************************************/

#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Include PSoC HAL - always include for CM55 builds */
#include "cybsp.h"
#include "cy_gpio.h"
#include "cy_tcpwm_pwm.h"
#include "cy_sysint.h"
#include "cycfg_peripherals.h"       /* For CYBSP_PWM_LED_CTRL_* */
#include "cycfg_peripheral_clocks.h"  /* For PWM clock divider */

/* Check if hardware GPIO is available by checking BSP LED definitions */
#if defined(CYBSP_USER_LED1_PORT) && defined(CYBSP_USER_LED1_PIN)
#define HW_GPIO_AVAILABLE 1
#else
#define HW_GPIO_AVAILABLE 0
#endif

/*******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* Debounce time in milliseconds */
#define BUTTON_DEBOUNCE_MS      (50U)

/* PWM default frequency */
#define PWM_DEFAULT_FREQ_HZ     (1000U)

/* PWM period (for 100 brightness levels) */
#define PWM_PERIOD              (100U)

#if HW_GPIO_AVAILABLE && defined(CYBSP_PWM_LED_CTRL_HW)
/* PWM initialized flag (only used when hardware PWM is configured) */
static bool pwm_initialized = false;
#endif

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* LED state tracking (software state) */
static bool led_states[AIC_LED_COUNT] = {false, false, false};

/* PWM brightness tracking */
static uint8_t pwm_brightness[AIC_LED_COUNT] = {0, 0, 0};

/* Button state tracking */
static bool button_pressed_flag[AIC_BTN_COUNT] = {false};
static bool last_button_state[AIC_BTN_COUNT] = {false};

/* Button callbacks */
static aic_button_callback_t button_callbacks[AIC_BTN_COUNT] = {NULL};

/* Initialization flag */
static bool gpio_initialized = false;

/* LED names for display */
static const char* led_names[AIC_LED_COUNT] = {"Red", "Green", "Blue"};

/* Button names for display */
static const char* button_names[AIC_BTN_COUNT] = {"User Button 1", "User Button 2"};

/*******************************************************************************
 * Hardware Pin Mapping (PSoC Edge E84)
 ******************************************************************************/

#if HW_GPIO_AVAILABLE

/* LED Pin Mapping */
/* Note: Update these based on actual BSP configuration */
static GPIO_PRT_Type* const led_ports[AIC_LED_COUNT] = {
    CYBSP_USER_LED1_PORT,   /* Red LED - P19_2 */
    CYBSP_USER_LED2_PORT,   /* Green LED - P19_3 */
    CYBSP_USER_LED3_PORT    /* Blue LED - P16_5 (PWM) */
};

static const uint32_t led_pins[AIC_LED_COUNT] = {
    CYBSP_USER_LED1_PIN,    /* Red LED */
    CYBSP_USER_LED2_PIN,    /* Green LED */
    CYBSP_USER_LED3_PIN     /* Blue LED */
};

/* Button Pin Mapping */
static GPIO_PRT_Type* const btn_ports[AIC_BTN_COUNT] = {
    CYBSP_USER_BTN_PORT,    /* User Button 1 (SW2) - P8_3 */
    CYBSP_USER_BTN2_PORT    /* User Button 2 (SW4) - P8_7 */
};

static const uint32_t btn_pins[AIC_BTN_COUNT] = {
    CYBSP_USER_BTN_PIN,     /* User Button 1 */
    CYBSP_USER_BTN2_PIN     /* User Button 2 */
};

#endif /* HW_GPIO_AVAILABLE */

/*******************************************************************************
 * GPIO Initialization
 ******************************************************************************/

bool aic_gpio_init(void)
{
    if (gpio_initialized) {
        return true;
    }

    /* Initialize state arrays */
    memset(led_states, 0, sizeof(led_states));
    memset(pwm_brightness, 0, sizeof(pwm_brightness));
    memset(button_pressed_flag, 0, sizeof(button_pressed_flag));
    memset(last_button_state, 0, sizeof(last_button_state));
    memset(button_callbacks, 0, sizeof(button_callbacks));

#if HW_GPIO_AVAILABLE
    /* Hardware GPIO initialization */
    /* Note: cybsp_init() should be called before this function */

    printf("[GPIO] Hardware GPIO init starting...\r\n");
    printf("[GPIO] LED1 (Red):   Port=%p, Pin=%u\r\n",
           (void*)led_ports[0], (unsigned int)led_pins[0]);
    printf("[GPIO] LED2 (Green): Port=%p, Pin=%u\r\n",
           (void*)led_ports[1], (unsigned int)led_pins[1]);
    printf("[GPIO] LED3 (Blue):  Port=%p, Pin=%u\r\n",
           (void*)led_ports[2], (unsigned int)led_pins[2]);
    printf("[GPIO] LED_STATE_ON=%u, LED_STATE_OFF=%u\r\n",
           (unsigned int)CYBSP_LED_STATE_ON, (unsigned int)CYBSP_LED_STATE_OFF);

    /* Turn off all LEDs initially */
    for (int i = 0; i < AIC_LED_COUNT; i++) {
        Cy_GPIO_Write(led_ports[i], led_pins[i], CYBSP_LED_STATE_OFF);
    }

    /* Quick LED test - blink all LEDs once to verify they work */
    printf("[GPIO] Testing LEDs - should blink once...\r\n");
    for (int i = 0; i < AIC_LED_COUNT; i++) {
        Cy_GPIO_Write(led_ports[i], led_pins[i], CYBSP_LED_STATE_ON);
    }
    Cy_SysLib_Delay(200);  /* 200ms on */
    for (int i = 0; i < AIC_LED_COUNT; i++) {
        Cy_GPIO_Write(led_ports[i], led_pins[i], CYBSP_LED_STATE_OFF);
    }
    Cy_SysLib_Delay(200);  /* 200ms off */
    printf("[GPIO] LED test complete\r\n");

    printf("[GPIO] Hardware GPIO initialized\r\n");
#else
    printf("[GPIO] Software simulation mode (no hardware)\r\n");
#endif

    gpio_initialized = true;
    return true;
}

void aic_gpio_deinit(void)
{
    /* Turn off all LEDs */
    aic_gpio_led_all_off();

    /* Clear callbacks */
    for (int i = 0; i < AIC_BTN_COUNT; i++) {
        button_callbacks[i] = NULL;
    }

    gpio_initialized = false;
}

/*******************************************************************************
 * LED Control Functions
 ******************************************************************************/

void aic_gpio_led_set(aic_led_t led, bool state)
{
    if (led >= AIC_LED_COUNT) {
        return;
    }

    led_states[led] = state;

#if HW_GPIO_AVAILABLE
    /* Write to hardware GPIO */
    Cy_GPIO_Write(led_ports[led], led_pins[led],
                  state ? CYBSP_LED_STATE_ON : CYBSP_LED_STATE_OFF);
#endif

    /* Debug output */
    printf("[GPIO] LED %s: %s\r\n", led_names[led], state ? "ON" : "OFF");
}

void aic_gpio_led_toggle(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return;
    }

    aic_gpio_led_set(led, !led_states[led]);
}

bool aic_gpio_led_get(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return false;
    }

    return led_states[led];
}

void aic_gpio_led_all_on(void)
{
    for (int i = 0; i < AIC_LED_COUNT; i++) {
        aic_gpio_led_set((aic_led_t)i, true);
    }
}

void aic_gpio_led_all_off(void)
{
    for (int i = 0; i < AIC_LED_COUNT; i++) {
        aic_gpio_led_set((aic_led_t)i, false);
    }
}

/*******************************************************************************
 * PWM LED Control Functions
 ******************************************************************************/

bool aic_gpio_pwm_init(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return false;
    }

    /* Only LED_BLUE supports hardware PWM on this kit */
    if (led != AIC_LED_BLUE) {
        printf("[GPIO] Warning: LED %s does not support hardware PWM\r\n",
               led_names[led]);
        /* Still allow software brightness tracking */
    }

#if HW_GPIO_AVAILABLE
    /*
     * Hardware PWM initialization using TCPWM
     *
     * Reference: PSOC_Edge_PWM_Square_Wave/proj_cm33_ns/main.c
     *   Cy_TCPWM_PWM_Init(CYBSP_PWM_LED_CTRL_HW,
     *                     CYBSP_PWM_LED_CTRL_NUM,
     *                     &CYBSP_PWM_LED_CTRL_config);
     *   Cy_TCPWM_PWM_Enable(CYBSP_PWM_LED_CTRL_HW, CYBSP_PWM_LED_CTRL_NUM);
     *   Cy_TCPWM_TriggerStart_Single(CYBSP_PWM_LED_CTRL_HW, CYBSP_PWM_LED_CTRL_NUM);
     *
     * Note: PWM requires TCPWM configuration in Device Configurator.
     * The CYBSP_PWM_LED_CTRL_* macros must be defined in the BSP.
     * If PWM is not configured, we use GPIO on/off as fallback.
     */
#ifdef CYBSP_PWM_LED_CTRL_HW
    if (led == AIC_LED_BLUE) {
        /* Change PWM clock divider to get ~1kHz PWM frequency for smooth dimming
         * Default BSP config: divider=50000 gives ~2Hz (too slow, visible blinking)
         * New config: divider=100 gives ~1kHz (smooth dimming)
         */
        Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)CYBSP_PWM_LED_CTRL_CLK_DIV_GRP_NUM,
                                         CY_SYSCLK_DIV_16_BIT, CYBSP_PWM_LED_CTRL_CLK_DIV_NUM);
        Cy_SysClk_PeriPclkSetDivider((en_clk_dst_t)CYBSP_PWM_LED_CTRL_CLK_DIV_GRP_NUM,
                                     CY_SYSCLK_DIV_16_BIT, CYBSP_PWM_LED_CTRL_CLK_DIV_NUM, 99U);
        Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)CYBSP_PWM_LED_CTRL_CLK_DIV_GRP_NUM,
                                        CY_SYSCLK_DIV_16_BIT, CYBSP_PWM_LED_CTRL_CLK_DIV_NUM);

        cy_en_tcpwm_status_t result = Cy_TCPWM_PWM_Init(CYBSP_PWM_LED_CTRL_HW,
                                                        CYBSP_PWM_LED_CTRL_NUM,
                                                        &CYBSP_PWM_LED_CTRL_config);
        if (CY_TCPWM_SUCCESS == result) {
            Cy_TCPWM_PWM_Enable(CYBSP_PWM_LED_CTRL_HW, CYBSP_PWM_LED_CTRL_NUM);
            Cy_TCPWM_TriggerStart_Single(CYBSP_PWM_LED_CTRL_HW, CYBSP_PWM_LED_CTRL_NUM);
            pwm_initialized = true;
            printf("[GPIO] Hardware PWM initialized for LED %s (1kHz)\r\n", led_names[led]);
        } else {
            printf("[GPIO] PWM init failed, using GPIO fallback\r\n");
        }
    }
#else
    /* PWM not configured in BSP, use GPIO on/off fallback */
    printf("[GPIO] PWM not configured in BSP, using GPIO fallback\r\n");
#endif
#endif

    pwm_brightness[led] = 0;
    printf("[GPIO] PWM initialized for LED %s\r\n", led_names[led]);
    return true;
}

void aic_gpio_pwm_set_brightness(aic_led_t led, uint8_t brightness)
{
    if (led >= AIC_LED_COUNT) {
        return;
    }

    /* Clamp brightness to 0-100 */
    if (brightness > 100) {
        brightness = 100;
    }

    pwm_brightness[led] = brightness;

#if HW_GPIO_AVAILABLE
    /*
     * Set PWM duty cycle using TCPWM
     *
     * The compare value determines the duty cycle:
     *   duty_cycle = compare / period * 100%
     *
     * For brightness control:
     *   compare = (brightness * period) / 100
     */
#ifdef CYBSP_PWM_LED_CTRL_HW
    if (led == AIC_LED_BLUE && pwm_initialized) {
        /* Get current period and calculate compare value */
        uint32_t period = Cy_TCPWM_PWM_GetPeriod0(CYBSP_PWM_LED_CTRL_HW,
                                                   CYBSP_PWM_LED_CTRL_NUM);
        uint32_t compare = (brightness * period) / 100;
        Cy_TCPWM_PWM_SetCompare0(CYBSP_PWM_LED_CTRL_HW,
                                 CYBSP_PWM_LED_CTRL_NUM,
                                 compare);
    } else {
        /* Fallback: Use on/off for non-PWM capable LEDs or if PWM not init */
        Cy_GPIO_Write(led_ports[led], led_pins[led],
                      brightness > 50 ? CYBSP_LED_STATE_ON : CYBSP_LED_STATE_OFF);
    }
#else
    /* PWM not configured, use GPIO on/off as fallback */
    Cy_GPIO_Write(led_ports[led], led_pins[led],
                  brightness > 50 ? CYBSP_LED_STATE_ON : CYBSP_LED_STATE_OFF);
#endif
#endif

    /* Update LED state tracking */
    led_states[led] = (brightness > 0);
}

uint8_t aic_gpio_pwm_get_brightness(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return 0;
    }

    return pwm_brightness[led];
}

void aic_gpio_pwm_set_frequency(aic_led_t led, uint32_t freq_hz)
{
    if (led >= AIC_LED_COUNT || freq_hz == 0) {
        return;
    }

#if HW_GPIO_AVAILABLE
#ifdef CYBSP_PWM_LED_CTRL_HW
    if (led == AIC_LED_BLUE && pwm_initialized) {
        /*
         * Set PWM frequency by changing period
         * Note: This requires knowing the TCPWM clock frequency.
         * For proper implementation, get clock freq from BSP/Device Configurator.
         *
         * period = clock_freq / freq_hz
         *
         * After changing period, recalculate compare value to maintain
         * the same brightness percentage.
         */
        /* Implementation depends on clock configuration */
        (void)freq_hz;  /* Suppress unused warning for now */
    }
#else
    (void)freq_hz;  /* PWM not available */
#endif
#else
    (void)freq_hz;  /* Suppress unused warning */
#endif

    printf("[GPIO] LED %s PWM frequency: %u Hz\r\n", led_names[led],
           (unsigned int)freq_hz);
}

void aic_gpio_pwm_deinit(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return;
    }

#if HW_GPIO_AVAILABLE
#ifdef CYBSP_PWM_LED_CTRL_HW
    if (led == AIC_LED_BLUE && pwm_initialized) {
        /* Disable PWM counter */
        Cy_TCPWM_PWM_Disable(CYBSP_PWM_LED_CTRL_HW, CYBSP_PWM_LED_CTRL_NUM);
        pwm_initialized = false;
    }
#endif
    /* Turn off the LED */
    Cy_GPIO_Write(led_ports[led], led_pins[led], CYBSP_LED_STATE_OFF);
#endif

    pwm_brightness[led] = 0;
    led_states[led] = false;
    printf("[GPIO] PWM deinitialized for LED %s\r\n", led_names[led]);
}

/*******************************************************************************
 * Button Input Functions
 ******************************************************************************/

bool aic_gpio_button_read(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return false;
    }

    bool current_state = false;

#if HW_GPIO_AVAILABLE
    /* Read hardware button (active low typically) */
    current_state = (Cy_GPIO_Read(btn_ports[btn], btn_pins[btn]) == 0);

    /* Simple software debounce */
    /* For more robust debouncing, use timer-based approach */
    if (current_state != last_button_state[btn]) {
        Cy_SysLib_Delay(BUTTON_DEBOUNCE_MS);
        current_state = (Cy_GPIO_Read(btn_ports[btn], btn_pins[btn]) == 0);
    }
#endif

    last_button_state[btn] = current_state;
    return current_state;
}

bool aic_gpio_button_read_raw(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return false;
    }

#if HW_GPIO_AVAILABLE
    return (Cy_GPIO_Read(btn_ports[btn], btn_pins[btn]) == 0);
#else
    return false;
#endif
}

bool aic_gpio_button_set_callback(aic_button_t btn, aic_button_callback_t callback)
{
    if (btn >= AIC_BTN_COUNT) {
        return false;
    }

    button_callbacks[btn] = callback;

#if HW_GPIO_AVAILABLE
    /* Configure GPIO interrupt for button */
    /* This requires interrupt setup in Device Configurator */
    /* Example:
     * cy_stc_sysint_t intrCfg = {
     *     .intrSrc = CYBSP_USER_BTN_IRQ,
     *     .intrPriority = 7U
     * };
     * Cy_SysInt_Init(&intrCfg, button_interrupt_handler);
     * NVIC_EnableIRQ(intrCfg.intrSrc);
     */
#endif

    printf("[GPIO] Callback set for button %s\r\n", button_names[btn]);
    return true;
}

void aic_gpio_button_clear_callback(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return;
    }

    button_callbacks[btn] = NULL;
    printf("[GPIO] Callback cleared for button %s\r\n", button_names[btn]);
}

bool aic_gpio_button_was_pressed(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return false;
    }

    bool was_pressed = button_pressed_flag[btn];
    button_pressed_flag[btn] = false;  /* Clear flag after reading */
    return was_pressed;
}

/* Internal function to be called from ISR */
void aic_gpio_button_isr_handler(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return;
    }

    button_pressed_flag[btn] = true;

    /* Call registered callback */
    if (button_callbacks[btn] != NULL) {
        button_callbacks[btn]();
    }
}

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

const char* aic_gpio_led_name(aic_led_t led)
{
    if (led >= AIC_LED_COUNT) {
        return "Unknown";
    }
    return led_names[led];
}

const char* aic_gpio_button_name(aic_button_t btn)
{
    if (btn >= AIC_BTN_COUNT) {
        return "Unknown";
    }
    return button_names[btn];
}

/*******************************************************************************
 * Demo/Test Functions
 ******************************************************************************/

void aic_gpio_led_demo(uint8_t cycles, uint32_t delay_ms)
{
    printf("[GPIO] Running LED demo: %u cycles, %u ms delay\r\n",
           cycles, (unsigned int)delay_ms);

    for (uint8_t c = 0; c < cycles; c++) {
        /* Cycle through each LED */
        for (int i = 0; i < AIC_LED_COUNT; i++) {
            aic_gpio_led_set((aic_led_t)i, true);

#if HW_GPIO_AVAILABLE
            Cy_SysLib_Delay(delay_ms);
#endif

            aic_gpio_led_set((aic_led_t)i, false);
        }
    }

    printf("[GPIO] LED demo complete\r\n");
}

/* [] END OF FILE */
