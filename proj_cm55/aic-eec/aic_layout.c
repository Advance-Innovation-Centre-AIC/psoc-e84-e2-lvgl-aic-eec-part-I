/*******************************************************************************
 * File: aic_layout.c
 * Description: AIC-EEC UI Layout Helpers Implementation
 *
 * Flexbox-style layout helpers for LVGL.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#include "aic_layout.h"

/*******************************************************************************
 * Layout Containers
 ******************************************************************************/

lv_obj_t *aic_row_create(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);

    /* Clear default styling */
    lv_obj_remove_style_all(row);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    /* Set flex layout */
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Set size */
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);

    /* Default gap */
    lv_obj_set_style_pad_column(row, AIC_GAP_DEFAULT, 0);

    return row;
}

lv_obj_t *aic_col_create(lv_obj_t *parent)
{
    lv_obj_t *col = lv_obj_create(parent);

    /* Clear default styling */
    lv_obj_remove_style_all(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);

    /* Set flex layout */
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Set size */
    lv_obj_set_width(col, lv_pct(100));
    lv_obj_set_height(col, LV_SIZE_CONTENT);

    /* Default gap */
    lv_obj_set_style_pad_row(col, AIC_GAP_DEFAULT, 0);

    return col;
}

lv_obj_t *aic_spacer_create(lv_obj_t *parent)
{
    lv_obj_t *spacer = lv_obj_create(parent);

    lv_obj_remove_style_all(spacer);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);

    /* Make it grow */
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_height(spacer, 1);

    return spacer;
}

lv_obj_t *aic_center_create(lv_obj_t *parent)
{
    lv_obj_t *center = lv_obj_create(parent);

    lv_obj_remove_style_all(center);
    lv_obj_set_style_bg_opa(center, LV_OPA_TRANSP, 0);

    lv_obj_set_size(center, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(center, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    return center;
}

/*******************************************************************************
 * Layout Properties
 ******************************************************************************/

void aic_flex_grow(lv_obj_t *obj, uint8_t grow)
{
    lv_obj_set_flex_grow(obj, grow);
}

void aic_pad(lv_obj_t *obj, int32_t pad)
{
    lv_obj_set_style_pad_all(obj, pad, 0);
}

void aic_pad_all(lv_obj_t *obj, int32_t top, int32_t right, int32_t bottom, int32_t left)
{
    lv_obj_set_style_pad_top(obj, top, 0);
    lv_obj_set_style_pad_right(obj, right, 0);
    lv_obj_set_style_pad_bottom(obj, bottom, 0);
    lv_obj_set_style_pad_left(obj, left, 0);
}

void aic_gap(lv_obj_t *obj, int32_t gap)
{
    lv_obj_set_style_pad_row(obj, gap, 0);
    lv_obj_set_style_pad_column(obj, gap, 0);
}

void aic_full_width(lv_obj_t *obj)
{
    lv_obj_set_width(obj, lv_pct(100));
}

void aic_full_height(lv_obj_t *obj)
{
    lv_obj_set_height(obj, lv_pct(100));
}

void aic_full_size(lv_obj_t *obj)
{
    lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
}

/*******************************************************************************
 * Common Layouts
 ******************************************************************************/

lv_obj_t *aic_card_create(lv_obj_t *parent, const char *title)
{
    lv_obj_t *card = lv_obj_create(parent);

    /* Card styling */
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, AIC_COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, AIC_RADIUS_DEFAULT, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, AIC_PAD_DEFAULT, 0);

    /* Flex layout for content */
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, AIC_GAP_DEFAULT, 0);

    /* Title */
    if (title != NULL) {
        lv_obj_t *title_label = lv_label_create(card);
        lv_label_set_text(title_label, title);
        lv_obj_set_style_text_color(title_label, AIC_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    }

    return card;
}

lv_obj_t *aic_value_display_create(lv_obj_t *parent, const char *label)
{
    lv_obj_t *row = aic_row_create(parent);

    /* Label */
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text_fmt(lbl, "%s:", label);
    lv_obj_set_style_text_color(lbl, AIC_COLOR_TEXT_DIM, 0);

    /* Value */
    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, "---");
    lv_obj_set_style_text_color(val, AIC_COLOR_TEXT, 0);

    return val;
}

lv_obj_t *aic_status_indicator_create(lv_obj_t *parent, const char *label, bool initial_state)
{
    lv_obj_t *row = aic_row_create(parent);

    /* LED */
    lv_obj_t *led = lv_led_create(row);
    lv_led_set_color(led, initial_state ? AIC_COLOR_SUCCESS : AIC_COLOR_ERROR);
    lv_obj_set_size(led, 16, 16);
    if (initial_state) {
        lv_led_on(led);
    } else {
        lv_led_off(led);
    }

    /* Label */
    if (label != NULL) {
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, AIC_COLOR_TEXT, 0);
    }

    return led;
}

