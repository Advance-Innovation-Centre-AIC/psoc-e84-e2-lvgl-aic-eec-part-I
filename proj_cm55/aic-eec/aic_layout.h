/*******************************************************************************
 * File: aic_layout.h
 * Description: AIC-EEC UI Layout Helpers
 *
 * Flexbox-style layout helpers for LVGL to reduce boilerplate code.
 * Provides consistent spacing, alignment, and common UI patterns.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#ifndef AIC_LAYOUT_H
#define AIC_LAYOUT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Branding / Copyright Configuration
 ******************************************************************************/

#define AIC_COPYRIGHT_TEXT   "(C) 2023-2026 AIC-EEC.com and BiiL Centre, Burapha University"
#define AIC_COPYRIGHT_COLOR  lv_color_hex(0x666666)
#define AIC_FOOTER_Y_OFFSET  (-10)

/*******************************************************************************
 * Default Styling
 ******************************************************************************/

#define AIC_PAD_DEFAULT         (10)
#define AIC_GAP_DEFAULT         (8)
#define AIC_RADIUS_DEFAULT      (8)
#define AIC_BORDER_WIDTH        (1)

/* Color Palette */
#define AIC_COLOR_BG_DARK       lv_color_hex(0x16213e)
#define AIC_COLOR_BG_CARD       lv_color_hex(0x1f4068)
#define AIC_COLOR_PRIMARY       lv_color_hex(0x00d4ff)
#define AIC_COLOR_SECONDARY     lv_color_hex(0xff6b6b)
#define AIC_COLOR_SUCCESS       lv_color_hex(0x4ade80)
#define AIC_COLOR_WARNING       lv_color_hex(0xfbbf24)
#define AIC_COLOR_ERROR         lv_color_hex(0xef4444)
#define AIC_COLOR_TEXT          lv_color_hex(0xffffff)
#define AIC_COLOR_TEXT_DIM      lv_color_hex(0x94a3b8)

/*******************************************************************************
 * Layout Containers
 ******************************************************************************/

/**
 * @brief Create horizontal flex row
 * @param parent Parent object
 * @return Row container object
 *
 * Creates a container with:
 * - Horizontal flex layout
 * - Default gap between children
 * - Full width, content height
 */
lv_obj_t *aic_row_create(lv_obj_t *parent);

/**
 * @brief Create vertical flex column
 * @param parent Parent object
 * @return Column container object
 *
 * Creates a container with:
 * - Vertical flex layout
 * - Default gap between children
 * - Full width, content height
 */
lv_obj_t *aic_col_create(lv_obj_t *parent);

/**
 * @brief Create flexible spacer
 * @param parent Parent object (should be flex container)
 * @return Spacer object
 *
 * Grows to fill available space in flex container.
 */
lv_obj_t *aic_spacer_create(lv_obj_t *parent);

/**
 * @brief Create centered container
 * @param parent Parent object
 * @return Centered container
 *
 * Children are centered both horizontally and vertically.
 */
lv_obj_t *aic_center_create(lv_obj_t *parent);

/*******************************************************************************
 * Layout Properties
 ******************************************************************************/

/**
 * @brief Set flex grow factor
 * @param obj Object to modify
 * @param grow Grow factor (1 = grow, 0 = don't grow)
 */
void aic_flex_grow(lv_obj_t *obj, uint8_t grow);

/**
 * @brief Set padding (all sides)
 * @param obj Object to modify
 * @param pad Padding in pixels
 */
void aic_pad(lv_obj_t *obj, int32_t pad);

/**
 * @brief Set padding (individual sides)
 * @param obj Object to modify
 * @param top Top padding
 * @param right Right padding
 * @param bottom Bottom padding
 * @param left Left padding
 */
void aic_pad_all(lv_obj_t *obj, int32_t top, int32_t right, int32_t bottom, int32_t left);

/**
 * @brief Set gap between children
 * @param obj Container object
 * @param gap Gap in pixels
 */
void aic_gap(lv_obj_t *obj, int32_t gap);

