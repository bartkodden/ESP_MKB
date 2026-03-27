#include "styles.h"
#include "images.h"
#include "fonts.h"
#include "esp_log.h"

#include "ui.h"
#include "screens.h"

//
// Style: TOP_BAR
//

void init_style_top_bar_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff3775dd));
};

lv_style_t *get_style_top_bar_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_top_bar_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_top_bar(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_top_bar_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_top_bar(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_top_bar_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: bar_charging
//

void init_style_bar_charging_MAIN_DEFAULT(lv_style_t *style) {
};

lv_style_t *get_style_bar_charging_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_bar_charging_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_bar_charging(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_bar_charging_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_bar_charging(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_bar_charging_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: buttonStyle
//

void init_style_button_style_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_border_color(style, lv_color_hex(0xffd2d2d2));
    lv_style_set_border_width(style, 1);
    lv_style_set_bg_color(style, lv_color_hex(0xff50ce46));
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(style, lv_color_hex(0xff027c09));
    lv_style_set_text_color(style, lv_color_hex(0xffffffff));
};

lv_style_t *get_style_button_style_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_button_style_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_button_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_button_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_button_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_button_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: macrobuttons
//

void init_style_macrobuttons_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_layout(style, LV_LAYOUT_GRID);
    {
        static lv_coord_t dsc[] = {0, LV_GRID_TEMPLATE_LAST};
        lv_style_set_grid_row_dsc_array(style, dsc);
    }
    {
        static lv_coord_t dsc[] = {0, LV_GRID_TEMPLATE_LAST};
        lv_style_set_grid_column_dsc_array(style, dsc);
    }
    lv_style_set_flex_flow(style, LV_FLEX_FLOW_ROW);
    lv_style_set_pad_top(style, 0);
    lv_style_set_pad_bottom(style, 0);
    lv_style_set_pad_left(style, 0);
    lv_style_set_pad_right(style, 0);
    lv_style_set_pad_row(style, 4);
    lv_style_set_pad_column(style, 4);
    lv_style_set_bg_opa(style, 0);
    lv_style_set_border_width(style, 0);
    lv_style_set_radius(style, 7);
};

lv_style_t *get_style_macrobuttons_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_macrobuttons_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_macrobuttons_ITEMS_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[active_theme_index][0]));
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(style, lv_color_hex(theme_colors[active_theme_index][1]));
    lv_style_set_bg_main_stop(style, 100);
    lv_style_set_border_color(style, lv_color_hex(0xffffffff));
    lv_style_set_border_opa(style, 255);
    lv_style_set_border_width(style, 1);
    lv_style_set_border_side(style, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(style, 4);
    lv_style_set_width(style, 20);
    lv_style_set_text_color(style, lv_color_hex(theme_colors[active_theme_index][2]));
};

lv_style_t *get_style_macrobuttons_ITEMS_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_macrobuttons_ITEMS_DEFAULT(style);
    }
    return style;
};

void init_style_macrobuttons_ITEMS_CHECKED_PRESSED(lv_style_t *style) {
    lv_style_set_border_color(style, lv_color_hex(0xffffffff));
    lv_style_set_border_width(style, 2);
    lv_style_set_bg_color(style, lv_color_hex(0xfff39221));
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(style, lv_color_hex(0xff626262));
};

lv_style_t *get_style_macrobuttons_ITEMS_CHECKED_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_macrobuttons_ITEMS_CHECKED_PRESSED(style);
    }
    return style;
};

void init_style_macrobuttons_ITEMS_PRESSED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xfff39221));
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(style, lv_color_hex(0xff9d4701));
};

lv_style_t *get_style_macrobuttons_ITEMS_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_macrobuttons_ITEMS_PRESSED(style);
    }
    return style;
};

void init_style_macrobuttons_ITEMS_CHECKED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xfff39221));
    lv_style_set_bg_grad_color(style, lv_color_hex(0xff9d4701));
};

lv_style_t *get_style_macrobuttons_ITEMS_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_macrobuttons_ITEMS_CHECKED(style);
    }
    return style;
};

void add_style_macrobuttons(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_macrobuttons_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_macrobuttons_ITEMS_DEFAULT(), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_macrobuttons_ITEMS_CHECKED_PRESSED(), LV_PART_ITEMS | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_add_style(obj, get_style_macrobuttons_ITEMS_PRESSED(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_style(obj, get_style_macrobuttons_ITEMS_CHECKED(), LV_PART_ITEMS | LV_STATE_CHECKED);
};

void remove_style_macrobuttons(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_macrobuttons_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_macrobuttons_ITEMS_DEFAULT(), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_macrobuttons_ITEMS_CHECKED_PRESSED(), LV_PART_ITEMS | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_remove_style(obj, get_style_macrobuttons_ITEMS_PRESSED(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_remove_style(obj, get_style_macrobuttons_ITEMS_CHECKED(), LV_PART_ITEMS | LV_STATE_CHECKED);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_top_bar,
        add_style_bar_charging,
        add_style_button_style,
        add_style_macrobuttons,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_top_bar,
        remove_style_bar_charging,
        remove_style_button_style,
        remove_style_macrobuttons,
    };
    remove_style_funcs[styleIndex](obj);
}

uint32_t next_theme(uint32_t active_theme_index){
    uint32_t new_theme = active_theme_index + 1;
    if(new_theme >= (sizeof (theme_colors) / sizeof (theme_colors[0]))){
        new_theme = 0;
        change_color_theme(0);
    }else{
        change_color_theme(new_theme);
    }
    return new_theme;
}