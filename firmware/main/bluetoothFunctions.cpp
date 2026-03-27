#include "bluetoothfunctions.h"
#include "mcs_client.h"
#include "ui/vars.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/images.h"

static const char *TAG = "BLE";
static bool mcs_scan_started = false;
QueueHandle_t ble_display_queue = NULL;
static bool mcs_discovery_needed = false;
static bool mcs_discovery_triggered = false;
esp_bd_addr_t hid_connected_device = {0};
bool hid_device_connected = false;
static esp_bd_addr_t mcs_target_bda;
extern QueueHandle_t ble_display_queue;


// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void clearAllBonds() {
    ESP_LOGI(TAG, "Clearing all bonded devices...");
    
    int dev_num = esp_ble_get_bond_device_num();
    
    if (dev_num > 0) {
        esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
        if (dev_list) {
            esp_ble_get_bond_device_list(&dev_num, dev_list);
            
            for (int i = 0; i < dev_num; i++) {
                esp_ble_remove_bond_device(dev_list[i].bd_addr);
                ESP_LOGI(TAG, "Removed bond: %02x:%02x:%02x:%02x:%02x:%02x",
                        dev_list[i].bd_addr[0], dev_list[i].bd_addr[1],
                        dev_list[i].bd_addr[2], dev_list[i].bd_addr[3],
                        dev_list[i].bd_addr[4], dev_list[i].bd_addr[5]);
            }
            free(dev_list);
        }
    }
    
    ESP_LOGI(TAG, "All bonds cleared");
}

void initialize_attribute_values() {
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Initializing attribute values");
    for (int s = 0; s < SERVICE_COUNT; s++) {
        const ble_service_t* service = &ble_services[s];
        
        for (int c = 0; c < service->char_count; c++) {
            const ble_characteristic_t* characteristic = &service->characteristics[c];
            
            if (characteristic->init_value != NULL && characteristic->init_length > 0) {
                uint16_t handle = char_handles[service->profile_id][c];
                
                if (handle != 0) {
                    esp_err_t status = esp_ble_gatts_set_attr_value(
                        handle, 
                        characteristic->init_length, 
                        characteristic->init_value
                    );
                    
                    if (status == ESP_OK) {
                        ESP_LOGI(TAG, "Initialized %s", characteristic->name);
                    } else {
                        ESP_LOGE(TAG, "Failed to initialize %s: %d", characteristic->name, status);
                    }
                }
            }
        }
    }
}

void setup_service(esp_gatt_if_t gatts_if, uint8_t profile_id) {
    const ble_service_t* service = NULL;
    
    for (int i = 0; i < SERVICE_COUNT; i++) {
        if (ble_services[i].profile_id == profile_id) {
            service = &ble_services[i];
            break;
        }
    }
    
    if (service == NULL) {
        ESP_LOGE(TAG, "Service not found for profile ID %d", profile_id);
        return;
    }

    gl_profile_tab[profile_id].service_id.is_primary = service->is_primary;
    gl_profile_tab[profile_id].service_id.id.inst_id = 0x00;
    gl_profile_tab[profile_id].service_id.id.uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[profile_id].service_id.id.uuid.uuid.uuid16 = service->service_uuid;
    
    uint16_t num_handles = service->char_count * 3 + 1;  // 3 handles per char (char + cccd + padding)

    esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[profile_id].service_id, num_handles);
}

