#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_charging_black;
extern const lv_img_dsc_t img_charging_white;
extern const lv_img_dsc_t img_wifi_on;
extern const lv_img_dsc_t img_wifi_off;
extern const lv_img_dsc_t img_bt_on;
extern const lv_img_dsc_t img_bt_off;
extern const lv_img_dsc_t img_album;
extern const lv_img_dsc_t img_noart;
extern const lv_img_dsc_t img_pcb;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[9];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/