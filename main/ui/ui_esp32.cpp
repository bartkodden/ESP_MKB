#include "ui.h"
#include "init.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

static const char *TAG = "UI";

static void lv_tick_cb(void *arg) {
    lv_tick_inc(1);  // Advance LVGL internal tick by 1 ms
}

static void tft_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushPixels((uint16_t *)px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

extern "C" esp_err_t ui_init_display() {
    lv_init();

    // Set up LVGL tick source
    static esp_timer_handle_t lvgl_tick_timer = nullptr;
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_cb,
        .name = "lvgl_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1000));  // 1 ms

    // Allocate draw buffers
    size_t buf_size = 240 * 10 * sizeof(lv_color_t);  // 10 rows buffer
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);

    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        return ESP_ERR_NO_MEM;
    }

    lv_display_t *disp = lv_display_create(240, 280);
    lv_display_set_flush_cb(disp, tft_flush);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    return ESP_OK;
}

#include <stdio.h>
#include <string>

static void *fs_open_cb(lv_fs_drv_t *, const char *path, lv_fs_mode_t mode) {
    const char *flags = (mode == LV_FS_MODE_WR) ? "wb" : "rb";
    std::string full = std::string("/storage/") + path;
    return fopen(full.c_str(), flags);
}

static lv_fs_res_t fs_close_cb(lv_fs_drv_t *, void *fp) {
    if (!fp) return LV_FS_RES_INV_PARAM;
    fclose((FILE *)fp);
    return LV_FS_RES_OK;
}

// Same pattern for read/seek/tell:
static lv_fs_res_t fs_read_cb(lv_fs_drv_t *, void *fp, void *buf, uint32_t btr, uint32_t *br) {
    if (!fp) return LV_FS_RES_INV_PARAM;
    size_t r = fread(buf, 1, btr, (FILE *)fp);
    if (br) *br = (uint32_t)r;
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek_cb(lv_fs_drv_t *, void *fp, uint32_t pos, lv_fs_whence_t whence) {
    if (!fp) return LV_FS_RES_INV_PARAM;
    int origin = (whence == LV_FS_SEEK_END) ? SEEK_END :
                 (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_SET;
    return fseek((FILE *)fp, (long)pos, origin) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_tell_cb(lv_fs_drv_t *, void *fp, uint32_t *pos_p) {
    if (!fp || !pos_p) return LV_FS_RES_INV_PARAM;
    long p = ftell((FILE *)fp);
    if (p < 0) return LV_FS_RES_FS_ERR;
    *pos_p = (uint32_t)p;
    return LV_FS_RES_OK;
}

extern "C" void ui_lvgl_register_fs() {
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter  = 'L';
    drv.open_cb = fs_open_cb;
    drv.close_cb = fs_close_cb;
    drv.read_cb  = fs_read_cb;
    drv.seek_cb  = fs_seek_cb;
    drv.tell_cb  = fs_tell_cb;

    lv_fs_drv_register(&drv);
}