#include "button_labels.h"
#include "ui.h"
#include "../init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static const char *TAG = "BTN_LABELS";

// External state
extern ButtonSet *buttonSets;
extern int        buttonSetsCount;
extern int        activeButtonSetIndex;
void update_button_labels(void);
void cleanup_button_labels(void);

ButtonMapping *getActiveButtonMappings(void);

// Persistent label storage
static char label_a[32] = "";
static char label_b[32] = "";
static char label_c[32] = "";
static char label_d[32] = "";
static char label_e[32] = "";
static char label_f[32] = "";
static char label_g[32] = "";
static char label_h[32] = "";

// Maps fed to the two matrices
static const char *button_labels_bm0[] = {
    label_a,
    label_b,
    "\n",
    label_c,
    label_d,
    NULL
};

static const char *button_labels_bm1[] = {
    label_e,
    label_f,
    "\n",
    label_g,
    label_h,
    NULL
};

// Overlay images for each of the 8 slots (a…h).
static lv_obj_t *button_images[8] = { nullptr };

// --- Helpers --------------------------------------------------------------

static bool icon_is_image(const char *icon) {
    if (!icon || !icon[0]) return false;
    const char *dot = strrchr(icon, '.');
    if (!dot || dot == icon) return false;

    char ext[6] = {0};
    size_t len = strlen(dot + 1);
    if (len == 0 || len > 5) return false;

    for (size_t i = 0; i < len; ++i) {
        ext[i] = (char)tolower((unsigned char)dot[1 + i]);
    }

    return (strcmp(ext, "png") == 0) ||
           (strcmp(ext, "jpg") == 0) ||
           (strcmp(ext, "jpeg") == 0) ||
           (strcmp(ext, "bmp") == 0);
}

static lv_obj_t *matrix_for_index(uint8_t index, uint16_t &btn_idx) {
    if (index < 4) {
        btn_idx = index;
        return objects.bm0;
    } else {
        btn_idx = index - 4;
        return objects.bm1;
    }
}

static void ensure_button_image(uint8_t index) {
    if (button_images[index]) return;

    uint16_t btn_idx;
    lv_obj_t *matrix = matrix_for_index(index, btn_idx);
    if (!matrix) return;

    button_images[index] = lv_image_create(matrix);
    lv_obj_add_flag(button_images[index], LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(button_images[index], LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_set_size(button_images[index], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_flag(button_images[index], LV_OBJ_FLAG_HIDDEN);
}

static void hide_button_image(uint8_t index) {
    if (!button_images[index]) return;
    lv_obj_add_flag(button_images[index], LV_OBJ_FLAG_HIDDEN);
}


static void position_button_image(uint8_t index) {
    uint16_t btn_idx;
    lv_obj_t *matrix = matrix_for_index(index, btn_idx);
    if (!matrix || !button_images[index]) return;

    // For maps like {btn, btn, "\n", btn, btn}, we have 2 columns x 2 rows
    const uint8_t cols = 2;
    const uint8_t rows = 2;

    uint8_t row = btn_idx / cols;
    uint8_t col = btn_idx % cols;

    lv_area_t content;
    lv_obj_get_content_coords(matrix, &content);

    lv_coord_t cell_w = lv_area_get_width(&content) / cols;
    lv_coord_t cell_h = lv_area_get_height(&content) / rows;

    lv_obj_t *img = button_images[index];
    lv_coord_t img_w = lv_obj_get_width(img);
    lv_coord_t img_h = lv_obj_get_height(img);

    lv_coord_t x = content.x1 + col * cell_w + (cell_w - img_w) / 2;
    lv_coord_t y = content.y1 + row * cell_h + (cell_h - img_h) / 2;

    lv_obj_set_pos(img, x, y);
}


static void set_button_image(uint8_t index, const char *relative_path) {
    uint16_t btn_idx;
    lv_obj_t *matrix = matrix_for_index(index, btn_idx);
    if (!matrix) return;

    ensure_button_image(index);
    lv_obj_t *img = button_images[index];
    if (!img) return;

    const char *path = relative_path;
    if (strncmp(relative_path, "L:/", 3) == 0) {
        path = relative_path;
    } else if (relative_path[0] == '/') {
        path = relative_path + 1;
    }

    char src[128];
    if (strncmp(path, "L:/", 3) != 0) {
        snprintf(src, sizeof(src), "L:/%s", path);
    } else {
        strlcpy(src, path, sizeof(src));
    }

    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_src(img, src);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
    position_button_image(index);
}

// --- Name lookup ---------------------------------------------------------

static const ButtonMapping *find_mapping_for_key(char key_id) {
    ButtonMapping *mappings = getActiveButtonMappings();
    if (!mappings) {
        ESP_LOGE(TAG, "getActiveButtonMappings() returned NULL!");
        return nullptr;
    }
    for (int i = 0; i < 8; ++i) {
        if (mappings[i].key_id == key_id) {
            return &mappings[i];
        }
    }
    return nullptr;
}

// --- Public API ----------------------------------------------------------

void update_button_labels(void) {
    if (!buttonSets || buttonSetsCount == 0) {
        ESP_LOGW(TAG, "No button sets loaded");
        return;
    }
    if (activeButtonSetIndex < 0 || activeButtonSetIndex >= buttonSetsCount) {
        ESP_LOGW(TAG, "Active set index out of range");
        return;
    }

    static const char key_order[8] = { 'a','b','c','d','e','f','g','h' };
    static char *label_slots[8] = {
        label_a,label_b,label_c,label_d,
        label_e,label_f,label_g,label_h
    };
    static size_t label_sizes[8] = {
        sizeof(label_a),sizeof(label_b),sizeof(label_c),sizeof(label_d),
        sizeof(label_e),sizeof(label_f),sizeof(label_g),sizeof(label_h)
    };

    ESP_LOGI(TAG, "Updating button labels for set: %s (index %d)",
             buttonSets[activeButtonSetIndex].set_name, activeButtonSetIndex);

    for (uint8_t i = 0; i < 8; ++i) {
        const ButtonMapping *mapping = find_mapping_for_key(key_order[i]);
        const char *text = "";

        if (mapping) {
            bool has_image = icon_is_image(mapping->icon);
            if (has_image) {
                if (mapping->name[0]) {
                    text = mapping->name;
                } else {
                    text = " ";
                }
                set_button_image(i, mapping->icon);
            } else {
                hide_button_image(i);
                if (mapping->icon[0]) {
                    text = mapping->icon;
                } else if (mapping->name[0]) {
                    text = mapping->name;
                } else {
                    text = " ";
                }
            }
        } else {
            hide_button_image(i);
            text = " ";
        }

        strlcpy(label_slots[i], text, label_sizes[i]);
    }

    if (objects.bm0) {
        lv_buttonmatrix_set_map(objects.bm0, button_labels_bm0);
        lv_obj_invalidate(objects.bm0);
    }
    if (objects.bm1) {
        lv_buttonmatrix_set_map(objects.bm1, button_labels_bm1);
        lv_obj_invalidate(objects.bm1);
    }

    for (uint8_t i = 0; i < 8; ++i) {
        if (button_images[i]) {
            position_button_image(i);
        }
    }
}

void cleanup_button_labels(void) {
    for (auto &img : button_images) {
        if (img) {
            lv_obj_del(img);
            img = nullptr;
        }
    }
}