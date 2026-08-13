#include "iconManager.h"
#include "buttonUI.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "ICON_MGR";

// Compiled-in font from gen_icons.py --format lvgl
// Declared in the generated material_icons_used_24.c
extern const lv_font_t material_icons_used_24;

static const lv_font_t *s_font = nullptr;

esp_err_t iconmanager_init(void) {
    ESP_LOGI(TAG, "Free heap before font init: %" PRIu32 " bytes",
             esp_get_free_heap_size());

    // Use compiled-in font — no file I/O, no format issues, instant
    s_font = &material_icons_used_24;
    buttonui_set_icon_font(s_font);

    ESP_LOGI(TAG, "Font ready: %d px", s_font->line_height);
    ESP_LOGI(TAG, "Free heap after font init:  %" PRIu32 " bytes",
             esp_get_free_heap_size());
    return ESP_OK;
}

esp_err_t iconmanager_reload(void) {
    // Future: reload from LittleFS binary after web upload
    // For now same as init
    return iconmanager_init();
}

const lv_font_t* iconmanager_get_font(void) {
    return s_font;
}