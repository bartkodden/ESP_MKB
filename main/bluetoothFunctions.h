#ifndef BLUETOOTHFUNCTIONS_H
#define BLUETOOTHFUNCTIONS_H

#include "init.h"
#include "ble_config.h"

// BLE Display Event Types
typedef enum {
    DISPLAY_PASSKEY,
    DISPLAY_SUCCESS,
    DISPLAY_FAILED,
    DISPLAY_CLEAR
} ble_display_event_type_t;

typedef struct {
    ble_display_event_type_t type;
    uint32_t passkey;
    char device_name[32];
} ble_display_event_t;

extern QueueHandle_t ble_display_queue; 

extern esp_bd_addr_t hid_connected_device;
extern bool hid_device_connected;

// Function declarations
void clearAllBonds(void);
void initialize_attribute_values(void);
void handle_gatts_read_evt(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
esp_err_t gatts_process_command(uint8_t profile_id, uint8_t command, const uint8_t *value, uint16_t value_len);
void update_ble_battery_level(uint8_t level);
void send_consumer_key(uint16_t key_code);
void send_keyboard_key(uint8_t key_code, uint8_t modifier = 0);
void update_volume(uint8_t new_volume);
void start_mcs_scanning(void);
void handle_ble_display_events();

// Event handlers
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_mcs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_hid_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_bs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Helper functions
void setup_service(esp_gatt_if_t gatts_if, uint8_t profile_id);
void add_characteristic(esp_gatt_if_t gatts_if, uint8_t profile_id, uint8_t char_index);
void check_mcs_discovery(void);

#endif // BLUETOOTHFUNCTIONS_H