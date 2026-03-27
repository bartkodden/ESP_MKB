#include "init.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Hardware
int p1val = 5000;
int p1read = 0;
int p2val = 5000;
int p2read = 0;
int analogDamper = 100;
int volume_value = 0;
int mic_value = 0;
adc_oneshot_unit_handle_t adc1_handle;

TFT_eSPI tft;
uint16_t accentColor = TFT_RED;

Adafruit_TCA8418 keypad;
char keymap[COLS][ROWS] = {
    {'a', 'e', 'j'},
    {'b', 'f', 'i'},
    {'c', 'g', 'k'},
    {'d', 'h', 'l'},
};
volatile bool TCA8418_event = false;
int keysPressed[8] = {0};

led_strip_handle_t led_strip;
const Color colorsDim[8] = {
    {ledsDim, 0, 0}, {0, ledsDim, 0}, {0, 0, ledsDim}, {ledsDim, ledsDim, 0},
    {0, ledsDim, ledsDim}, {ledsDim, 0, ledsDim}, {ledsDim, ledsDim, ledsDim}, {0, 0, 0}
};
const Color colors[8] = {
    {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
    {0, 255, 255}, {255, 0, 255}, {255, 255, 255}, {0, 0, 0}
};

// Encoder
simple_encoder_t rotaryEncoder;
volatile int encoder_delta = 0;
static int last_state = 0;
static bool is_isr_service_installed = false;

void IRAM_ATTR encoder_isr_handler(void* arg) {
    int state_a = gpio_get_level(rotaryEncoder.pin_a);
    int state_b = gpio_get_level(rotaryEncoder.pin_b);
    if (state_a != last_state) {
        encoder_delta += (state_a == state_b) ? 1 : -1;
        last_state = state_a;
    }
}

// Battery
int soc = 100;
int batPerc = 0;
bool charging = false;
bool pluggedIn = false;
bool chargingState = false;
bool doneCharging = false;

// Menu
bool use_eez_ui = true;
ButtonSet* buttonSets = NULL;
int buttonSetsCount = 0;
int activeButtonSetIndex = 0;
int currentMenuIndex = 0;
MenuItem* menuItems = NULL;
int menuSize = 0;
MenuItem menuStack[5];
int menuStackIndex = -1;

// BLE
uint8_t media_state = MCS_STATE_INACTIVE;
char track_title[32] = "No Track";
uint32_t track_duration = 0;
uint32_t track_position = 0;
uint8_t battery_level = 100;
uint8_t current_volume = 50;
uint8_t adv_config_done = 0;
bool ble_conn = false;
esp_bd_addr_t gl_remote_bda;
bool all_services_started = false;
uint8_t services_started_count = 0;
uint8_t volume[2] = {MCS_OP_SET_VOLUME, 50};
uint8_t media_control[1] = {0x00};
uint8_t hid_report[8] = {0};
uint16_t char_handles[PROFILE_NUM][15] = {{0}};
struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];
bool pairing_in_progress = false;
device_mode_t device_mode = MODE_HID_ONLY;  // Default to HID mode

// Sleep
int goSleep = 0;
int timeToSleep = awakeTime;
RTC_DATA_ATTR int bootCount = 0;

// ============================================================================
// INIT FUNCTIONS
// ============================================================================

void init_char_handles(void) {
    for (int i = 0; i < PROFILE_NUM; i++) {
        for (int j = 0; j < 15; j++) {
            char_handles[i][j] = 0;
        }
    }
}

bool get_isr_service_status(void) {
    return is_isr_service_installed;
}

void set_isr_service_installed(void) {
    is_isr_service_installed = true;
}