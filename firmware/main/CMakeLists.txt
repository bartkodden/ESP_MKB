cmake_minimum_required(VERSION 3.16)

set(COMPONENT_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

# ─── EEZ / UI sources ────────────────────────────────────────────────────────
file(GLOB UI_C_SOURCES   "${COMPONENT_SRCDIR}/ui/*.c")
file(GLOB UI_CPP_SOURCES "${COMPONENT_SRCDIR}/ui/*.cpp")

# ─── Firmware-embedded static assets (img/ only — small, never-changing) ─────
file(GLOB EMBEDDED_IMAGES "${COMPONENT_SRCDIR}/img/*.png")

# ─── Material Icons Font Generation ──────────────────────────────────────────
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GEN_ICONS_SCRIPT   "${COMPONENT_SRCDIR}/scripts/gen_icons.py")
set(MI_TTF             "${COMPONENT_SRCDIR}/scripts/MaterialIcons-Regular.ttf")
set(MI_CODEPOINTS      "${COMPONENT_SRCDIR}/scripts/MaterialIcons-Regular.codepoints")
set(BUTTONS_JSON       "${COMPONENT_SRCDIR}/buttons.json")
set(MENU_JSON          "${COMPONENT_SRCDIR}/menu.json")
set(MI_FONT_C          "${COMPONENT_SRCDIR}/material_icons_used_24.c")
set(MI_FONT_BIN        "${COMPONENT_SRCDIR}/data/icons/material_icons_used_24.bin")

# Guard: add_custom_command is not available during ESP-IDF script mode
# (dependency resolution phase). Only register build steps during actual build.
if(NOT CMAKE_SCRIPT_MODE_FILE)
    add_custom_command(
        OUTPUT
            ${MI_FONT_C}
            ${MI_FONT_BIN}
        COMMAND
            ${Python3_EXECUTABLE} ${GEN_ICONS_SCRIPT}
        DEPENDS
            ${GEN_ICONS_SCRIPT}
            ${MI_TTF}
            ${MI_CODEPOINTS}
            ${BUTTONS_JSON}
            ${MENU_JSON}
        WORKING_DIRECTORY ${COMPONENT_SRCDIR}/scripts
        COMMENT "Generating Material Icons subset font..."
        VERBATIM
    )

    add_custom_target(generate_mi_font
        DEPENDS ${MI_FONT_C} ${MI_FONT_BIN}
    )
endif()

idf_component_register(
    SRCS
        "main.cpp"
        "init.cpp"
        "ble_config.cpp"
        "bluetoothFunctions.cpp"
        "setupFunctions.cpp"
        "fileFunctions.cpp"
        "menuFunctions.cpp"
        "functions.cpp"
        "mcs_client.cpp"
        "iconManager.cpp"
        "material_icons_used_24.c"
        "buttonUI.cpp"
        "iconManager.cpp"
        ${UI_C_SOURCES}
        ${UI_CPP_SOURCES}
    INCLUDE_DIRS
        "."
        "ui"
    EMBED_TXTFILES
        "buttons.json"          # factory defaults only
        "menu.json"
    EMBED_FILES
        ${EMBEDDED_IMAGES}      # now only truly static assets
    REQUIRES
        json
        joltwallet__littlefs
        bt
        nvs_flash
        led_strip
        esp32-bq27441
        esp32-TCA8418
        TFT_eSPI
        esp_adc
        espressif__arduino-esp32
        lvgl__lvgl
        espressif__esp_lvgl_port
        ESPAsyncWebServer
        AsyncTCP
)

# ─── Font must be generated before compiling material_icons_used_24.c ─────────
if(NOT CMAKE_SCRIPT_MODE_FILE)
    add_dependencies(__idf_main generate_mi_font)
endif()

# ─── LittleFS partition image ─────────────────────────────────────────────────
littlefs_create_partition_image(
    storage
    data
    FLASH_IN_PROJECT
)

# ─── Compiler settings ───────────────────────────────────────────────────────
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    LV_CONF_PATH="${COMPONENT_SRCDIR}/lv_conf.h"
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_definitions(${lvgl_lib} PUBLIC
    LV_CONF_PATH="${COMPONENT_SRCDIR}/lv_conf.h"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-const-variable
    -Wno-missing-field-initializers
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-error -Wno-format")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-format")