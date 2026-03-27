#include "functions.h"
#include "bluetoothfunctions.h"
#include "fileFunctions.h"
#include "menuFunctions.h"
#include "setupfunctions.h"
#include <esp_sleep.h>
#include "ui/screens.h"
#include "ui/vars.h"
#include "ui/ui.h"
#include "ui/styles.h"
#include "lvgl.h"  

static const char *TAG = "FUNC";

// Define BAS_IDX_BATT_LVL_VAL if not already defined
#ifndef BAS_IDX_BATT_LVL_VAL
#define BAS_IDX_BATT_LVL_VAL 2
#endif

#define TIME_TO_SLEEP 60

void resetPin(uint8_t pin) {
  digitalWrite(pin, LOW);
  delay(100);
  digitalWrite(pin, HIGH);
  delay(100);
}

void handleVolumeControl(uint8_t volume) {
    if (volume > 100) {
        volume = 100;
    }
}

void setPixelColor(led_strip_handle_t led_strip, int pixel_index, const Color& color) {
  led_strip_set_pixel(led_strip, pixel_index, color.red, color.green, color.blue);
  led_strip_refresh(led_strip);
}

void setPixelRange(led_strip_handle_t led_strip, int pixel_index_start, int pixel_index_end, const Color& color) {
  for (int i = pixel_index_start; i <= pixel_index_end; i++) {
    led_strip_set_pixel(led_strip, i, color.red, color.green, color.blue);
  }
  led_strip_refresh(led_strip);
}

void setLEDColorRange(int potValue, int ledIndex, int minVal, int maxVal, int bright) {
  int greenValue = map(potValue, minVal, maxVal, 0, bright);
  int redValue = bright - greenValue;
  led_strip_set_pixel(led_strip, ledIndex, redValue, greenValue, 0);
  led_strip_refresh(led_strip);
}

void encTurn(int value) {
    // Encoder turn callback (currently empty)
}

int readEncoder() {
  if(encoder_delta > 1) {
    if(rotaryEncoder.value >= rotaryEncoder.max_value){
      rotaryEncoder.value = rotaryEncoder.min_value;
    }else{
      rotaryEncoder.value += 1;
    }
    encoder_delta = 0;
  }else if(encoder_delta < -1){
    if(rotaryEncoder.value <= rotaryEncoder.min_value){
      rotaryEncoder.value = rotaryEncoder.max_value;
    }else{
      rotaryEncoder.value -= 1;
    }
    encoder_delta = 0;
  }
  return rotaryEncoder.value;
}

int readAnalogValue(adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel) {
    int total = 0;
    int value;
    for (int i = 0; i < 10; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &value));
        total += value;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return total / 10;
}

void printBatteryStats(){
    unsigned int _soc = bq27441Soc(FILTERED);
    unsigned int _volts = bq27441Voltage();
    int _current = bq27441Current(AVG);
    unsigned int _fullCapacity = bq27441Capacity(FULL);
    unsigned int _capacity = bq27441Capacity(REMAIN);
    int _power = bq27441Power();
    int _health = bq27441Soh(PERCENT);
    printf("Battery SOC %i%% VOLTS %imV CURRENT %imA CAPACITY %i / %imAh\n",
           _soc, _volts, _current, _capacity, _fullCapacity);
    printf("Battery HEALTH %i%% POWER %imW\n", _health, _power);
}

void updateChargingStatus(bool charging) {
    // TODO: Implement charging status update
}

void updateBatteryLevel(uint8_t level) {
    if (level > 100) {
        level = 100;
    }
    // Level is clamped, actual update handled elsewhere
}

void checkSleepMode(int64_t& lastTimeCheck) {
  int64_t currentTime = esp_timer_get_time() / 1000000;
  int64_t elapsedTime = (currentTime - lastTimeCheck);
  lastTimeCheck = currentTime;
  
  if (elapsedTime >= 1) {
    timeToSleep--;
    //displayTimeToSleep(timeToSleep);

    if (timeToSleep <= 0) {
        setPixelRange(led_strip, 0, 18, colorsDim[OFF]);
        setPixelColor(led_strip, 10, colorsDim[CYAN]);
        led_strip_refresh(led_strip);
        delay(1000);
        esp_deep_sleep_start();
    }
  }
}

