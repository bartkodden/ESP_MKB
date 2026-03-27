#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include "init.h"

// HID Report Map
extern uint8_t hid_report_map[];
extern const size_t hid_report_map_len;

// HID Descriptors
extern uint8_t hid_info[4];
extern uint8_t hid_control_point;
extern uint8_t protocol_mode;

// Service Definitions
extern const ble_characteristic_t mcs_characteristics[];
extern const size_t mcs_char_count;
extern const ble_characteristic_t battery_characteristics[];
extern const size_t battery_char_count;
extern const ble_characteristic_t hid_characteristics[];
extern const size_t hid_char_count;

extern const ble_service_t ble_services[SERVICE_COUNT];

// Advertising
extern uint8_t HID_UUID_128[16];
extern esp_ble_adv_data_t adv_data;
extern esp_ble_adv_data_t scan_rsp_data;
extern esp_ble_adv_params_t adv_params;

// Profile initialization
void init_profile_tab(void);

#endif // BLE_CONFIG_HC