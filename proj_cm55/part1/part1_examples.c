/*******************************************************************************
 * File Name:   part1_examples.c
 *
 * Description: Part 1 LVGL Examples - GPIO Integration
 *              Part of Embedded C for IoT Course
 *
 * Learning Objectives:
 *   - Basic LVGL widgets (Label, Button, LED, Switch)
 *   - Event handling patterns
 *   - GPIO integration concepts
 *
 * LVGL Version: v9.2.0
 *
 * =============================================================================
 * SCREEN & COORDINATE SYSTEM (PSoC Edge E84 Eval Kit)
 * =============================================================================
 *
 *   Physical Display: 480 x 800 pixels (Vertical/Portrait)
 *   Usable Area:      480 x 320 pixels
 *   Origin (0,0):     Top-Left Corner
 *
 *       X=0           X=240          X=479
 *   Y=0  +-----------------------------+
 *        | (0,0)                       |
 *        |         X+ -------->        |
 *        |         |                   |
 *   Y=160|         | Y+    * CENTER    |
 *        |         v     (240,160)     |
 *        |                             |
 *   Y=319+-----------------------------+
 *
 * =============================================================================
 * ALIGNMENT TYPES (LV_ALIGN_*)
 * =============================================================================
 *
 *   +---------------------------------------------+
 *   | TOP_LEFT    TOP_MID      TOP_RIGHT         |  y_offset: +20 (title)
 *   |    *          *             *              |
 *   |                                            |
 *   | LEFT_MID    CENTER       RIGHT_MID         |  y_offset: 0 (main)
 *   |    *          *             *              |
 *   |                                            |
 *   | BOTTOM_LEFT BOTTOM_MID   BOTTOM_RIGHT      |  y_offset: -50 (desc)
 *   |    *          *             *              |
 *   +---------------------------------------------+
 *
 * =============================================================================
 * COMMON PATTERNS USED IN THIS FILE
 * =============================================================================
 *
 *   Pattern 1: Vertical Stacking from CENTER
 *   -----------------------------------------
 *   - Title:       LV_ALIGN_TOP_MID,    y = +20
 *   - Main Widget: LV_ALIGN_CENTER,     y = -40 to -70
 *   - Sub Widget:  LV_ALIGN_CENTER,     y = 0 to +30
 *   - Buttons:     LV_ALIGN_CENTER,     y = +50 to +90
 *   - Slider:      LV_ALIGN_CENTER,     y = +110 to +160
 *   - Description: LV_ALIGN_BOTTOM_MID, y = -30 to -50
 *
 *   Pattern 2: Button Creation Order (IMPORTANT!)
 *   ----------------------------------------------
 *   1. lv_button_create()
 *   2. lv_obj_add_event_cb()
 *   3. lv_obj_align() or lv_obj_set_pos()
 *   4. lv_obj_set_style_pad_*() -- use instead of lv_obj_set_size()
 *   5. lv_label_create(btn) -- create label LAST
 *   6. lv_obj_center(label)
 *
 *   Pattern 3: Container for Grid Layout
 *   -------------------------------------
 *   - Container size: 420 x 200 pixels
 *   - Position: LV_ALIGN_CENTER, y = 0 to +10
 *   - Grid: 2 columns x 2 rows, x_spacing = 200, y_spacing = 80
 *
 * Official LVGL Documentation and Example References:
 *   - Label:  https://docs.lvgl.io/9.2/widgets/label.html
 *             https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/label/lv_example_label_1.c
 *   - Button: https://docs.lvgl.io/9.2/widgets/button.html
 *             https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/button/lv_example_button_1.c
 *   - LED:    https://docs.lvgl.io/9.2/widgets/led.html
 *             https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/led/lv_example_led_1.c
 *   - Switch: https://docs.lvgl.io/9.2/widgets/switch.html
 *             https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/switch/lv_example_switch_1.c
 *   - Slider: https://docs.lvgl.io/9.2/widgets/slider.html
 *             https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/slider/lv_example_slider_1.c
 *
 ******************************************************************************/

#include "part1_examples.h"
#include "aic-eec/aic-eec.h"
#include <stdio.h>

/*******************************************************************************
 * Example 1: Hello World - Basic Label
 *
 * Official Reference: https://docs.lvgl.io/9.2/widgets/label.html
 * Example Code:       https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/label/lv_example_label_1.c
 *
 * WHY: Label is the most fundamental widget for displaying text
 * HOW: Used to show sensor values, status messages, debug output
 * CAUTION: Long text may overflow - use lv_label_set_long_mode() if needed
 ******************************************************************************/
void part1_ex1_hello_world(void)
{
    /* Set background color */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x003a57),
                              LV_PART_MAIN);

    /* Create title label */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 - Example 1");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Create main label */
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello BUU!");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /* Create description label */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "Basic Label Example\n"
                           "Learning: lv_label_create, lv_obj_align");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -50);

    /* Footer */
    aic_create_footer(lv_screen_active());
}

