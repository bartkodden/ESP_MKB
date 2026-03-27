#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1


// Memory
#define LV_MEM_SIZE (48 * 1024U)

// Display
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 280

#define LV_DEF_REFR_PERIOD 33
#define LV_USE_ANIM 1
#define LV_ANIM_DEFAULT_TIME 200

#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

// Logging
#define LV_USE_LOG 0

// Fonts
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1

// Widgets
#define LV_USE_CANVAS 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_WIN 0
#define LV_USE_ANIM 1

// Disable unused widgets
#define LV_USE_MSGBOX 1
#define LV_USE_MENU 0
#define LV_USE_TABVIEW 0
#define LV_USE_SNAPSHOT 0

// Image cache
#define LV_IMAGE_CACHE_DEF_SIZE 1

// PNG Loading
#define LV_USE_PNG     1
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_STDIO 1
#define LV_FS_STDIO_LETTER  'L'
#define LV_FS_STDIO_PATH "/storage"
#define LV_FS_STDIO_CACHE_SIZE 0

#endif