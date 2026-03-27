/*
ESP_MKB - by B Kodden
*/

#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unordered_map>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include <Arduino.h>
#include <Wire.h>

#include "esp_littlefs.h"
#include "led_strip.h"
#include "bq27441.h"
#include "Adafruit_TCA8418.h"
#include <TFT_eSPI.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#include "icons.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/vars.h"
#include "ui/images.h"
#include "pinDef.h"
#include "init.h"
#include "ble_config.h"
#include "fileFunctions.h"
#include "menuFunctions.h"
#include "bluetoothFunctions.h"
#include "mcs_client.h"
#include "functions.h"
#include "setupFunctions.h"

#define LOADING_TASK_STACK 8192
#define LOADING_TASK_PRIO  5
#define LOADING_TASK_CORE  1

static const char *MAIN_TAG = "ESP_MKB";

static void bar_anim_cb(void *var, int32_t val) {
    lv_bar_set_value((lv_obj_t*)var, val, LV_ANIM_OFF);
}

static volatile bool loading_ui_task_running = false;
static TaskHandle_t loading_ui_task_handle = nullptr;

static void loading_ui_task(void *param) {
    while (loading_ui_task_running) {
        ui_tick();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(nullptr);
}

static inline void update_loading_status(const char *msg) {
    set_var_loading_status(msg);
}

static bool anim_started = false;
static lv_anim_t charge_anim;
static uint32_t last_mcs_check = 0;
static uint32_t last_progress_update = 0;

extern "C" {
    void app_main(void);
};

static void handle_init_error(const char* component, esp_err_t err) {
    ESP_LOGE(MAIN_TAG, "FATAL: %s initialization failed: %s", component, esp_err_to_name(err));

    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.printf("INIT ERROR:\n%s\n%s", component, esp_err_to_name(err));
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGE(MAIN_TAG, "Restarting in 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
}

static void stop_loading_task() {
    if (!loading_ui_task_running) return;

    loading_ui_task_running = false;
    if (loading_ui_task_handle) {
        while (eTaskGetState(loading_ui_task_handle) != eDeleted) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        loading_ui_task_handle = nullptr;
    }
}

void app_main(){
    esp_log_level_set("gpio", ESP_LOG_WARN);

    ESP_LOGI(MAIN_TAG, "=== ESP_MKB Starting ===");
    ESP_LOGI(MAIN_TAG, "Initializing Bluetooth");
    device_mode = MODE_BOTH;
    esp_err_t ret = setupBluetooth();
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Bluetooth init failed!");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }
    tft.begin();
    tft.fillScreen(TFT_BLACK);

    ui_init_display();
    ui_init();

    lv_scr_load(objects.loading);
    update_loading_status("Starting...");

    loading_ui_task_running = true;
    xTaskCreatePinnedToCore(
        loading_ui_task,
        "lvgl_loading",
        LOADING_TASK_STACK,
        nullptr,
        LOADING_TASK_PRIO,
        &loading_ui_task_handle,
        LOADING_TASK_CORE
    );

    vTaskDelay(pdMS_TO_TICKS(100));

    update_loading_status("GPIO...");
    ESP_LOGI(MAIN_TAG, "Configuring GPIO pins");
    pinMode(SLP_IO, OUTPUT);
    digitalWrite(SLP_IO, HIGH);
    pinMode(POT_1, INPUT);
    pinMode(POT_2, INPUT);
    pinMode(CHRG, INPUT);
    pinMode(PWR, INPUT);
    pinMode(DONE, INPUT);
    pinMode(MINT, INPUT);
    pinMode(MRES, OUTPUT);
    pinMode(SPI_BL, OUTPUT);
    resetPin(MRES);

    update_loading_status("LED strip...");
    ESP_LOGI(MAIN_TAG, "Initializing LED strip");
    ret = setupLeds();
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "LED strip init failed: %s", esp_err_to_name(ret));
    } else {
        setPixelColor(led_strip, 10, colorsDim[RED]);
        led_strip_refresh(led_strip);
    }

    update_loading_status("Power mgmt...");
    ESP_LOGI(MAIN_TAG, "Configuring power management");
    ret = setupPM();
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "Power management failed: %s", esp_err_to_name(ret));
    }

    update_loading_status("I2C bus...");
    ESP_LOGI(MAIN_TAG, "Initializing I2C bus");
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        handle_init_error("I2C", ret);
        return;
    }

    update_loading_status("ADC...");
    ESP_LOGI(MAIN_TAG, "Initializing ADC");
    ret = setupADC();
    if (ret != ESP_OK) {
        handle_init_error("ADC", ret);
        return;
    }

    update_loading_status("Battery...");
    ESP_LOGI(MAIN_TAG, "Initializing battery gauge");
    ret = setupBQ27441();
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "Battery gauge failed: %s", esp_err_to_name(ret));
    }

    update_loading_status("Keypad...");
    ESP_LOGI(MAIN_TAG, "Initializing keypad");
    ret = setupTCA8418();
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "Keypad failed: %s", esp_err_to_name(ret));
    }

    update_loading_status("Bluetooth...");
    device_mode = MODE_BOTH;
    ESP_LOGI(MAIN_TAG, "Device Mode: MCS & HID (Both)");
    ESP_LOGI(MAIN_TAG, "Initializing Bluetooth");
    ret = setupBluetooth();
    if (ret != ESP_OK) {
        handle_init_error("Bluetooth", ret);
        return;
    }

    update_loading_status("Arduino...");
    ESP_LOGI(MAIN_TAG, "Initializing Arduino framework");
    initArduino();

    update_loading_status("File system...");
    ESP_LOGI(MAIN_TAG, "Mounting file system");
    ret = setup_littlefs();
    if (ret != ESP_OK) {
        handle_init_error("LittleFS", ret);
        return;
    }

    update_loading_status("Config...");
    copy_embedded_json_to_littlefs();
    loadButtonMappings();

    update_loading_status("Menu...");
    ESP_LOGI(MAIN_TAG, "Loading menu");
    ret = setupMenu();
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "Menu setup failed: %s", esp_err_to_name(ret));
    } else {
        update_buttonset_label();
    }

    update_loading_status("Encoder...");
    ESP_LOGI(MAIN_TAG, "Initializing encoder");
    ret = setupEncoder(ENC_A, ENC_B, 0, 100, true, encTurn);
    if (ret != ESP_OK) {
        ESP_LOGW(MAIN_TAG, "Encoder failed: %s", esp_err_to_name(ret));
    }

    update_loading_status("MCS...");
    if (device_mode == MODE_MCS_ONLY || device_mode == MODE_BOTH) {
        ESP_LOGI(MAIN_TAG, "Initializing MCS client");
        mcs_client_init();
    }

    current_volume = 50;
    battery_level = 100;

    update_loading_status("Ready!");
    vTaskDelay(pdMS_TO_TICKS(300));

    if (led_strip) {
        setPixelColor(led_strip, 10, colorsDim[GREEN]);
        led_strip_refresh(led_strip);
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);

    ESP_LOGI(MAIN_TAG, "Switching to main screen");
    lv_scr_load(objects.main);

    stop_loading_task();

    ESP_LOGI(MAIN_TAG, "=== Initialization Complete ===");
    ESP_LOGI(MAIN_TAG, "Entering main loop");

    while(1) {
        lv_timer_handler();
        ui_tick();
        handle_ble_display_events();

        if (ble_conn && (device_mode == MODE_MCS_ONLY || device_mode == MODE_BOTH)) {
            mcs_process_ui_updates();
        }

        static bool last_ble_conn = false;
        if (ble_conn != last_ble_conn) {
            last_ble_conn = ble_conn;
            if (objects.bticon) {
                lv_image_set_src(objects.bticon, ble_conn ? &img_bt_on : &img_bt_off);
            }
        }

        if (device_mode == MODE_BOTH) {
            uint32_t now = esp_timer_get_time() / 1000000;

            if (now - last_mcs_check >= 10) {
                last_mcs_check = now;

                bool mcs_connected = mcs_is_connected();

                static bool last_mcs_status = false;
                if (mcs_connected != last_mcs_status) {
                    last_mcs_status = mcs_connected;

                    if (mcs_connected) {
                        ESP_LOGI(MAIN_TAG, "✓ MCS connected");
                        if (objects.mcs_nc) lv_obj_add_flag(objects.mcs_nc, LV_OBJ_FLAG_HIDDEN);
                        if (objects.mediabox) lv_obj_remove_flag(objects.mediabox, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        ESP_LOGI(MAIN_TAG, "✗ MCS disconnected");
                        if (objects.mcs_nc) lv_obj_remove_flag(objects.mcs_nc, LV_OBJ_FLAG_HIDDEN);
                        if (objects.mediabox) lv_obj_add_flag(objects.mediabox, LV_OBJ_FLAG_HIDDEN);
                    }
                }

                if (!mcs_connected && ble_conn) {
                    ESP_LOGI(MAIN_TAG, "Restarting MCS scan");
                    extern void start_mcs_scanning(void);
                    start_mcs_scanning();
                }
            }

            static uint32_t last_set_duration = 0;
            uint32_t current_duration = mcs_get_track_duration();

            if (current_duration != last_set_duration && current_duration > 0) {
                last_set_duration = current_duration;
                if (objects.trackprogress) {
                    lv_slider_set_range(objects.trackprogress, 0, current_duration);
                }
            }

            if (mcs_is_connected() && (now - last_progress_update) >= 1) {
                last_progress_update = now;
                uint32_t pos_sec = mcs_get_estimated_position();

                if (objects.trackprogress && current_duration > 0) {
                    if (pos_sec > current_duration) pos_sec = current_duration;
                    lv_slider_set_value(objects.trackprogress, pos_sec, LV_ANIM_OFF);
                }
            }

            static char last_fetched_track[128] = "";
            if (mcs_is_connected() && !mcs_is_album_transfer_in_progress()) {
                const char* current_track = mcs_get_track_title();

                if (strcmp(current_track, last_fetched_track) != 0 && strcmp(current_track, "No Media") != 0) {
                    strncpy(last_fetched_track, current_track, sizeof(last_fetched_track) - 1);
                    mcs_fetch_album_art();
                }
            }

            check_mcs_discovery();
        }

        adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &p1read);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &p2read);
        pluggedIn = digitalRead(PWR);
        chargingState = digitalRead(CHRG);
        doneCharging = digitalRead(DONE);

        static bool last_charging = false;
        static bool last_done = true;

        bool current_charging = false;
        Color led_color = colorsDim[RED];

        if (!chargingState || !doneCharging) {
            if (doneCharging == 0) {
                current_charging = false;
                led_color = colorsDim[GREEN];
            } else {
                current_charging = true;
                led_color = colorsDim[BLUE];
            }
        } else {
            current_charging = false;
            led_color = colorsDim[RED];
        }

        if (led_strip && (current_charging != last_charging || doneCharging != last_done)) {
            charging = current_charging;
            setPixelColor(led_strip, 10, led_color);
            last_charging = current_charging;
            last_done = doneCharging;
        }

        if (charging) {
            if (!anim_started && objects.batpercbar) {
                lv_anim_init(&charge_anim);
                lv_anim_set_var(&charge_anim, objects.batpercbar);
                lv_anim_set_values(&charge_anim, 0, 100);
                lv_anim_set_time(&charge_anim, 2000);
                lv_anim_set_repeat_count(&charge_anim, LV_ANIM_REPEAT_INFINITE);
                lv_anim_set_repeat_delay(&charge_anim, 500);
                lv_anim_set_exec_cb(&charge_anim, bar_anim_cb);
                lv_anim_start(&charge_anim);
                anim_started = true;
                ESP_LOGI(MAIN_TAG, "Battery charging animation started");
            }
        } else {
            if (anim_started) {
                lv_anim_delete(objects.batpercbar, nullptr);
                anim_started = false;
                ESP_LOGI(MAIN_TAG, "Battery charging animation stopped");
            }
        }

        if (abs(batPerc - bq27441Soc(FILTERED)) >= 1) {
            batPerc = (uint8_t)bq27441Soc(FILTERED);
            updateBatteryLevel(batPerc);
            update_ble_battery_level(batPerc);
            set_var_batperc(batPerc);

            if (!charging && objects.batpercbar) {
                lv_bar_set_value(objects.batpercbar, batPerc, LV_ANIM_OFF);
            }

            if (objects.batperclabel) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d%%", batPerc);
                lv_label_set_text(objects.batperclabel, buf);
            }
        }

        if (abs(p1val - p1read) >= analogDamper) {
            p1val = p1read;
            if (led_strip) {
                setLEDColorRange(p1val, 11, 0, 4100, ledsDim);
            }
        }

        if (abs(p2val - p2read) >= analogDamper) {
            p2val = p2read;
            if (led_strip) {
                setLEDColorRange(p2val, 9, 0, 4100, ledsDim);
            }
        }

        if (mcs_is_connected()) {
            static uint32_t last_audio_send = 0;
            uint32_t now = esp_timer_get_time() / 1000000;
            static uint8_t last_sent_vol = 255;
            static uint8_t last_sent_mic = 255;

            if (now - last_audio_send >= 1) {
                last_audio_send = now;

                uint8_t current_vol = (uint8_t)((p1val * 100) / 4095);
                uint8_t current_mic = (uint8_t)((p2val * 100) / 4095);

                if (abs((int)current_vol - (int)last_sent_vol) >= 1) {
                    mcs_set_volume(current_vol);
                    last_sent_vol = current_vol;
                }

                if (abs((int)current_mic - (int)last_sent_mic) >= 1) {
                    mcs_set_microphone(current_mic);
                    last_sent_mic = current_mic;
                }
            }
        }

        if (keypad.available() > 0) {
            int k = keypad.getEvent();
            bool pressed = k & 0x80;
            k &= 0x7F;
            k--;

            uint8_t row = k / 10;
            uint8_t col = k % 10;
            char key = keymap[col][row];

            if (pressed) {
                timeToSleep = awakeTime;
                keyPressed(key);
            } else {
                int ledIndex = key - 'a';
                if (ledIndex >= 0 && ledIndex <= 7 && led_strip) {
                    keysPressed[ledIndex] = 0;
                    setPixelColor(led_strip, ledIndex, colors[OFF]);
                    clear_button_highlight(key);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}