/*******************************************************************************
 * Example 2: Button with Click Counter
 *
 * Official Reference: https://docs.lvgl.io/9.2/widgets/button.html
 * Example Code:       https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/button/lv_example_button_1.c
 *
 * WHY: Button is the primary input widget for user interaction
 * HOW: Used to trigger actions like GPIO ON/OFF, start/stop operations
 * CAUTION: Always filter event code in callback (check for LV_EVENT_CLICKED)
 *
 * IMPORTANT Pattern (from official example):
 *   1. lv_button_create()
 *   2. lv_obj_add_event_cb()
 *   3. lv_obj_align() or lv_obj_center()
 *   4. Create label child
 *   5. Do NOT use lv_obj_set_size() before adding label (causes text clipping)
 ******************************************************************************/
static void ex2_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        static uint32_t cnt = 0;
        cnt++;

        /* Get the label child and update text */
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Clicked: %u", (unsigned int)cnt);

        printf("Button clicked %u times\r\n", (unsigned int)cnt);
    }
}

void part1_ex2_button_counter(void)
{
    /* Set background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x1a1a2e),
                              LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 - Example 2: Button Counter");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Create button - following official LVGL example pattern */
    lv_obj_t * btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn, ex2_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_hor(btn, 30, 0);
    lv_obj_set_style_pad_ver(btn, 15, 0);

    /* Add label to button */
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);

    /* Description */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "Learning: lv_button_create, lv_obj_add_event_cb\n"
                           "Pattern: Event callback with LV_EVENT_CLICKED");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -50);

    /* Footer */
    aic_create_footer(lv_screen_active());
}

/*******************************************************************************
 * Example 3: LED Widget Control
 *
 * Official Reference: https://docs.lvgl.io/9.2/widgets/led.html
 * Example Code:       https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/led/lv_example_led_1.c
 * Slider Reference:   https://docs.lvgl.io/9.2/widgets/slider.html
 * Slider Example:     https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/slider/lv_example_slider_1.c
 *
 * WHY: LED widget clearly shows ON/OFF status visually
 * HOW: Used to display GPIO state, sensor status, system indicators
 * CAUTION: This is a VIRTUAL LED (UI only) - not the physical LED on board
 ******************************************************************************/
static lv_obj_t * ex3_led;
static lv_obj_t * ex3_brightness_label;
static lv_obj_t * slider_label;

static void ex3_on_btn_cb(lv_event_t * e)
{
    (void)e;
    lv_led_on(ex3_led);
    lv_label_set_text(ex3_brightness_label, "Brightness: 255 (ON)");
}

static void ex3_off_btn_cb(lv_event_t * e)
{
    (void)e;
    lv_led_off(ex3_led);
    lv_label_set_text(ex3_brightness_label, "Brightness: 0 (OFF)");
}

static void ex3_slider_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    /* Update percentage label (0-255 -> 0-100%) */
    int percent = (value * 100) / 255;
    lv_label_set_text_fmt(slider_label, "%d%%", percent);

    lv_led_set_brightness(ex3_led, (uint8_t)value);
    lv_label_set_text_fmt(ex3_brightness_label, "Brightness: %d", (int)value);
}

void part1_ex3_led_control(void)
{
    /* Background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x0f0f23),
                              LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 - Example 3: LED Widget");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Create LED widget */
    ex3_led = lv_led_create(lv_screen_active());
    lv_obj_set_size(ex3_led, 80, 80);
    lv_obj_align(ex3_led, LV_ALIGN_CENTER, 0, -70);  /* Moved up from -40 */
    lv_led_set_color(ex3_led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_brightness(ex3_led, 150);

    /* Brightness label */
    ex3_brightness_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ex3_brightness_label, "Brightness: 150");
    lv_obj_set_style_text_color(ex3_brightness_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ex3_brightness_label, LV_ALIGN_CENTER, 0, 0);  /* Moved up from 30 */

    /* ON Button - following official pattern: create -> event -> align -> label */
    lv_obj_t * btn_on = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_on, ex3_on_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, -60, 50);  /* Moved up from 90 */
    lv_obj_set_style_bg_color(btn_on, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_pad_hor(btn_on, 30, 0);
    lv_obj_set_style_pad_ver(btn_on, 15, 0);

    lv_obj_t * label_on = lv_label_create(btn_on);
    lv_label_set_text(label_on, "ON");
    lv_obj_center(label_on);

    /* OFF Button - following official pattern: create -> event -> align -> label */
    lv_obj_t * btn_off = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_off, ex3_off_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 60, 50);  /* Moved up from 90 */
    lv_obj_set_style_bg_color(btn_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_pad_hor(btn_off, 30, 0);
    lv_obj_set_style_pad_ver(btn_off, 15, 0);

    lv_obj_t * label_off = lv_label_create(btn_off);
    lv_label_set_text(label_off, "OFF");
    lv_obj_center(label_off);

    /* Brightness Slider - align FIRST, then create label below */
    lv_obj_t * slider = lv_slider_create(lv_screen_active());
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 110);  /* Moved up from 160, align BEFORE label */
    lv_slider_set_range(slider, 0, 255);
    lv_slider_set_value(slider, 150, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, ex3_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Slider percentage label - create AFTER slider is positioned */
    slider_label = lv_label_create(lv_screen_active());
    lv_label_set_text(slider_label, "59%");  /* 150/255 = 59% */
    lv_obj_set_style_text_color(slider_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /* Description */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "Learning: lv_led_create, lv_led_on/off, lv_led_set_brightness");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -30);

    /* Footer */
    aic_create_footer(lv_screen_active());
}