/**
 * @brief Set object to full width of parent
 * @param obj Object to modify
 */
void aic_full_width(lv_obj_t *obj);

/**
 * @brief Set object to full height of parent
 * @param obj Object to modify
 */
void aic_full_height(lv_obj_t *obj);

/**
 * @brief Set object to full size of parent
 * @param obj Object to modify
 */
void aic_full_size(lv_obj_t *obj);

/*******************************************************************************
 * Common Layouts
 ******************************************************************************/

/**
 * @brief Create card container with title
 * @param parent Parent object
 * @param title Card title (NULL for no title)
 * @return Card content area (add children here)
 *
 * Creates a styled card with:
 * - Background color
 * - Rounded corners
 * - Title label
 * - Content area with padding
 */
lv_obj_t *aic_card_create(lv_obj_t *parent, const char *title);

/**
 * @brief Create labeled value display
 * @param parent Parent object
 * @param label Label text
 * @return Value label object (use lv_label_set_text to update)
 *
 * Creates a row with label and value:
 * [Label:] [Value]
 */
lv_obj_t *aic_value_display_create(lv_obj_t *parent, const char *label);

/**
 * @brief Create status indicator
 * @param parent Parent object
 * @param label Indicator label
 * @param initial_state Initial state (true = on, false = off)
 * @return LED indicator object
 */
lv_obj_t *aic_status_indicator_create(lv_obj_t *parent, const char *label, bool initial_state);

/**
 * @brief Create button with icon and text
 * @param parent Parent object
 * @param icon Icon symbol (LV_SYMBOL_xxx or NULL)
 * @param text Button text
 * @return Button object
 */
lv_obj_t *aic_icon_button_create(lv_obj_t *parent, const char *icon, const char *text);

/*******************************************************************************
 * Section Helpers
 ******************************************************************************/

/**
 * @brief Create section with title
 * @param parent Parent object
 * @param title Section title
 * @return Section content area
 */
lv_obj_t *aic_section_create(lv_obj_t *parent, const char *title);

/**
 * @brief Create horizontal divider
 * @param parent Parent object
 * @return Divider object
 */
lv_obj_t *aic_divider_create(lv_obj_t *parent);

/*******************************************************************************
 * Data Display Helpers
 ******************************************************************************/

/**
 * @brief Create gauge display with label
 * @param parent Parent object
 * @param label Label text
 * @param min Minimum value
 * @param max Maximum value
 * @param initial Initial value
 * @return Arc object for gauge
 */
lv_obj_t *aic_gauge_create(lv_obj_t *parent, const char *label, int32_t min, int32_t max, int32_t initial);

/**
 * @brief Create progress bar with label
 * @param parent Parent object
 * @param label Label text
 * @return Bar object
 */
lv_obj_t *aic_progress_bar_create(lv_obj_t *parent, const char *label);

/**
 * @brief Create XYZ display for IMU data
 * @param parent Parent object
 * @param title Title text
 * @param labels Array of 3 label pointers to receive label objects
 * @return Container object
 */
lv_obj_t *aic_xyz_display_create(lv_obj_t *parent, const char *title, lv_obj_t **labels);

/*******************************************************************************
 * Screen Helpers
 ******************************************************************************/

/**
 * @brief Apply dark theme to screen
 * @param scr Screen object (NULL for active screen)
 */
void aic_apply_dark_theme(lv_obj_t *scr);

/**
 * @brief Create footer with default copyright text
 * @param parent Parent object
 * @return Footer label object
 */
lv_obj_t *aic_create_footer(lv_obj_t *parent);

/**
 * @brief Create footer with custom text and color
 * @param parent Parent object
 * @param text Custom text to display
 * @param color Text color
 * @return Footer label object
 */
lv_obj_t *aic_create_footer_custom(lv_obj_t *parent, const char *text, lv_color_t color);

/**
 * @brief Create header with title
 * @param parent Parent object
 * @param title Header title
 * @return Header object
 */
lv_obj_t *aic_create_header(lv_obj_t *parent, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* AIC_LAYOUT_H */
