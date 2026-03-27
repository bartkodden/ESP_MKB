#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_MENU = 2,
    SCREEN_ID_LOADING = 3,
    SCREEN_ID_ERROR = 4,
    _SCREEN_ID_LAST = 4
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *menu;
    lv_obj_t *loading;
    lv_obj_t *error;
    lv_obj_t *topbar;
    lv_obj_t *batperclabel;
    lv_obj_t *bticon;
    lv_obj_t *batpercbar;
    lv_obj_t *buttonbox;
    lv_obj_t *bm0;
    lv_obj_t *bm1;
    lv_obj_t *blepairingpopup;
    lv_obj_t *devicename;
    lv_obj_t *devicename_1;
    lv_obj_t *paircode;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *mediabox;
    lv_obj_t *trackprogress;
    lv_obj_t *albumart;
    lv_obj_t *trackname;
    lv_obj_t *artistname;
    lv_obj_t *albumname;
    lv_obj_t *buttonset;
    lv_obj_t *mcs_nc;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *loadingstatus;
    lv_obj_t *obj4;
    lv_obj_t *errortext;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_menu();
void tick_screen_menu();

void create_screen_loading();
void tick_screen_loading();

void create_screen_error();
void tick_screen_error();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

// Color themes

enum Themes {
    THEME_ID_RED,
    THEME_ID_GREEN,
    THEME_ID_BLUE,
    THEME_ID_DARK,
    THEME_ID_PINK,
};
enum Colors {
    COLOR_ID_ACCENTBRIGHT,
    COLOR_ID_ACCENTDARK,
    COLOR_ID_TEXT,
};
void change_color_theme(uint32_t themeIndex);
extern uint32_t theme_colors[5][3];
extern uint32_t active_theme_index;

//
// Helper functions
//

lv_anim_t *get_anim();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/