lv_obj_t *aic_icon_button_create(lv_obj_t *parent, const char *icon, const char *text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_height(btn, 40);

    /* Button content */
    lv_obj_t *row = aic_row_create(btn);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_center(row);

    if (icon != NULL) {
        lv_obj_t *icon_lbl = lv_label_create(row);
        lv_label_set_text(icon_lbl, icon);
        lv_obj_set_style_text_color(icon_lbl, AIC_COLOR_TEXT, 0);
    }

    if (text != NULL) {
        lv_obj_t *text_lbl = lv_label_create(row);
        lv_label_set_text(text_lbl, text);
        lv_obj_set_style_text_color(text_lbl, AIC_COLOR_TEXT, 0);
    }

    return btn;
}

/*******************************************************************************
 * Section Helpers
 ******************************************************************************/

lv_obj_t *aic_section_create(lv_obj_t *parent, const char *title)
{
    lv_obj_t *section = aic_col_create(parent);
    aic_full_width(section);

    /* Title */
    if (title != NULL) {
        lv_obj_t *title_lbl = lv_label_create(section);
        lv_label_set_text(title_lbl, title);
        lv_obj_set_style_text_color(title_lbl, AIC_COLOR_PRIMARY, 0);
        lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    }

    /* Content container */
    lv_obj_t *content = aic_col_create(section);
    lv_obj_set_style_pad_left(content, AIC_PAD_DEFAULT, 0);

    return content;
}

lv_obj_t *aic_divider_create(lv_obj_t *parent)
{
    lv_obj_t *line = lv_obj_create(parent);

    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, lv_pct(100), 1);
    lv_obj_set_style_bg_color(line, AIC_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_30, 0);

    return line;
}

/*******************************************************************************
 * Data Display Helpers
 ******************************************************************************/

lv_obj_t *aic_gauge_create(lv_obj_t *parent, const char *label, int32_t min, int32_t max, int32_t initial)
{
    lv_obj_t *cont = aic_col_create(parent);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Arc gauge */
    lv_obj_t *arc = lv_arc_create(cont);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, min, max);
    lv_arc_set_value(arc, initial);
    lv_obj_set_size(arc, 100, 100);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_color(arc, AIC_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, AIC_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);

    /* Label */
    if (label != NULL) {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, AIC_COLOR_TEXT_DIM, 0);
    }

    return arc;
}

lv_obj_t *aic_progress_bar_create(lv_obj_t *parent, const char *label)
{
    lv_obj_t *cont = aic_col_create(parent);
    aic_full_width(cont);

    /* Label */
    if (label != NULL) {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, AIC_COLOR_TEXT_DIM, 0);
    }

    /* Bar */
    lv_obj_t *bar = lv_bar_create(cont);
    lv_obj_set_width(bar, lv_pct(100));
    lv_obj_set_height(bar, 20);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(bar, AIC_COLOR_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, AIC_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);

    return bar;
}

lv_obj_t *aic_xyz_display_create(lv_obj_t *parent, const char *title, lv_obj_t **labels)
{
    lv_obj_t *card = aic_card_create(parent, title);

    const char *axes[] = {"X", "Y", "Z"};
    lv_color_t colors[] = {
        lv_color_hex(0xff6b6b),  /* Red */
        lv_color_hex(0x4ade80),  /* Green */
        lv_color_hex(0x60a5fa)   /* Blue */
    };

    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = aic_row_create(card);

        /* Axis label */
        lv_obj_t *axis = lv_label_create(row);
        lv_label_set_text(axis, axes[i]);
        lv_obj_set_style_text_color(axis, colors[i], 0);
        lv_obj_set_width(axis, 20);

        /* Value label */
        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, "+0.00");
        lv_obj_set_style_text_color(val, AIC_COLOR_TEXT, 0);

        if (labels != NULL) {
            labels[i] = val;
        }
    }

    return card;
}

/*******************************************************************************
 * Screen Helpers
 ******************************************************************************/

void aic_apply_dark_theme(lv_obj_t *scr)
{
    if (scr == NULL) {
        scr = lv_screen_active();
    }

    lv_obj_set_style_bg_color(scr, AIC_COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

lv_obj_t *aic_create_footer(lv_obj_t *parent)
{
    return aic_create_footer_custom(parent, AIC_COPYRIGHT_TEXT, AIC_COPYRIGHT_COLOR);
}

lv_obj_t *aic_create_footer_custom(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *footer = lv_label_create(parent);
    lv_label_set_text(footer, text);
    lv_obj_set_style_text_color(footer, color, 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, AIC_FOOTER_Y_OFFSET);

    return footer;
}

lv_obj_t *aic_create_header(lv_obj_t *parent, const char *title)
{
    lv_obj_t *header = lv_obj_create(parent);

    lv_obj_set_size(header, lv_pct(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, AIC_COLOR_BG_CARD, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, AIC_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(title_lbl);

    return header;
}