void add_characteristic(esp_gatt_if_t gatts_if, uint8_t profile_id, uint8_t char_index) {
    const ble_service_t* service = NULL;
    
    for (int i = 0; i < SERVICE_COUNT; i++) {
        if (ble_services[i].profile_id == profile_id) {
            service = &ble_services[i];
            break;
        }
    }
    
    if (service == NULL || char_index >= service->char_count) {
        ESP_LOGE(TAG, "Service not found or invalid char index");
        return;
    }
    
    const ble_characteristic_t* characteristic = &service->characteristics[char_index];
    
    esp_bt_uuid_t char_uuid;
    char_uuid.len = ESP_UUID_LEN_16;
    char_uuid.uuid.uuid16 = characteristic->char_uuid;
    
    esp_attr_value_t *attr_value = NULL;
    esp_attr_value_t init_attr_value;
    
    if (characteristic->init_value != NULL && characteristic->init_length > 0) {
        init_attr_value.attr_max_len = characteristic->init_length * 2;
        init_attr_value.attr_len = characteristic->init_length;
        init_attr_value.attr_value = characteristic->init_value;
        attr_value = &init_attr_value;
    }
    
    esp_err_t ret = esp_ble_gatts_add_char(
        gl_profile_tab[profile_id].service_handle,
        &char_uuid,
        characteristic->permissions,
        characteristic->properties,
        attr_value, NULL);
        
    if (ret) {
        ESP_LOGE(TAG, "Add char %s failed, error code = %x", characteristic->name, ret);
    }
}

void handle_gatts_read_evt(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    uint16_t handle = param->read.handle;

    if (device_mode == MODE_BOTH && !mcs_discovery_triggered && ble_conn) {
        mcs_discovery_triggered = true;
        
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        ESP_LOGI(TAG, "First READ event - triggering MCS discovery");
        ESP_LOGI(TAG, "  conn_id from read: %d", param->read.conn_id);
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        
        if (param->read.conn_id != 0) {
            mcs_use_existing_connection(param->read.conn_id, gl_remote_bda);
        } else {
            ESP_LOGE(TAG, "Read conn_id also 0!");
        }
    }

    for (int p = 0; p < PROFILE_NUM; p++) {
        for (int i = 0; i < ble_services[p].char_count; i++) {
            if (char_handles[p][i] == handle) {
                const ble_characteristic_t* characteristic = &ble_services[p].characteristics[i];
                
                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
                rsp.attr_value.handle = handle;
                
                uint16_t value_len = 0;
                const uint8_t* current_value = NULL;
                esp_err_t get_status = esp_ble_gatts_get_attr_value(handle, &value_len, &current_value);
                
                if (get_status == ESP_OK && current_value != NULL) {
                    rsp.attr_value.len = value_len;
                    memcpy(rsp.attr_value.value, current_value, value_len);
                }
                
                esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                return;
            }
        }
    }
}

