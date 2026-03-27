cmake_minimum_required(VERSION 3.16)

set(COMPONENT_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

# Ensure JSON templates are re-embedded every build
foreach(json_file IN ITEMS buttons.json menu.json)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E touch
                "${COMPONENT_SRCDIR}/${json_file}"
    )
endforeach()

# Collect EEZ-generated sources
file(GLOB UI_C_SOURCES   "${COMPONENT_SRCDIR}/ui/*.c")
file(GLOB UI_CPP_SOURCES "${COMPONENT_SRCDIR}/ui/*.cpp")

# Collect all PNG assets under ./main/img
file(GLOB BUTTON_IMAGE_FILES
     "${COMPONENT_SRCDIR}/img/*.png")

idf_component_register(
    SRCS
        "main.cpp"
        "init.cpp"
        "ble_config.cpp"
        "bluetoothfunctions.cpp"
        "setupfunctions.cpp"
        "filefunctions.cpp"
        "menufunctions.cpp"
        "functions.cpp"
        "mcs_client.cpp"
        ${UI_C_SOURCES}
        ${UI_CPP_SOURCES}
    INCLUDE_DIRS
        "."
        "ui"
    EMBED_TXTFILES
        "menu.json"
        "buttons.json"
    EMBED_FILES
        ${BUTTON_IMAGE_FILES}
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