void vOff(){
  setPixelRange(led_strip, 0, NUM_LEDS - 1, colorsDim[OFF]);
  led_strip_refresh(led_strip);
  digitalWrite(SLP_IO, LOW);
}

void vOn(){
  digitalWrite(SLP_IO, HIGH);
}

// ═══════════════════════════════════════════════════════════
// HELPER: Check if key is mapped button
// ═══════════════════════════════════════════════════════════
static bool is_mapped_button(char key) {
    return (key >= 'a' && key <= 'h');
}

// ═══════════════════════════════════════════════════════════
// HELPER: Execute button command
// ═══════════════════════════════════════════════════════════
static bool execute_button_command(char key) {
    ButtonMapping* buttonMappings = getActiveButtonMappings();

    for (int i = 0; i < 8; i++) {
        if (buttonMappings[i].key_id != key) continue;
        
        uint16_t cmd = buttonMappings[i].cmd;
        if (cmd == 0) return false;
        
        ESP_LOGI(TAG, "Executing command: %d for key: %c", cmd, key);
        
        // Consumer control (media keys)
        if ((cmd >= 176 && cmd <= 188) || cmd == 205 || cmd == 206 || 
            cmd == 226 || cmd == 233 || cmd == 234) {
            ESP_LOGI(TAG, "→ CONSUMER key: 0x%04X", cmd);
            send_consumer_key(cmd);
            return true;
        }
        
        // Regular keyboard keys
        if (cmd >= 4 && cmd <= 101) {
            ESP_LOGI(TAG, "→ KEYBOARD key: 0x%02X", cmd);
            send_keyboard_key(cmd, 0);
            return true;
        }
        
        // Modifier keys
        if (cmd >= 224 && cmd <= 231) {
            uint8_t modifier = 1 << (cmd - 224);
            ESP_LOGI(TAG, "→ MODIFIER: 0x%02X", modifier);
            send_keyboard_key(0, modifier);
            return true;
        }
        
        ESP_LOGW(TAG, "❌ Unrecognized command: %d", cmd);
        return true;
    }
    
    return false;  // Key not mapped
}

void highlight_button(char key) {
    int ledIndex = key - 'a';
    if (ledIndex < 0 || ledIndex > 7) return;
    
    // Map key to button matrix and button index
    lv_obj_t *matrix = (ledIndex < 4) ? objects.bm0 : objects.bm1;
    uint16_t btn_id = ledIndex % 4;
    
    if (matrix) {
        lv_buttonmatrix_clear_button_ctrl_all(matrix, LV_BUTTONMATRIX_CTRL_CHECKED);
        lv_buttonmatrix_set_button_ctrl(matrix, btn_id, LV_BUTTONMATRIX_CTRL_CHECKED);
        ESP_LOGD(TAG, "Highlighted button %d on matrix %s", btn_id, 
                 (ledIndex < 4) ? "bm0" : "bm1");
    }
    ui_tick();
}