void handle_ble_display_events() {
    ble_display_event_t evt;
    
    if (ble_display_queue != NULL && xQueueReceive(ble_display_queue, &evt, 0) == pdTRUE) {
        switch (evt.type) {
          case DISPLAY_PASSKEY:
            ESP_LOGI(TAG, "Pairing: %s → %06d", evt.device_name, evt.passkey);
            
            pairing_in_progress = true;
            
            set_var_bledevicename(evt.device_name);
            
            char passkey_str[8];
            snprintf(passkey_str, sizeof(passkey_str), "%06d", evt.passkey);
            set_var_blepairingcode(passkey_str);
            
            if (objects.blepairingpopup) {
                lv_obj_clear_flag(objects.blepairingpopup, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case DISPLAY_SUCCESS:
        case DISPLAY_FAILED:
        case DISPLAY_CLEAR:
            pairing_in_progress = false;
            
            if (objects.blepairingpopup) {
                lv_obj_add_flag(objects.blepairingpopup, LV_OBJ_FLAG_HIDDEN);
            }
            if (objects.bticon) {
                lv_image_set_src(objects.bticon, ble_conn ? &img_bt_on : &img_bt_off);
            }
            break;
        }
    }
}

// ============================================================================
// GAP EVENT HANDLER
// ============================================================================

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~adv_config_flag);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
            
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~scan_rsp_config_flag);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
            
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed, status %d", param->adv_start_cmpl.status);
            } else {
                ESP_LOGI(TAG, "Advertising started successfully");
                
                /* if (!mcs_scan_initiated) {
                    start_mcs_scanning();
                    mcs_scan_initiated = true;
                } */
            }
            break;
            
        // ADD MCS SCANNING EVENTS
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "MCS: Scan parameters set, starting scan...");
            esp_ble_gap_start_scanning(30);
            break;
            
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "MCS: Scanning for MCS server...");
            } else {
                ESP_LOGE(TAG, "MCS: Scan start failed, status %d", param->scan_start_cmpl.status);
            }
            break;
            
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                mcs_handle_scan_result(param);
            }
            break;
            
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "MCS: Scan stopped");
            }
            break;
            
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            }
            break;
            
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "Connection params updated");
            break;
            
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            break;
            
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            if (param->ble_security.auth_cmpl.success) {
                ESP_LOGI(TAG, "✓ Authentication SUCCESS");
                
                ble_display_event_t evt = {.type = DISPLAY_SUCCESS, .passkey = 0};
                xQueueSend(ble_display_queue, &evt, 0);
                
                ble_conn = true;
                
                // Start scanning for MCS server (could be same Windows device)
                if (device_mode == MODE_BOTH) {
                    ESP_LOGI(TAG, "HID paired - now scanning for MCS server...");
                    vTaskDelay(pdMS_TO_TICKS(2000));  // Wait for connection to stabilize
                    start_mcs_scanning();
                }
            } else {
                ESP_LOGE(TAG, "✗ Authentication FAILED, reason: %d", 
                        param->ble_security.auth_cmpl.fail_reason);
                
                ble_display_event_t evt = {.type = DISPLAY_FAILED, .passkey = 0};
                xQueueSend(ble_display_queue, &evt, 0);
            }
            break;
        }
            
        case ESP_GAP_BLE_SEC_REQ_EVT:
            ESP_LOGI(TAG, "Security request received");
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
            
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
            ESP_LOGI(TAG, "═══════════════════════════════════");
            ESP_LOGI(TAG, "  PAIRING PASSKEY: %06d", param->ble_security.key_notif.passkey);
            ESP_LOGI(TAG, "═══════════════════════════════════");
            
            ble_display_event_t evt = {.type = DISPLAY_PASSKEY, .passkey = param->ble_security.key_notif.passkey};
            xQueueSend(ble_display_queue, &evt, 0);
            break;
        }
            
        case ESP_GAP_BLE_NC_REQ_EVT: {
            uint32_t passkey = param->ble_security.key_notif.passkey;
            ESP_LOGI(TAG, "Numeric comparison: %06d", passkey);
            
            ble_display_event_t evt;
            evt.type = DISPLAY_PASSKEY;
            evt.passkey = passkey;
            
            uint8_t *bda = param->ble_security.ble_req.bd_addr;
            snprintf(evt.device_name, sizeof(evt.device_name), 
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            
            memcpy(gl_remote_bda, bda, sizeof(esp_bd_addr_t));
            
            xQueueSend(ble_display_queue, &evt, 0);
            break;
        }
            
        default:
            break;
    }
}

void start_mcs_scanning(void) {
    ESP_LOGI(TAG, "Starting MCS scan...");
    
    // Stop any existing scan first
    esp_ble_gap_stop_scanning();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    static esp_ble_scan_params_t scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
    };
    
    esp_ble_gap_set_scan_params(&scan_params);
}
// ============================================================================
// GATTS PROFILE EVENT HANDLER
// ============================================================================

