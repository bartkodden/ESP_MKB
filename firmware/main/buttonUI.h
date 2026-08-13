// buttonUI.h
#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Call once in ui_init(), after create_screens()
void buttonui_init(lv_obj_t *panel_left, lv_obj_t *panel_right);

// Call after LittleFS mounts and icon font loads
void buttonui_set_icon_font(const lv_font_t *font);

// Call when active button set changes (nextButtonSet / previousButtonSet)
void buttonui_refresh(void);

// Call from your keypress / keyrelease handler (index 0–7)
void buttonui_set_pressed(uint8_t index, bool pressed);

#ifdef __cplusplus
}
#endif