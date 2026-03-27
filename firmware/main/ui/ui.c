#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include "button_labels.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"

// Simple mode (no EEZ flow framework)

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

esp_err_t ui_init() {
    create_screens();
    currentScreen = 0;
    lv_scr_load(objects.main);
    update_button_labels();
    return ESP_OK;
}

void ui_tick() {
    tick_screen(currentScreen);
    //lv_refr_now(lv_display_get_default());
}

typedef struct {
    lv_obj_t *image_objs[8];   // matches 2×4 matrix
} button_images_t;

static button_images_t button_images;

static void ensure_button_image(uint8_t index) {
    if (button_images.image_objs[index]) return;
    button_images.image_objs[index] = lv_image_create(objects.buttonbox);
    lv_obj_add_flag(button_images.image_objs[index], LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(button_images.image_objs[index], 64, 64); // adjust as needed
}