void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint8_t profile_id) {
    const ble_service_t* service = NULL;
    
    for (int i = 0; i < SERVICE_COUNT; i++) {
        if (ble_services[i].profile_id == profile_id) {
            service = &ble_services[i];
            break;
        }
    }
    if (service == NULL) {
        ESP_LOGE(TAG, "Service not found for profile ID %d", profile_id);
        return;
    }
    static uint8_t current_char_idx[PROFILE_NUM] = {0};
    
    switch (event) {
    case ESP_GATTS_REG_EVT:
        if (profile_id == 0) {
            esp_ble_gap_set_device_name("ESP-MKB");
            esp_ble_gap_config_adv_data(&adv_data);
            adv_config_done |= adv_config_flag;
            esp_ble_gap_config_adv_data(&scan_rsp_data);
            adv_config_done |= scan_rsp_config_flag;
        }
        setup_service(gatts_if, profile_id);
        break;
        
    case ESP_GATTS_CREATE_EVT:
        gl_profile_tab[profile_id].service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(gl_profile_tab[profile_id].service_handle);
        current_char_idx[profile_id] = 0;
        
        if (service->char_count > 0) {
            add_characteristic(gatts_if, profile_id, current_char_idx[profile_id]);
        }
        break;
        
    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG, "%s ADD_CHAR: UUID=0x%04x, handle=%d", 
                service->name, param->add_char.char_uuid.uuid.uuid16, param->add_char.attr_handle);

        if (current_char_idx[profile_id] == 0) {
            gl_profile_tab[profile_id].char_handle = param->add_char.attr_handle;
        }

        if (current_char_idx[profile_id] < 15) {
            char_handles[profile_id][current_char_idx[profile_id]] = param->add_char.attr_handle;
            ESP_LOGI(TAG, "→ Mapped handle %d to %s", param->add_char.attr_handle, 
                    service->characteristics[current_char_idx[profile_id]].name);
        }

        // ═══════════════════════════════════════════════════════════════════
        // ADD REPORT REFERENCE DESCRIPTOR FOR HID REPORTS
        // ═══════════════════════════════════════════════════════════════════
        
        if (profile_id == HID_APP_ID && param->add_char.char_uuid.uuid.uuid16 == 0x2A4D) {
            // This is a HID Report characteristic - add Report Reference descriptor
            
            ESP_LOGI(TAG, "→ Adding Report Reference for char index %d", current_char_idx[profile_id]);
            
            static uint8_t keyboard_report_ref[2] = {0x01, 0x01};  // Report ID 1, Input Report
            static uint8_t consumer_report_ref[2] = {0x02, 0x01};   // Report ID 2, Input Report
            
            uint8_t *report_ref = NULL;
            
            if (current_char_idx[profile_id] == 4) {
                // Keyboard Report (index 4)
                report_ref = keyboard_report_ref;
                ESP_LOGI(TAG, "  → Keyboard Report: ID=1");
            } else if (current_char_idx[profile_id] == 5) {
                // Consumer Control Report (index 5)
                report_ref = consumer_report_ref;
                ESP_LOGI(TAG, "  → Consumer Report: ID=2");
            }
            
            if (report_ref != NULL) {
                esp_bt_uuid_t descr_uuid;
                descr_uuid.len = ESP_UUID_LEN_16;
                descr_uuid.uuid.uuid16 = 0x2908;  // Report Reference descriptor UUID
                
                // *** KEY FIX: Add control structure with auto_rsp ***
                esp_attr_control_t control;
                memset(&control, 0, sizeof(control));
                control.auto_rsp = ESP_GATT_AUTO_RSP;  // <-- THIS IS CRITICAL!
                
                esp_attr_value_t descr_val = {
                    .attr_max_len = 2,
                    .attr_len = 2,
                    .attr_value = report_ref
                };
                
                esp_err_t ret = esp_ble_gatts_add_char_descr(
                    gl_profile_tab[profile_id].service_handle,
                    &descr_uuid,
                    ESP_GATT_PERM_READ,
                    &descr_val,
                    &control);  // <-- Pass the control structure!
                
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "  ✓ Report Reference added with auto-response");
                } else {
                    ESP_LOGE(TAG, "  ✗ Report Reference add failed: 0x%x", ret);
                }
            }
        }

        // ═══════════════════════════════════════════════════════════════════
        // ADD CCCD FOR ALL CHARACTERISTICS WITH NOTIFY/INDICATE
        // ═══════════════════════════════════════════════════════════════════
        
        uint16_t properties = service->characteristics[current_char_idx[profile_id]].properties;
        bool needs_cccd = (properties & PROP_NOTIFY) || (properties & PROP_INDICATE);
        
        if (needs_cccd) {
            ESP_LOGI(TAG, "→ Adding CCCD for UUID 0x%04x", param->add_char.char_uuid.uuid.uuid16);
            
            esp_bt_uuid_t descr_uuid;
            descr_uuid.len = ESP_UUID_LEN_16;
            descr_uuid.uuid.uuid16 = 0x2902;  // CCCD UUID

            uint8_t cccd_value[2] = {0x00, 0x00};
            esp_attr_control_t control;
            memset(&control, 0, sizeof(control));
            control.auto_rsp = ESP_GATT_AUTO_RSP;

            esp_attr_value_t descr_val = {
                .attr_max_len = 2,
                .attr_len = 2,
                .attr_value = cccd_value
            };

            esp_err_t ret = esp_ble_gatts_add_char_descr(
                gl_profile_tab[profile_id].service_handle,
                &descr_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                &descr_val,
                &control);
            
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ CCCD added");
            } else {
                ESP_LOGE(TAG, "✗ CCCD add failed: 0x%x", ret);
            }
        }

        current_char_idx[profile_id]++;
        if (current_char_idx[profile_id] < service->char_count) {
            add_characteristic(gatts_if, profile_id, current_char_idx[profile_id]);
        }
        break;
    }

    case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
        uint16_t descr_uuid = param->add_char_descr.descr_uuid.uuid.uuid16;
        uint16_t handle = param->add_char_descr.attr_handle;
        
        ESP_LOGI(TAG, "Descriptor added: UUID=0x%04x, handle=%d", descr_uuid, handle);
        
        if (descr_uuid == 0x2908) {
            ESP_LOGI(TAG, "  ✓ Report Reference descriptor (auto-response enabled)");
        } else if (descr_uuid == 0x2902) {
            ESP_LOGI(TAG, "  ✓ CCCD (auto-response enabled)");
        }
        break;
    }
        
    case ESP_GATTS_WRITE_EVT:
        if (param->write.len == 2) {
            uint16_t descr_value = param->write.value[0] | (param->write.value[1] << 8);
            if (descr_value == 0x0001) {
                ESP_LOGI(TAG, "✓ Notifications enabled for handle %d", param->write.handle - 1);
            }
        }

        if (profile_id == MCS_APP_ID) {
            for (int i = 0; i < service->char_count; i++) {
                if (char_handles[profile_id][i] == param->write.handle) {
                    uint16_t char_uuid = service->characteristics[i].char_uuid;

                    if (char_uuid == TRACK_TITLE && param->write.len > 0) {
                        size_t copy_len = param->write.len < sizeof(track_title) - 1 ? 
                                         param->write.len : sizeof(track_title) - 1;
                        memcpy(track_title, param->write.value, copy_len);
                        track_title[copy_len] = '\0';
                    }
                    break;
                }
            }
        }
        
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }
        break;
        
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "%s READ: handle=%d", service->name, param->read.handle);
        handle_gatts_read_evt(event, gatts_if, param);
        break;
        
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        ESP_LOGI(TAG, "GATTS CONNECT EVENT");
        ESP_LOGI(TAG, "  conn_id: %d", param->connect.conn_id);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->connect.remote_bda, 6, ESP_LOG_INFO);
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        
        for (int i = 0; i < PROFILE_NUM; i++) {
            gl_profile_tab[i].conn_id = param->connect.conn_id;
            ESP_LOGI(TAG, "Set profile[%d].conn_id = %d", i, param->connect.conn_id);
        }
        ble_conn = true;
        
        ESP_LOGI(TAG, "Device connected, requesting pairing...");
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        break;
        
    case ESP_GATTS_DISCONNECT_EVT:
        for (int i = 0; i < PROFILE_NUM; i++) {
            gl_profile_tab[i].conn_id = 0;
        }
        ble_conn = false;
        esp_ble_gap_start_advertising(&adv_params);
        break;
        
    case ESP_GATTS_START_EVT:
        services_started_count++;
        ESP_LOGI(TAG, "✓ %s service started", service->name);
        
        if (profile_id == HID_APP_ID) {
            ESP_LOGI(TAG, "━━━ HID Service Characteristics ━━━");
            for (int i = 0; i < service->char_count; i++) {
                ESP_LOGI(TAG, "  %d. %s (0x%04x) → handle %d", 
                         i+1, service->characteristics[i].name,
                         service->characteristics[i].char_uuid,
                         char_handles[HID_APP_ID][i]);
            }
            ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        }
        
        if (services_started_count == SERVICE_COUNT) {
            all_services_started = true;
            initialize_attribute_values();
        }
        break;
        
    default:
        break;
    }
}

