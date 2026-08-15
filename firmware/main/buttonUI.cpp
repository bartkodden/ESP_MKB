// buttonUI.cpp
#include "buttonUI.h"
#include "button_labels.h"     // ButtonSet, ButtonMapping types
#include "icon_codepoints.h"   // icon_lookup_codepoint(), icon_cp_to_utf8()
#include "ui/screens.h"        // objects, theme_colors, active_theme_index
#include "esp_log.h"
#include <string.h>
#include "styles.h"
#include <stdio.h>

static const char *TAG = "BTNUI";

// ── External state from fileFunctions / screens ───────────────────────────────
extern ButtonSet *buttonSets;
extern int        buttonSetsCount;
extern int        activeButtonSetIndex;

// ── Internal state ────────────────────────────────────────────────────────────
static lv_obj_t        *s_panel_left  = nullptr;
static lv_obj_t        *s_panel_right = nullptr;
static const lv_font_t *s_mi_font     = nullptr;
static bool             s_created     = false;

typedef struct {
    lv_obj_t *btn;
    lv_obj_t *icon_lbl;
    lv_obj_t *name_lbl;
} btn_slot_t;

static btn_slot_t s_slots[8];

// ── Panel setup (flex grid) ───────────────────────────────────────────────────
// Each 115×115 panel holds a 2×2 grid of 55×55 buttons with 4px gap:
//   2 × 55 + 4 = 114px → fits in 115px ✓
static void setup_panel(lv_obj_t *panel) {
    lv_obj_set_style_layout(panel, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(panel, LV_FLEX_FLOW_ROW_WRAP, 0);
    lv_obj_set_style_pad_row(panel, 4, 0);
    lv_obj_set_style_pad_column(panel, 4, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    // bg/border already transparent from EEZ, belt-and-braces:
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
}

// ── Per-button creation ───────────────────────────────────────────────────────
static void create_slot(uint8_t i) {
    lv_obj_t *parent = (i < 4) ? s_panel_left : s_panel_right;
    if (!parent) return;

    btn_slot_t *s = &s_slots[i];

    // Button — uses EEZ styles directly, theme changes propagate automatically
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 55, 55);
    lv_obj_add_style(btn, get_style_macrobuttons_ITEMS_DEFAULT(),
                    (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_add_style(btn, get_style_macrobuttons_ITEMS_PRESSED(),
                    (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_PRESSED));
    lv_obj_add_style(btn, get_style_macrobuttons_ITEMS_CHECKED(),
                    (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_CHECKED));
    lv_obj_add_style(btn, get_style_macrobuttons_ITEMS_CHECKED_PRESSED(),
                    (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED));
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    // Get text color from EEZ style — no extern needed
    lv_style_value_t text_val;
    lv_color_t text_col = lv_color_white();   // safe default
    if (lv_style_get_prop(get_style_macrobuttons_ITEMS_DEFAULT(),
                          LV_STYLE_TEXT_COLOR, &text_val) == LV_STYLE_RES_FOUND) {
        text_col = text_val.color;
    }

    // Icon label
    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, "");
    lv_obj_set_style_text_font(icon,
        s_mi_font ? s_mi_font : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(icon, text_col, 0);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, 0);

    // Name label
    lv_obj_t *name = lv_label_create(btn);
    lv_label_set_text(name, "");
    lv_obj_set_style_text_font(name, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(name, text_col, 0);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(name, 51);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);

    s->btn      = btn;
    s->icon_lbl = icon;
    s->name_lbl = name;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
static bool starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

// ── Public API ────────────────────────────────────────────────────────────────

void buttonui_init(lv_obj_t *panel_left, lv_obj_t *panel_right) {
    s_panel_left  = panel_left;
    s_panel_right = panel_right;

    // init_styles();   ← remove, EEZ owns styles now

    setup_panel(panel_left);
    setup_panel(panel_right);

    for (uint8_t i = 0; i < 8; i++) {
        create_slot(i);
    }

    s_created = true;
    buttonui_refresh();
    ESP_LOGI(TAG, "Button UI initialised (8 slots)");
}

void buttonui_set_icon_font(const lv_font_t *font) {
    s_mi_font = font;
    if (!s_created) return;
    for (uint8_t i = 0; i < 8; i++) {
        if (s_slots[i].icon_lbl) {
            lv_obj_set_style_text_font(s_slots[i].icon_lbl, font ? font : &lv_font_montserrat_14, 0);
        }
    }
    buttonui_refresh();   // re-render glyphs with real font
}

void buttonui_refresh(void) {
    if (!s_created) {
        ESP_LOGW(TAG, "refresh() called before init()");
        return;
    }
    if (!buttonSets || buttonSetsCount == 0 ||
        activeButtonSetIndex < 0 || activeButtonSetIndex >= buttonSetsCount) {
        ESP_LOGW(TAG, "No button sets available");
        return;
    }

    ButtonMapping *mappings =
        buttonSets[activeButtonSetIndex].buttonMappings;

    for (uint8_t i = 0; i < 8; i++) {
        btn_slot_t *s = &s_slots[i];
        if (!s->btn) continue;
        if (!lv_obj_is_valid(s->btn)) {
            s->btn = s->icon_lbl = s->name_lbl = nullptr;
            continue;
        }

        const char *icon_str = mappings[i].icon;
        const char *name_str = mappings[i].name;
        bool icon_rendered = false;

        lv_style_value_t text_val;
        lv_color_t text_col = lv_color_white();
        if (lv_style_get_prop(get_style_macrobuttons_ITEMS_DEFAULT(),
                            LV_STYLE_TEXT_COLOR, &text_val) == LV_STYLE_RES_FOUND) {
            text_col = text_val.color;
        }
        lv_obj_set_style_text_color(s->icon_lbl, text_col, 0);
        lv_obj_set_style_text_color(s->name_lbl, text_col, 0);

        // ── mi:<name> → Material Icon glyph ──────────────────────────────
        if (starts_with(icon_str, "mi:")) {
            const char *icon_name = icon_str + 3;   // skip "mi:"
            uint32_t cp = icon_lookup_codepoint(icon_name);

            if (cp == 0) {
                ESP_LOGW(TAG, "Slot %d: unknown MI icon '%s'", i, icon_name);
            } else if (!s_mi_font) {
                ESP_LOGW(TAG, "Slot %d: MI font not loaded yet", i);
            } else {
                char utf8[5] = {0};
                icon_cp_to_utf8(cp, utf8);
                lv_font_glyph_dsc_t g;
                bool found = lv_font_get_glyph_dsc(s_mi_font, &g, cp, 0);
                ESP_LOGI(TAG, "Slot %d: cp=0x%lx found=%d", i, cp, found);
                lv_label_set_text(s->icon_lbl, utf8);
                lv_obj_set_style_text_font(s->icon_lbl, s_mi_font, 0);
                lv_obj_remove_flag(s->icon_lbl, LV_OBJ_FLAG_HIDDEN);
                icon_rendered = true;
            }
        }

        // ── custom:<file>.bin → lv_image (placeholder for now) ───────────
        else if (starts_with(icon_str, "custom:")) {
            // TODO: implement in the web-upload milestone
            // For now fall through to text
        }

        // ── plain text / emoji fallback ───────────────────────────────────
        else if (icon_str[0] != '\0') {
            lv_label_set_text(s->icon_lbl, icon_str);
            lv_obj_set_style_text_font(s->icon_lbl, &lv_font_montserrat_18, 0);
            lv_obj_remove_flag(s->icon_lbl, LV_OBJ_FLAG_HIDDEN);
            icon_rendered = true;
        }

        // ── no icon → hide icon label, name takes full button ─────────────
        if (!icon_rendered) {
            lv_label_set_text(s->icon_lbl, "");
            lv_obj_add_flag(s->icon_lbl, LV_OBJ_FLAG_HIDDEN);
        }

        // ── name label ─────────────────────────────────────────────────────
        lv_label_set_text(s->name_lbl,
            (name_str && name_str[0]) ? name_str : " ");
    }
}

void buttonui_set_pressed(uint8_t index, bool pressed) {
    if (index >= 8) return;
    if (!s_slots[index].btn) return;

    // Verify the object is still valid before touching it
    if (!lv_obj_is_valid(s_slots[index].btn)) {
        ESP_LOGW(TAG, "Slot %d btn is invalid — clearing", index);
        s_slots[index].btn      = nullptr;
        s_slots[index].icon_lbl = nullptr;
        s_slots[index].name_lbl = nullptr;
        return;
    }

    if (pressed) {
        lv_obj_add_state(s_slots[index].btn, LV_STATE_PRESSED);
    } else {
        lv_obj_remove_state(s_slots[index].btn, LV_STATE_PRESSED);
    }
}