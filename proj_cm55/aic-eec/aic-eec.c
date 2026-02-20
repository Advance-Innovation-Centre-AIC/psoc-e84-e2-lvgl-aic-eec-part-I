/*******************************************************************************
 * File Name:   aic-eec.c
 *
 * Description: AIC-EEC Common UI Components Implementation
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 * 
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 ******************************************************************************/

#include "aic-eec.h"
#include <stdio.h>

/*******************************************************************************
 * Footer/Footnote Functions
 ******************************************************************************/

/* aic_create_footer() is now provided by aic_layout.c */
/* aic_create_header() is now provided by aic_layout.c */

lv_obj_t * aic_create_header_custom(lv_obj_t * parent, const char * text, lv_color_t color)
{
    lv_obj_t * header = lv_label_create(parent);
    lv_label_set_text(header, text);
    lv_obj_set_style_text_color(header, color, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, AIC_HEADER_Y_OFFSET);
    return header;
}

/*******************************************************************************
 * Logo Functions (Placeholder)
 ******************************************************************************/

lv_obj_t * aic_create_logo(lv_obj_t * parent, lv_align_t align, int32_t x_ofs, int32_t y_ofs)
{
    /* TODO: Implement when logo image is available
     *
     * To add a logo:
     * 1. Convert PNG/JPG to C array using LVGL Image Converter
     *    https://lvgl.io/tools/imageconverter
     * 2. Include the generated .c file
     * 3. Use lv_image_create() and lv_image_set_src()
     *
     * Example:
     *   LV_IMAGE_DECLARE(aic_logo);
     *   lv_obj_t * logo = lv_image_create(parent);
     *   lv_image_set_src(logo, &aic_logo);
     *   lv_obj_align(logo, align, x_ofs, y_ofs);
     *   return logo;
     */

    (void)parent;
    (void)align;
    (void)x_ofs;
    (void)y_ofs;

    printf("aic_create_logo: Not implemented yet\r\n");
    return NULL;
}

/*******************************************************************************
 * Menu Functions (Placeholder)
 ******************************************************************************/

lv_obj_t * aic_create_menu(lv_obj_t * parent)
{
    /* TODO: Implement navigation menu
     *
     * Possible implementation:
     * - Use lv_menu_create() for hierarchical menu
     * - Or lv_btnmatrix_create() for simple button grid
     * - Or lv_list_create() for list-based menu
     */

    (void)parent;

    printf("aic_create_menu: Not implemented yet\r\n");
    return NULL;
}

/*******************************************************************************
 * Video/Media Functions (Placeholder)
 ******************************************************************************/

lv_obj_t * aic_create_video_controls(lv_obj_t * parent)
{
    /* TODO: Implement video playback controls
     *
     * Possible implementation:
     * - Play/Pause button
     * - Progress slider
     * - Volume control
     * - Fullscreen toggle
     */

    (void)parent;

    printf("aic_create_video_controls: Not implemented yet\r\n");
    return NULL;
}

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

void aic_apply_theme(lv_obj_t * screen, lv_color_t bg_color)
{
    lv_obj_set_style_bg_color(screen, bg_color, LV_PART_MAIN);
}

lv_obj_t * aic_create_container(lv_obj_t * parent, int32_t width, int32_t height)
{
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_size(cont, width, height);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(cont, 2, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(cont, 10, 0);
    return cont;
}
