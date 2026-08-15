#ifndef LV_CONF_H
#define LV_CONF_H

// ── Color ─────────────────────────────────────────────────────────────────────
#define LV_COLOR_DEPTH   16
#define LV_COLOR_16_SWAP 1

// ── Memory ───────────────────────────────────────────────────────────────────
// LVGL uses its own isolated memory pool
// This pool lives in .bss and is never fragmented by BLE/WiFi/HTTP allocations
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
//#define LV_MEM_SIZE                  (48 * 1024U)
#define LV_DRAW_SW_GRAD_CACHE_DEF_SIZE  2048
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE   (24 * 1024)

// ── Display ──────────────────────────────────────────────────────────────────
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 280

// ── Animation ────────────────────────────────────────────────────────────────
#define LV_USE_ANIM          1
#define LV_ANIM_DEFAULT_TIME 200
#define LV_DEF_REFR_PERIOD   33

// ── Logging ──────────────────────────────────────────────────────────────────
#define LV_USE_LOG 0

// ── Fonts ────────────────────────────────────────────────────────────────────
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1

// ── Widgets ──────────────────────────────────────────────────────────────────
#define LV_USE_CANVAS   1
#define LV_USE_IMG      1
#define LV_USE_LABEL    1
#define LV_USE_BAR      1
#define LV_USE_WIN      0
#define LV_USE_MSGBOX   1
#define LV_USE_MENU     0
#define LV_USE_TABVIEW  0
#define LV_USE_SNAPSHOT 0
#define LV_USE_BTNMATRIX 0

// ── Image cache ──────────────────────────────────────────────────────────────
#define LV_IMAGE_CACHE_DEF_SIZE 1

// ── PNG decoder ──────────────────────────────────────────────────────────────
#define LV_USE_PNG 1

// ── Filesystem (STDIO — auto-initialised by LVGL, no registration call needed)
#define LV_USE_FS_FATFS  0
#define LV_USE_FS_STDIO  0
#define LV_FS_STDIO_LETTER     'L'
#define LV_FS_STDIO_PATH       "/storage"
#define LV_FS_STDIO_CACHE_SIZE 4096

// ── Demos (disabled) ─────────────────────────────────────────────────────────
#define LV_BUILD_EXAMPLES      0
#define LV_USE_DEMO_WIDGETS    0
#define LV_USE_DEMO_BENCHMARK  0
#define LV_USE_DEMO_STRESS     0
#define LV_USE_DEMO_MUSIC      0

#endif