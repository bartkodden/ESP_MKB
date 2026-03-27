#include "esp_err.h"
#ifndef EEZ_LVGL_UI_GUI_H
#define EEZ_LVGL_UI_GUI_H

#include <lvgl.h>

#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_init();
void ui_tick();

void loadScreen(enum ScreensEnum screenId);

esp_err_t ui_init_display();

#ifdef __cplusplus
}
#endif

#endif // EEZ_LVGL_UI_GUI_H