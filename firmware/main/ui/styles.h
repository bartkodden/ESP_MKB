#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: TOP_BAR
lv_style_t *get_style_top_bar_MAIN_DEFAULT();
void add_style_top_bar(lv_obj_t *obj);
void remove_style_top_bar(lv_obj_t *obj);

// Style: bar_charging
lv_style_t *get_style_bar_charging_MAIN_DEFAULT();
void add_style_bar_charging(lv_obj_t *obj);
void remove_style_bar_charging(lv_obj_t *obj);

// Style: buttonStyle
lv_style_t *get_style_button_style_MAIN_DEFAULT();
void add_style_button_style(lv_obj_t *obj);
void remove_style_button_style(lv_obj_t *obj);

// Style: macrobuttons
lv_style_t *get_style_macrobuttons_MAIN_DEFAULT();
lv_style_t *get_style_macrobuttons_ITEMS_DEFAULT();
lv_style_t *get_style_macrobuttons_ITEMS_CHECKED_PRESSED();
lv_style_t *get_style_macrobuttons_ITEMS_PRESSED();
lv_style_t *get_style_macrobuttons_ITEMS_CHECKED();
void add_style_macrobuttons(lv_obj_t *obj);
void remove_style_macrobuttons(lv_obj_t *obj);

void change_color_theme(uint32_t themeIndex);
uint32_t next_theme(uint32_t active_theme_index);
#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/