#include "setupFunctions.h"
#include "ble_config.h"
#include "bluetoothFunctions.h"
#include "fileFunctions.h"
#include "menuFunctions.h"
#include "esp_pm.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"
#include "esp_err.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/vars.h"

static const char *SETUP_TAG = "SETUP";

// Forward declarations
extern bool get_isr_service_status(void);
extern void set_isr_service_installed(void);

esp_err_t i2c_master_init(void){
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = (gpio_num_t)I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_FREQ_HZ;
    
    esp_err_t err = i2c_param_config(I2C_PORT_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_set_timeout(I2C_PORT_NUM, 80000);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "I2C set timeout failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(SETUP_TAG, "I2C initialized successfully");
    return ESP_OK;
}

esp_err_t resetLEDs() {
    if (led_strip == NULL) {
        ESP_LOGE(SETUP_TAG, "LED strip not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    for (int i = 0; i < NUM_LEDS; i++) {
        esp_err_t err = led_strip_set_pixel(led_strip, i, 0, 0, 0);
        if (err != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "Failed to set LED %d: %s", i, esp_err_to_name(err));
            return err;
        }
    }
    
    esp_err_t err = led_strip_refresh(led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "Failed to refresh LED strip: %s", esp_err_to_name(err));
        return err;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(LEDS, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    ESP_LOGI(SETUP_TAG, "LEDs reset successfully");
    return ESP_OK;
}

esp_err_t setupLeds(void){
    led_strip_config_t strip_config = {
        .strip_gpio_num = LEDS,
        .max_leds = NUM_LEDS,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false}
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_APB,
        .resolution_hz = 5 * 1000 * 1000,
        .mem_block_symbols = 256,
        .flags = {.with_dma = false}
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "LED strip initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(SETUP_TAG, "LED strip initialized successfully");
    return resetLEDs();
}

esp_err_t setupPM(){
    #ifdef CONFIG_PM_ENABLE
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 160,
        .min_freq_mhz = 40,
        .light_sleep_enable = true
    };
    
    esp_err_t err = esp_pm_configure(&pm_config);
    if (err != ESP_OK) {
        ESP_LOGW(SETUP_TAG, "Power management configuration failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(SETUP_TAG, "Power management configured successfully");
    #else
    ESP_LOGI(SETUP_TAG, "Power management not enabled in menuconfig");
    #endif  // <--- ADD THIS LINE
    
    ++bootCount;
    ESP_LOGI(SETUP_TAG, "Boot number: %d", bootCount);
    
    return ESP_OK;
}

esp_err_t setupDisplay(){
    // Initialize TFT
    tft.begin();
    tft.fillScreen(TFT_BLACK);
    
    // Initialize UI display
    esp_err_t err = ui_init_display();
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "UI display initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize UI
    err = ui_init();
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "UI initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(SETUP_TAG, "Display initialized successfully");
    return ESP_OK;
}

esp_err_t setupBQ27441(){
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (!bq27441Begin(I2C_PORT_NUM)) {
        ESP_LOGE(SETUP_TAG, "BQ27441 initialization failed");
        return ESP_FAIL;
    }
    
    if (!bq27441SetCapacity(CONFIG_BQ27441_DESIGN_CAPACITY)) {
        ESP_LOGE(SETUP_TAG, "BQ27441 set capacity failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(SETUP_TAG, "BQ27441 initialized successfully");
    return ESP_OK;
}

esp_err_t setupTCA8418(){
    if (!keypad.begin(0x34, I2C_PORT_NUM)) {
        ESP_LOGE(SETUP_TAG, "TCA8418 initialization failed");
        return ESP_FAIL;
    }
    
    if (!keypad.matrix(ROWS, COLS)) {
        ESP_LOGE(SETUP_TAG, "TCA8418 matrix configuration failed");
        return ESP_FAIL;
    }
    
    keypad.flush();
    keypad.disableInterrupts();
    
    ESP_LOGI(SETUP_TAG, "TCA8418 initialized successfully");
    return ESP_OK;
}

esp_err_t setupADC() {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    
    esp_err_t err = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "ADC unit initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    
    err = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "ADC channel 3 config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "ADC channel 0 config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(SETUP_TAG, "ADC initialized successfully");
    return ESP_OK;
}

esp_err_t setupMenu(){
    cJSON *menuJson = parse_json_menu("/storage/menu.json");
    if (!menuJson) {
        ESP_LOGE(SETUP_TAG, "Failed to parse menu JSON");
        return ESP_FAIL;
    }
    
    list_files();
    
    if (!parseMenu(menuJson, menuItems, menuSize)) {
        ESP_LOGE(SETUP_TAG, "Failed to parse menu structure");
        cJSON_Delete(menuJson);
        return ESP_FAIL;
    }
    
    cJSON_Delete(menuJson);

    menuStack[0].submenu = menuItems;
    menuStack[0].submenuSize = menuSize;
    
    ESP_LOGI(SETUP_TAG, "Menu initialized successfully");
    return ESP_OK;
}

esp_err_t setupEncoder(gpio_num_t pin_a, gpio_num_t pin_b, int min, int max, bool wrap_around, void (*callback)(int)) {
    if (callback == NULL) {
        ESP_LOGE(SETUP_TAG, "Encoder callback cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    rotaryEncoder.pin_a = pin_a;
    rotaryEncoder.pin_b = pin_b;
    rotaryEncoder.min_value = min;
    rotaryEncoder.max_value = max;
    rotaryEncoder.wrap_around = wrap_around;
    rotaryEncoder.value = min;
    rotaryEncoder.on_turn = callback;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }

    if (!get_isr_service_status()) {
        err = gpio_install_isr_service(0);
        if (err != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "Failed to install ISR service: %s", esp_err_to_name(err));
            return err;
        }
        set_isr_service_installed();
    }

    err = gpio_isr_handler_add(pin_a, encoder_isr_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "Failed to add ISR handler: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(SETUP_TAG, "Encoder initialized successfully");
    return ESP_OK;
}

esp_err_t setupBluetooth(void) {
    ESP_LOGI(SETUP_TAG, "Starting Bluetooth setup");
    esp_err_t ret;
    
    // ═══════════════════════════════════════════════════════════
    // CONDITIONAL CLEANUP - Only if BT was previously running
    // ═══════════════════════════════════════════════════════════
    
    esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
    esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
    
    // Only cleanup if something is actually running
    if (controller_status != ESP_BT_CONTROLLER_STATUS_IDLE || 
        bluedroid_status != ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        
        ESP_LOGI(SETUP_TAG, "Cleaning up existing BT state...");
        
        if (bluedroid_status == ESP_BLUEDROID_STATUS_ENABLED) {
            esp_bluedroid_disable();
        }
        if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
            esp_bluedroid_deinit();
        }
        
        if (controller_status == ESP_BT_CONTROLLER_STATUS_ENABLED) {
            esp_bt_controller_disable();
        }
        if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
            esp_bt_controller_deinit();
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_LOGI(SETUP_TAG, "BT cleanup complete");
    } else {
        ESP_LOGI(SETUP_TAG, "BT already in clean state, skipping cleanup");
    }
    
    // ═══════════════════════════════════════════════════════════
    // CREATE QUEUE
    // ═══════════════════════════════════════════════════════════
    
    if (ble_display_queue == NULL) {
        ble_display_queue = xQueueCreate(5, sizeof(ble_display_event_t));
        if (ble_display_queue == NULL) {
            ESP_LOGE(SETUP_TAG, "Failed to create BLE display queue!");
            return ESP_ERR_NO_MEM;
        }
    }
    
    init_profile_tab();
    init_char_handles();
    
    // ═══════════════════════════════════════════════════════════
    // NVS INITIALIZATION
    // ═══════════════════════════════════════════════════════════
    
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "NVS erase failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // ═══════════════════════════════════════════════════════════
    // DON'T RELEASE MEMORY - It's already optimized at compile time!
    // Classic BT is disabled in sdkconfig (CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y)
    // ═══════════════════════════════════════════════════════════
    
    // ═══════════════════════════════════════════════════════════
    // BT CONTROLLER INITIALIZATION
    // ═══════════════════════════════════════════════════════════
    
    ESP_LOGI(SETUP_TAG, "Initializing BT controller...");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        ESP_LOGE(SETUP_TAG, "Controller status: %d", esp_bt_controller_get_status());
        return ret;
    }
    
    ESP_LOGI(SETUP_TAG, "Enabling BT controller for BLE...");
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // ═══════════════════════════════════════════════════════════
    // BLUEDROID STACK
    // ═══════════════════════════════════════════════════════════
    
    ESP_LOGI(SETUP_TAG, "Initializing Bluedroid...");
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(SETUP_TAG, "Enabling Bluedroid...");
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // ═══════════════════════════════════════════════════════════
    // REGISTER CALLBACKS
    // ═══════════════════════════════════════════════════════════
    
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "GATTS callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(SETUP_TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // ═══════════════════════════════════════════════════════════
    // SECURITY PARAMETERS
    // ═══════════════════════════════════════════════════════════
    
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IO; 
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    
    // ═══════════════════════════════════════════════════════════
    // REGISTER SERVICES
    // ═══════════════════════════════════════════════════════════
    
    if (device_mode == MODE_HID_ONLY || device_mode == MODE_BOTH) {
        ret = esp_ble_gatts_app_register(HID_APP_ID);
        if (ret != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "HID app register failed: %s", esp_err_to_name(ret));
            return ret;
        }
        
        ret = esp_ble_gatts_app_register(BATT_APP_ID);
        if (ret != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "BATT app register failed: %s", esp_err_to_name(ret));
            return ret;
        }
        
        ret = esp_ble_gatts_app_register(MCS_APP_ID);
        if (ret != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "MCS app register failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // MCS SCANNING
    // ═══════════════════════════════════════════════════════════
    
    if (device_mode == MODE_MCS_ONLY) {
        ESP_LOGI(SETUP_TAG, "MCS_ONLY mode: Starting BLE scan for MCS server...");
        
        static esp_ble_scan_params_t scan_params = {
            .scan_type = BLE_SCAN_TYPE_ACTIVE,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
            .scan_interval = 0x50,
            .scan_window = 0x30,
            .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
        };
        
        ret = esp_ble_gap_set_scan_params(&scan_params);
        if (ret != ESP_OK) {
            ESP_LOGE(SETUP_TAG, "Set scan params failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret != ESP_OK) {
        ESP_LOGW(SETUP_TAG, "Set local MTU failed: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(SETUP_TAG, "Bluetooth setup completed successfully");
    return ESP_OK;
}