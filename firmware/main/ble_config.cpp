#include "ble_config.h"

// Forward declarations from bluetoothfunctions.h
extern void gatts_profile_hid_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
extern void gatts_profile_mcs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
extern void gatts_profile_bs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// ============================================================================
// HID DESCRIPTORS
// ============================================================================

uint8_t hid_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
    0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
    0xC0,
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x02,
    0x19, 0x00, 0x2A, 0x3C, 0x02, 0x15, 0x00, 0x26, 0x3C, 0x02, 0x95, 0x01, 0x75, 0x10, 0x81, 0x00,
    0xC0
};

const size_t hid_report_map_len = sizeof(hid_report_map);

uint8_t hid_info[4] = {0x11, 0x01, 0x00, 0x03};
uint8_t hid_control_point = 0x00;
uint8_t protocol_mode = 0x01;

// ============================================================================
// CHARACTERISTICS
// ============================================================================

const ble_characteristic_t mcs_characteristics[] = {
    {MCS_UUID, MEDIA_STATE, PERM_READ | PERM_WRITE, PROP_READ | PROP_WRITE, "Media State", 0, 0, &media_state, 1},
    {MCS_UUID, MEDIA_CTRL, PERM_READ, PROP_READ | PROP_NOTIFY, "Media Control Point", 0, 0, media_control, 1},
    {MCS_UUID, TRACK_TITLE, PERM_WRITE, PROP_WRITE, "Track Title", 0, 0, (uint8_t*)track_title, sizeof(track_title)},
    {MCS_UUID, TRACK_DURATION, PERM_WRITE, PROP_WRITE, "Track Duration", 0, 0, (uint8_t*)&track_duration, sizeof(track_duration)},
    {MCS_UUID, TRACK_POS, PERM_WRITE, PROP_WRITE, "Track Position", 0, 0, (uint8_t*)&track_position, sizeof(track_position)},
    {MCS_UUID, VOLUME_CONTROL, PERM_READ | PERM_WRITE, PROP_READ | PROP_WRITE | PROP_NOTIFY, "Volume Control", 0, 0, volume, 2}
};
const size_t mcs_char_count = sizeof(mcs_characteristics) / sizeof(mcs_characteristics[0]);

const ble_characteristic_t battery_characteristics[] = {
    {BATT_UUID, 0x2A19, PERM_READ, PROP_READ | PROP_NOTIFY, "Battery Level", 0, 0, &battery_level, 1}
};
const size_t battery_char_count = sizeof(battery_characteristics) / sizeof(battery_characteristics[0]);

const ble_characteristic_t hid_characteristics[] = {
    {HID_UUID, 0x2A4A, PERM_READ, PROP_READ, "HID Information", 0, 0, hid_info, sizeof(hid_info)},
    {HID_UUID, 0x2A4B, PERM_READ, PROP_READ, "HID Report Map", 0, 0, hid_report_map, sizeof(hid_report_map)},
    {HID_UUID, 0x2A4C, PERM_WRITE, PROP_WRITE, "HID Control Point", 0, 0, &hid_control_point, 1},
    {HID_UUID, 0x2A4E, PERM_READ | PERM_WRITE, PROP_READ | PROP_WRITE, "Protocol Mode", 0, 0, &protocol_mode, 1},
    {HID_UUID, 0x2A4D, PERM_READ, PROP_READ | PROP_NOTIFY, "HID Keyboard Report", 0, 0, hid_report, 8},
    {HID_UUID, 0x2A4D, PERM_READ, PROP_READ | PROP_NOTIFY, "HID Consumer Report", 0, 0, NULL, 2}
};
const size_t hid_char_count = sizeof(hid_characteristics) / sizeof(hid_characteristics[0]);

// ============================================================================
// SERVICES
// ============================================================================

const ble_service_t ble_services[SERVICE_COUNT] = {
    {MCS_UUID, MCS_APP_ID, 1, "Media Control Service", mcs_characteristics, mcs_char_count},
    {BATT_UUID, BATT_APP_ID, 1, "Battery Service", battery_characteristics, battery_char_count},
    {HID_UUID, HID_APP_ID, 1, "HID Service", hid_characteristics, hid_char_count}
};

// ============================================================================
// ADVERTISING
// ============================================================================

uint8_t HID_UUID_128[16] = {0x00, 0x00, 0x18, 0x12, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x03C1,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(HID_UUID_128),
    .p_service_uuid = HID_UUID_128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
};

esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x03C1,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
};

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

// ============================================================================
// PROFILE INITIALIZATION
// ============================================================================

void init_profile_tab(void) {
    // Forward declarations
    extern void gatts_profile_hid_event_handler(esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t*);
    extern void gatts_profile_mcs_event_handler(esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t*);
    extern void gatts_profile_bs_event_handler(esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t*);
    
    gl_profile_tab[HID_APP_ID].gatts_cb = gatts_profile_hid_event_handler;
    gl_profile_tab[HID_APP_ID].gatts_if = ESP_GATT_IF_NONE;
    
    gl_profile_tab[MCS_APP_ID].gatts_cb = gatts_profile_mcs_event_handler;
    gl_profile_tab[MCS_APP_ID].gatts_if = ESP_GATT_IF_NONE;
    
    gl_profile_tab[BATT_APP_ID].gatts_cb = gatts_profile_bs_event_handler;
    gl_profile_tab[BATT_APP_ID].gatts_if = ESP_GATT_IF_NONE;
}