// ============================================================================
// SERVICE-SPECIFIC EVENT HANDLERS
// ============================================================================

void gatts_profile_mcs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event != ESP_GATTS_CONNECT_EVT) {
        gatts_profile_event_handler(event, gatts_if, param, MCS_APP_ID);
    }
}

void gatts_profile_hid_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event != ESP_GATTS_CONNECT_EVT) {
        gatts_profile_event_handler(event, gatts_if, param, HID_APP_ID);
    }
}

void gatts_profile_bs_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event != ESP_GATTS_CONNECT_EVT) {
        gatts_profile_event_handler(event, gatts_if, param, BATT_APP_ID);
    }
}

// ============================================================================
// MAIN GATTS EVENT HANDLER
// ============================================================================

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    if (event == ESP_GATTS_CONNECT_EVT) {
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        ESP_LOGI(TAG, "MAIN GATTS_CONNECT_EVT");
        ESP_LOGI(TAG, "  conn_id: %d", param->connect.conn_id);
        ESP_LOGI(TAG, "  gatts_if: %d", gatts_if);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->connect.remote_bda, 6, ESP_LOG_INFO);
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        
        uint16_t conn_id = param->connect.conn_id;
        for (int i = 0; i < PROFILE_NUM; i++) {
            gl_profile_tab[i].conn_id = conn_id;
            ESP_LOGI(TAG, "  Set profile[%d].conn_id = %d", i, conn_id);
        }
        memcpy(hid_connected_device, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        hid_device_connected = true;
        ble_conn = true;
    }
    else if (event == ESP_GATTS_DISCONNECT_EVT) {
        ESP_LOGI(TAG, "MAIN GATTS_DISCONNECT_EVT");
        for (int i = 0; i < PROFILE_NUM; i++) {
            gl_profile_tab[i].conn_id = 0;
            ESP_LOGI(TAG, "  Cleared profile[%d].conn_id", i);
        }
        ble_conn = false;
        mcs_discovery_triggered = false;
        hid_device_connected = false;
        memset(hid_connected_device, 0, sizeof(esp_bd_addr_t));
        esp_ble_gap_start_advertising(&adv_params);
    }

    for (int idx = 0; idx < PROFILE_NUM; idx++) {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab[idx].gatts_if) {
            if (gl_profile_tab[idx].gatts_cb) {
                gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

// ============================================================================
// UPDATE FUNCTIONS
// ============================================================================

void update_ble_battery_level(uint8_t level) {
    battery_level = level;
    if (ble_conn) {
        uint16_t char_handle = 0;
        for (int i = 0; i < ble_services[BATT_APP_ID].char_count; i++) {
            if (ble_services[BATT_APP_ID].characteristics[i].char_uuid == 0x2A19) {
                char_handle = char_handles[BATT_APP_ID][i];
                break;
            }
        }
        
        if (char_handle != 0) {
            esp_ble_gatts_send_indicate(
                gl_profile_tab[BATT_APP_ID].gatts_if,
                gl_profile_tab[BATT_APP_ID].conn_id,
                char_handle,
                sizeof(level), &level, false);
        }
    }
}

void send_consumer_key(uint16_t key_code) {
    if (!ble_conn) {
        ESP_LOGW(TAG, "Not connected, can't send key");
        return;
    }
    
    uint16_t char_handle = char_handles[HID_APP_ID][5];
    
    if (char_handle == 0) {
        ESP_LOGE(TAG, "HID Consumer Report handle not found!");
        return;
    }
    
    // PRESS
    uint8_t report[2] = {
        (uint8_t)(key_code & 0xFF),
        (uint8_t)(key_code >> 8)
    };
    
    ESP_LOGI(TAG, "▼ PRESS consumer key: 0x%04X", key_code);
    ESP_LOG_BUFFER_HEX(TAG, report, 2);
    
    esp_err_t ret = esp_ble_gatts_send_indicate(
        gl_profile_tab[HID_APP_ID].gatts_if,
        gl_profile_tab[HID_APP_ID].conn_id,
        char_handle,
        2,
        report,
        false);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send press: 0x%x", ret);
        return;
    }
    
    // RELEASE
    vTaskDelay(pdMS_TO_TICKS(50));
    
    uint8_t release[2] = {0x00, 0x00};
    
    ESP_LOGI(TAG, "▲ RELEASE consumer key");
    
    ret = esp_ble_gatts_send_indicate(
        gl_profile_tab[HID_APP_ID].gatts_if,
        gl_profile_tab[HID_APP_ID].conn_id,
        char_handle,
        2,
        release,
        false);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send release: 0x%x", ret);
    }
}

