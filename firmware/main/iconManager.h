// iconManager.h
#pragma once
#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Call once after LittleFS is mounted.
// Loads the font and wires it to buttonUI automatically.
esp_err_t iconmanager_init(void);

// Returns the loaded font, or NULL if not yet loaded.
const lv_font_t* iconmanager_get_font(void);

// Reload font from LittleFS (call after an OTA font update).
esp_err_t iconmanager_reload(void);

#ifdef __cplusplus
}
#endif