// CLEAR HIGHLIGHT on release
void clear_button_highlight(char key) {
    int ledIndex = key - 'a';
    if (ledIndex < 0 || ledIndex > 7) return;
    
    lv_obj_t *matrix = (ledIndex < 4) ? objects.bm0 : objects.bm1;
    uint16_t btn_id = ledIndex % 4;
    
    if (matrix) {
        lv_buttonmatrix_clear_button_ctrl(matrix, btn_id, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    ui_tick();
}

// ═══════════════════════════════════════════════════════════
// HELPER: Handle pairing keys
// ═══════════════════════════════════════════════════════════
static bool handle_pairing_keys(char key) {
    if (!pairing_in_progress) return false;
    
    if (key == 'j') {
        // Accept pairing
        ESP_LOGI(TAG, "✓ Pairing ACCEPTED");
        esp_ble_confirm_reply(gl_remote_bda, true);
        pairing_in_progress = false;
        
        if (objects.blepairingpopup) {
            lv_obj_add_flag(objects.blepairingpopup, LV_OBJ_FLAG_HIDDEN);
        }
        return true;
    }
    
    if (key == 'l') {
        // Reject pairing
        ESP_LOGI(TAG, "✗ Pairing REJECTED");
        esp_ble_confirm_reply(gl_remote_bda, false);
        pairing_in_progress = false;
        
        if (objects.blepairingpopup) {
            lv_obj_add_flag(objects.blepairingpopup, LV_OBJ_FLAG_HIDDEN);
        }
        return true;
    }
    
    return false;
}
void update_buttonset_label() {
    if (objects.buttonset) {
        ButtonSet* activeSet = &buttonSets[activeButtonSetIndex];
        // Use EEZ variable instead of direct label update
        set_var_buttonset(activeSet->set_name);
        ESP_LOGI(TAG, "Active button set: %s", activeSet->set_name);
    }
}
// ═══════════════════════════════════════════════════════════
// HELPER: Handle navigation keys
// ═══════════════════════════════════════════════════════════
static void handle_navigation_keys(char key) {
    switch (key) {
        case 'i':
            nextButtonSet();
            update_buttonset_label();
            break;
            
        case 'j':  // Menu enter
            if (menuStackIndex == -1) {
                ESP_LOGI(TAG, "Entering main menu");
                menuStackIndex = 0;
                menuItems = menuStack[menuStackIndex].submenu;
                menuSize = menuStack[menuStackIndex].submenuSize;
            } else if (menuItems[currentMenuIndex].submenu) {
                menuStack[menuStackIndex].submenu = menuItems;
                menuStack[menuStackIndex].submenuSize = menuSize;
                menuStackIndex++;
                menuItems = menuStack[menuStackIndex - 1].submenu[currentMenuIndex].submenu;
                menuSize = menuStack[menuStackIndex - 1].submenu[currentMenuIndex].submenuSize;
            } else {
                ESP_LOGD(TAG, "No submenu available");
            }
            rotaryEncoder.value = rotaryEncoder.min_value;
            setupEncoder(ENC_A, ENC_B, 0, menuSize - 1, true, encTurn);
            currentMenuIndex = 0;
            break;
            
        case 'k':  // Menu back
            if (menuStackIndex > 0) {
                menuStackIndex--;
                menuItems = menuStack[menuStackIndex].submenu;
                menuSize = menuStack[menuStackIndex].submenuSize;
                currentMenuIndex = 0;
                rotaryEncoder.value = rotaryEncoder.min_value;
                setupEncoder(ENC_A, ENC_B, 0, menuSize - 1, true, encTurn);
            } else if (menuStackIndex == 0) {
                menuStackIndex--;
                setupEncoder(ENC_A, ENC_B, 0, 100, true, encTurn);
            }
            break;
            
        case 'l':  // Utility
            previousButtonSet();
            update_buttonset_label();
            active_theme_index = next_theme(active_theme_index);
            break;
            
        default:
            ESP_LOGD(TAG, "Unhandled key: %c", key);
            break;
    }
}

// ═══════════════════════════════════════════════════════════
// Key Press Handler
// ═══════════════════════════════════════════════════════════
void keyPressed(char key) {
    // Light up LED and highlight button
    if (is_mapped_button(key)) {
        highlight_button(key);
        int ledIndex = key - 'a';
        keysPressed[ledIndex] = 1;
        setPixelColor(led_strip, ledIndex, colors[RED]);
    }
    
    // Priority 1: Pairing
    if (handle_pairing_keys(key)) return;
    
    // Priority 2: Button mapping
    if (execute_button_command(key)) return;
    
    // Priority 3: Navigation
    handle_navigation_keys(key);
}