void send_keyboard_key(uint8_t key_code, uint8_t modifier) {
    if (!ble_conn) {
        ESP_LOGW(TAG, "Not connected, can't send key");
        return;
    }
    
    // Find HID Keyboard Report handle (5th characteristic - index 4)
    uint16_t char_handle = char_handles[HID_APP_ID][4];
    
    if (char_handle == 0) {
        ESP_LOGE(TAG, "HID Keyboard Report handle not found!");
        return;
    }
    
    // Build keyboard report: [modifier, reserved, key1-6]
    uint8_t report[8] = {modifier, 0x00, key_code, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    ESP_LOGI(TAG, "Sending keyboard key: 0x%02X (modifier: 0x%02X)", key_code, modifier);
    
    esp_err_t ret = esp_ble_gatts_send_indicate(
        gl_profile_tab[HID_APP_ID].gatts_if,
        gl_profile_tab[HID_APP_ID].conn_id,
        char_handle,
        8,
        report,
        false);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send keyboard key: 0x%x", ret);
    }
    
    // Release key
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t release[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    esp_ble_gatts_send_indicate(
        gl_profile_tab[HID_APP_ID].gatts_if,
        gl_profile_tab[HID_APP_ID].conn_id,
        char_handle,
        8,
        release,
        false);
}

void update_volume(uint8_t new_volume) {
    current_volume = new_volume;
    
    uint16_t char_handle = 0;
    for (int i = 0; i < ble_services[MCS_APP_ID].char_count; i++) {
        if (ble_services[MCS_APP_ID].characteristics[i].char_uuid == VOLUME_CONTROL) {
            char_handle = char_handles[MCS_APP_ID][i];
            break;
        }
    }
    
    if (char_handle == 0) return;
    
    uint8_t data[2] = {MCS_OP_SET_VOLUME, current_volume};
    esp_ble_gatts_set_attr_value(char_handle, 2, data);
    
    if (ble_conn) {
        esp_ble_gatts_send_indicate(
            gl_profile_tab[MCS_APP_ID].gatts_if,
            gl_profile_tab[MCS_APP_ID].conn_id,
            char_handle, 2, data, false);
    }
}

void check_mcs_discovery(void) {
    static uint32_t last_attempt = 0;
    static int attempt_count = 0;
    
    if (!mcs_discovery_needed || !ble_conn) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;  // milliseconds
    
    // Try every 2 seconds, max 5 attempts
    if (now - last_attempt > 2000 && attempt_count < 5) {
        last_attempt = now;
        attempt_count++;
        
        uint16_t conn_id = 0;
        for (int i = 0; i < PROFILE_NUM; i++) {
            if (gl_profile_tab[i].conn_id != 0) {
                conn_id = gl_profile_tab[i].conn_id;
                break;
            }
        }
        
        if (conn_id != 0) {
            ESP_LOGI("BLE", "═══════════════════════════════════════");
            ESP_LOGI("BLE", "Attempt %d: Starting MCS discovery", attempt_count);
            ESP_LOGI("BLE", "  conn_id: %d", conn_id);
            ESP_LOGI("BLE", "═══════════════════════════════════════");
            
            mcs_use_existing_connection(conn_id, mcs_target_bda);
            mcs_discovery_needed = false;  // Done trying
            attempt_count = 0;
        } else {
            ESP_LOGW("BLE", "Attempt %d: conn_id still 0, will retry...", attempt_count);
        }
    }
    
    // Give up after 5 attempts
    if (attempt_count >= 5) {
        ESP_LOGE("BLE", "Gave up on MCS discovery after 5 attempts");
        mcs_discovery_needed = false;
        attempt_count = 0;
    }
}