/*******************************************************************************
 * Example 4: Switch Widget (ON/OFF Toggle)
 *
 * Official Reference: https://docs.lvgl.io/9.2/widgets/switch.html
 * Example Code:       https://github.com/lvgl/lvgl/blob/release/v9.2/examples/widgets/switch/lv_example_switch_1.c
 *
 * WHY: Switch is ideal for controlling binary states (ON/OFF)
 * HOW: Toggle GPIO outputs, enable/disable features
 * CAUTION: Use lv_obj_has_state(sw, LV_STATE_CHECKED) to check current state
 ******************************************************************************/
static lv_obj_t * ex4_status_label;
static lv_obj_t * ex4_led;

static void ex4_switch_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    bool is_checked = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if(is_checked) {
        lv_label_set_text(ex4_status_label, "GPIO State: HIGH (ON)");
        lv_led_on(ex4_led);
        printf("Switch ON - GPIO would be HIGH\r\n");
    } else {
        lv_label_set_text(ex4_status_label, "GPIO State: LOW (OFF)");
        lv_led_off(ex4_led);
        printf("Switch OFF - GPIO would be LOW\r\n");
    }
}

void part1_ex4_switch_toggle(void)
{
    /* Background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x16213e),
                              LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 - Example 4: Switch Control");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* LED indicator */
    ex4_led = lv_led_create(lv_screen_active());
    lv_obj_set_size(ex4_led, 60, 60);
    lv_obj_align(ex4_led, LV_ALIGN_CENTER, 0, -60);
    lv_led_set_color(ex4_led, lv_palette_main(LV_PALETTE_YELLOW));
    lv_led_off(ex4_led);

    /* Label for LED */
    lv_obj_t * led_label = lv_label_create(lv_screen_active());
    lv_label_set_text(led_label, "Virtual LED");
    lv_obj_set_style_text_color(led_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(led_label, ex4_led, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    /* Switch */
    lv_obj_t * sw = lv_switch_create(lv_screen_active());
    lv_obj_set_size(sw, 80, 40);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(sw, ex4_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Status label */
    ex4_status_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ex4_status_label, "GPIO State: LOW (OFF)");
    lv_obj_set_style_text_color(ex4_status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(ex4_status_label, LV_ALIGN_CENTER, 0, 100);

    /* Description */
    lv_obj_t * desc = lv_label_create(lv_screen_active());
    lv_label_set_text(desc, "Learning: lv_switch_create, LV_STATE_CHECKED\n"
                           "This switch would control a real GPIO in actual hardware");
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -45);

    /* Footer */
    aic_create_footer(lv_screen_active());
}

/*******************************************************************************
 * Example 5: GPIO Dashboard
 *
 * Widgets Used:
 *   - LED:    https://docs.lvgl.io/9.2/widgets/led.html
 *   - Switch: https://docs.lvgl.io/9.2/widgets/switch.html
 *   - Button: https://docs.lvgl.io/9.2/widgets/button.html
 *   - Container (Base Object): https://docs.lvgl.io/9.2/widgets/obj.html
 *
 * WHY: Display multiple GPIO states in a single dashboard view
 * HOW: Create a monitoring panel for LEDs, buttons, and other I/O
 * CAUTION: UI updates must be done from LVGL context (not from ISR directly)
 ******************************************************************************/
#define EX5_NUM_GPIOS 4

typedef struct {
    lv_obj_t *led;
    lv_obj_t *sw;
    lv_obj_t *label;
    const char *name;
    bool state;
} gpio_item_t;

static gpio_item_t ex5_gpios[EX5_NUM_GPIOS];

static void ex5_switch_cb(lv_event_t * e)
{
    gpio_item_t * gpio = (gpio_item_t *)lv_event_get_user_data(e);
    lv_obj_t * sw = lv_event_get_target(e);

    gpio->state = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if(gpio->state) {
        lv_led_on(gpio->led);
        printf("%s: ON\r\n", gpio->name);
    } else {
        lv_led_off(gpio->led);
        printf("%s: OFF\r\n", gpio->name);
    }
}

/* All ON button callback */
static void ex5_all_on_cb(lv_event_t * e)
{
    (void)e;
    for(int i = 0; i < EX5_NUM_GPIOS; i++) {
        ex5_gpios[i].state = true;
        lv_led_on(ex5_gpios[i].led);
        lv_obj_add_state(ex5_gpios[i].sw, LV_STATE_CHECKED);
        lv_obj_invalidate(ex5_gpios[i].sw);   /* Force redraw */
        lv_obj_invalidate(ex5_gpios[i].led);  /* Force redraw */
    }
    printf("All LEDs: ON\r\n");
}

/* All OFF button callback */
static void ex5_all_off_cb(lv_event_t * e)
{
    (void)e;
    for(int i = 0; i < EX5_NUM_GPIOS; i++) {
        ex5_gpios[i].state = false;
        lv_led_off(ex5_gpios[i].led);
        lv_obj_clear_state(ex5_gpios[i].sw, LV_STATE_CHECKED);
        lv_obj_invalidate(ex5_gpios[i].sw);   /* Force redraw */
        lv_obj_invalidate(ex5_gpios[i].led);  /* Force redraw */
    }
    printf("All LEDs: OFF\r\n");
}

void part1_ex5_gpio_dashboard(void)
{
    const char *gpio_names[] = {"LED1", "LED2", "LED3", "LED4"};
    lv_color_t gpio_colors[] = {
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_GREEN),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_YELLOW)
    };

    /* Background */
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_hex(0x1a1a2e),
                              LV_PART_MAIN);
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t * title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Part 1 - Example 5: GPIO Dashboard");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Create container - wider and no scroll */
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 420, 200);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x0f0f23), 0);
    lv_obj_set_style_border_width(cont, 2, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0x444444), 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);  /* Disable scrolling */

    /* Create GPIO items */
    for(int i = 0; i < EX5_NUM_GPIOS; i++) {
        ex5_gpios[i].name = gpio_names[i];
        ex5_gpios[i].state = false;

        int x_pos = (i % 2) * 200 + 20;
        int y_pos = (i / 2) * 80 + 20;

        /* LED */
        ex5_gpios[i].led = lv_led_create(cont);
        lv_obj_set_size(ex5_gpios[i].led, 40, 40);
        lv_obj_set_pos(ex5_gpios[i].led, x_pos, y_pos);
        lv_led_set_color(ex5_gpios[i].led, gpio_colors[i]);
        lv_led_off(ex5_gpios[i].led);

        /* Label */
        ex5_gpios[i].label = lv_label_create(cont);
        lv_label_set_text(ex5_gpios[i].label, gpio_names[i]);
        lv_obj_set_style_text_color(ex5_gpios[i].label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align_to(ex5_gpios[i].label, ex5_gpios[i].led,
                        LV_ALIGN_OUT_RIGHT_MID, 10, 0);

        /* Switch */
        ex5_gpios[i].sw = lv_switch_create(cont);
        lv_obj_set_size(ex5_gpios[i].sw, 60, 30);
        lv_obj_align_to(ex5_gpios[i].sw, ex5_gpios[i].label,
                        LV_ALIGN_OUT_RIGHT_MID, 15, 0);
        lv_obj_add_event_cb(ex5_gpios[i].sw, ex5_switch_cb,
                           LV_EVENT_VALUE_CHANGED, &ex5_gpios[i]);
    }

    /* All ON button - following official pattern: create -> event -> align -> label */
    lv_obj_t * btn_all_on = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_all_on, ex5_all_on_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_all_on, LV_ALIGN_BOTTOM_LEFT, 40, -50);
    lv_obj_set_style_bg_color(btn_all_on, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_pad_hor(btn_all_on, 30, 0);
    lv_obj_set_style_pad_ver(btn_all_on, 15, 0);

    lv_obj_t * label_on = lv_label_create(btn_all_on);
    lv_label_set_text(label_on, "All ON");
    lv_obj_center(label_on);

    /* All OFF button - following official pattern: create -> event -> align -> label */
    lv_obj_t * btn_all_off = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_all_off, ex5_all_off_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_all_off, LV_ALIGN_BOTTOM_RIGHT, -40, -50);
    lv_obj_set_style_bg_color(btn_all_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_pad_hor(btn_all_off, 30, 0);
    lv_obj_set_style_pad_ver(btn_all_off, 15, 0);

    lv_obj_t * label_off = lv_label_create(btn_all_off);
    lv_label_set_text(label_off, "All OFF");
    lv_obj_center(label_off);

    /* Footer */
    aic_create_footer(lv_screen_active());
}
