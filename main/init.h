#ifndef INIT_H
#define INIT_H

#pragma once
// Standard C includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// ESP-IDF includes
#include "esp_gatts_api.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// C++ includes (Arduino)
#include "Wire.h"
#include "TFT_eSPI.h"
#include "Adafruit_TCA8418.h"
#include "bq27441.h"
#include "led_strip.h"
#include "pinDef.h"

#include "ui/button_labels.h"

// ============================================================================
// CONSTANTS
// ============================================================================

// BLE
#define PROFILE_NUM                3
#define HID_APP_ID                 2
#define BATT_APP_ID                1
#define MCS_APP_ID                 0
#define SERVICE_COUNT              3

#define HID_UUID                   0x1812
#define MCS_UUID                   0x1848
#define BATT_UUID                  0x180F

// Media Control UUIDs
#define MEDIA_STATE                0x2BA3
#define MEDIA_CTRL                 0x2BA4
#define TRACK_TITLE                0x2B97
#define TRACK_DURATION             0x2B98
#define TRACK_POS                  0x2B99
#define VOLUME_CONTROL             0x2B7E

// Media opcodes
#define MCS_OP_PLAY                0x01
#define MCS_OP_PAUSE               0x02
#define MCS_OP_PREV_TRACK          0x30
#define MCS_OP_NEXT_TRACK          0x31
#define MCS_OP_VOLUME_UP           0x40
#define MCS_OP_VOLUME_DOWN         0x41
#define MCS_OP_SET_VOLUME          0x42

#define MCS_STATE_INACTIVE         0x00
#define MCS_STATE_PLAYING          0x01
#define MCS_STATE_PAUSED           0x02

// HID
#define HID_KEYBOARD_REPORT        0x01
#define HID_CONSUMER_REPORT        0x02

// Battery
#define BATT_UPDATE_LEVEL          0x01
#define CONFIG_BQ27441_DESIGN_CAPACITY 2600

// GATT Permissions
#define PERM_READ       ESP_GATT_PERM_READ
#define PERM_WRITE      ESP_GATT_PERM_WRITE
#define PROP_READ       ESP_GATT_CHAR_PROP_BIT_READ
#define PROP_WRITE      ESP_GATT_CHAR_PROP_BIT_WRITE
#define PROP_NOTIFY     ESP_GATT_CHAR_PROP_BIT_NOTIFY
#define PROP_INDICATE   ESP_GATT_CHAR_PROP_BIT_INDICATE

// Hardware
#define NUM_LEDS    18
#define ledsDim     20
#define ledsBright  200
#define ROWS 3
#define COLS 4

// Sleep
#define uS_TO_S_FACTOR 1000000
#define awakeTime  30

// I2C
#define I2C_FREQ_HZ             100000
#define I2C_PORT_NUM            I2C_NUM_0
#define I2C_TX_BUF_DISABLE      0
#define I2C_RX_BUF_DISABLE      0

#define PREPARE_BUF_MAX_SIZE    1024
#define adv_config_flag         (1 << 0)
#define scan_rsp_config_flag    (1 << 1)

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

struct Color {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
};

enum ColorName { RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE, OFF };

typedef struct {
    gpio_num_t pin_a, pin_b;
    int value;
    int min_value, max_value;
    bool wrap_around;
    void (*on_turn)(int);
} simple_encoder_t;

typedef struct MenuItem {
    char* name;
    char* type;
    struct MenuItem* submenu;
    int submenuSize;
} MenuItem;

// typedef struct {
//     char key_id;
//     char icon;
//     uint16_t cmd;
//     char name[32];
// } ButtonMapping;

// typedef struct {
//     char set_name[32];
//     ButtonMapping buttonMappings[8];
// } ButtonSet;

typedef struct {
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

typedef struct {
    uint16_t service_uuid;
    uint16_t char_uuid;
    uint16_t permissions;
    uint16_t properties;
    const char* name;
    uint16_t handle;
    uint16_t cccd_handle;
    uint8_t* init_value;
    uint8_t init_length;
} ble_characteristic_t;

typedef struct {
    uint16_t service_uuid;
    uint8_t profile_id;
    uint8_t is_primary;
    const char* name;
    const ble_characteristic_t* characteristics;
    uint8_t char_count;
} ble_service_t;

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};
typedef enum {
    MODE_HID_ONLY,      // Act as keyboard/media remote
    MODE_MCS_ONLY,      // Display media info from Windows
    MODE_BOTH           // Try both (experimental)
} device_mode_t;

extern device_mode_t device_mode;

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================

// Hardware
extern int p1val, p1read, p2val, p2read;
extern int analogDamper, volume_value, mic_value;
extern adc_oneshot_unit_handle_t adc1_handle;
extern TFT_eSPI tft;
extern uint16_t accentColor;
extern Adafruit_TCA8418 keypad;
extern led_strip_handle_t led_strip;
extern simple_encoder_t rotaryEncoder;
extern volatile int encoder_delta;
extern char keymap[COLS][ROWS];
extern volatile bool TCA8418_event;
extern int keysPressed[8];
extern const Color colorsDim[8];
extern const Color colors[8];

// Battery
extern int soc, batPerc;
extern bool pluggedIn, chargingState, doneCharging, charging, pairing_in_progress;

// Menu
extern bool use_eez_ui;
extern ButtonSet* buttonSets;
extern int buttonSetsCount;  
extern int activeButtonSetIndex;
extern int currentMenuIndex;
extern MenuItem* menuItems;
extern int menuSize;
extern MenuItem menuStack[5];
extern int menuStackIndex;

// BLE
extern uint8_t media_state;
extern char track_title[32];
extern uint32_t track_duration, track_position;
extern uint8_t battery_level, current_volume;
extern uint8_t adv_config_done;
extern bool ble_conn;
extern esp_bd_addr_t gl_remote_bda;
extern bool all_services_started;
extern uint8_t services_started_count;
extern uint8_t volume[2];
extern uint8_t media_control[1];
extern uint8_t hid_report[8];
extern uint16_t char_handles[PROFILE_NUM][15];
extern struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];

// Sleep
extern int goSleep, timeToSleep;
extern int bootCount;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Init
void init_char_handles(void);
void init_profile_tab(void);

void encoder_isr_handler(void* arg);

// ISR service helpers
bool get_isr_service_status(void);
void set_isr_service_installed(void);

#endif // INIT_H