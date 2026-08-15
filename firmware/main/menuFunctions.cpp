#include "menuFunctions.h"
extern esp_err_t wifimanager_connect(void);
extern esp_err_t wifimanager_stop(void);
cJSON *s_menu_root = nullptr;
extern uint32_t active_theme_index;
extern "C" uint32_t next_theme(uint32_t active_theme_index);
extern "C" void change_color_theme(uint32_t theme_index);
extern void buttonui_refresh(void);

// ============================================================================
// MENU PARSING
// ============================================================================

void getNextMenu(cJSON *menuArray, MenuItem *menuItems, int &menuIndex) {
    if (menuArray && cJSON_IsArray(menuArray)) {
        int size = cJSON_GetArraySize(menuArray);
        for (int i = 0; i < size; i++) {
            cJSON *menuObj = cJSON_GetArrayItem(menuArray, i);
            menuItems[menuIndex].name = strdup(cJSON_GetObjectItem(menuObj, "name")->valuestring);
            menuItems[menuIndex].type = strdup(cJSON_GetObjectItem(menuObj, "type")->valuestring);
            menuItems[menuIndex].submenu = nullptr;
            menuItems[menuIndex].submenuSize = 0;

            // Check if this menu item has a submenu
            cJSON *submenuArray = cJSON_GetObjectItem(menuObj, "submenu");
            if (submenuArray && cJSON_IsArray(submenuArray)) {
                // Allocate space for the submenu items
                menuItems[menuIndex].submenuSize = cJSON_GetArraySize(submenuArray);
                menuItems[menuIndex].submenu = new MenuItem[menuItems[menuIndex].submenuSize];
                
                // Recursively get the next menu items
                int submenuIndex = 0;
                getNextMenu(submenuArray, menuItems[menuIndex].submenu, submenuIndex);
            }
            menuIndex++;
        }
    }
}

bool parseMenu(cJSON *json, MenuItem *&menuItems, int &menuSize) {
    cJSON *menuArray = cJSON_GetObjectItem(json, "menu");
    if (!cJSON_IsArray(menuArray)) {
        printf("Invalid JSON format: 'menu' is not an array\n");
        return false;
    }

    menuSize = cJSON_GetArraySize(menuArray);
    menuItems = new MenuItem[menuSize];
    int menuIndex = 0;

    // Start processing from the main menu level
    getNextMenu(menuArray, menuItems, menuIndex);
    return true;
}

// ============================================================================
// MENU ACTION FUNCTIONS
// ============================================================================

void toggleBluetooth() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Bluetooth toggled!");
}

void enterPairingMode() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Entering pairing");
    tft.setCursor(10, 120);
    tft.println("mode...");
}

void toggleWiFi() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("WiFi toggled!");
}

void setWiFiMode() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Setting WiFi");
    tft.setCursor(10, 120);
    tft.println("mode...");
}

void inputSSID() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Input SSID...");
}

void inputPassword() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Input Password...");
}

void setBrightness() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Setting");
    tft.setCursor(10, 120);
    tft.println("brightness...");
}

void setColor() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Setting color...");
}

void resetSystem() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("System reset!");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void updateSystem() {
    tft.fillRect(0, 30, 240, 250, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("System");
    tft.setCursor(10, 120);
    tft.println("updating...");
    // TODO: Add OTA update logic here
}

static cJSON* find_menu_item(cJSON *arr, const char *id) {
    if (!arr) return nullptr;
    int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n; i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        cJSON *item_id = cJSON_GetObjectItem(item, "id");
        if (item_id && strcmp(item_id->valuestring, id) == 0) return item;

        cJSON *sub = cJSON_GetObjectItem(item, "submenu");
        if (sub) {
            cJSON *found = find_menu_item(sub, id);
            if (found) return found;
        }
    }
    return nullptr;
}

// ── Public: read value by id ──────────────────────────────────────────────────
const char* getMenuValue(const char *id) {
    if (!s_menu_root) return nullptr;
    cJSON *arr  = cJSON_GetObjectItem(s_menu_root, "menu");
    cJSON *item = find_menu_item(arr, id);
    if (!item) return nullptr;
    cJSON *val  = cJSON_GetObjectItem(item, "value");
    return val ? val->valuestring : nullptr;
}

// ── Public: save value by id → write back to LittleFS ────────────────────────
bool saveMenuValue(const char *id, const char *value) {
    if (!s_menu_root) return false;
    cJSON *arr  = cJSON_GetObjectItem(s_menu_root, "menu");
    cJSON *item = find_menu_item(arr, id);
    if (!item) return false;

    // Update or create the value field
    cJSON *val = cJSON_GetObjectItem(item, "value");
    if (val) {
        cJSON_SetValuestring(val, value);
    } else {
        cJSON_AddStringToObject(item, "value", value);
    }

    // Write back to LittleFS
    char *out = cJSON_PrintUnformatted(s_menu_root);
    if (!out) return false;

    FILE *f = fopen("/storage/menu.json", "w");
    if (f) {
        fputs(out, f);
        fclose(f);
        ESP_LOGI("MENU", "Saved %s = %s", id, value);
    }
    cJSON_free(out);

    // Apply the change immediately
    applyMenuValue(id, value);
    return true;
}

// ── Public: apply a value change at runtime ───────────────────────────────────
void applyMenuValue(const char *id, const char *value) {
    // WiFi on/off
    if (strcmp(id, "3.1") == 0) {
        if (strcmp(value, "On") == 0)  wifimanager_connect();
        else                           wifimanager_stop();
    }
    // WiFi connect action
    else if (strcmp(id, "3.5") == 0) {
        wifimanager_connect();
    }
    // BT advertise
    else if (strcmp(id, "2.2") == 0) {
        extern void start_ble_advertising(void);
        start_ble_advertising();
    }
    // Theme
    else if (strcmp(id, "4.2") == 0) {
        const char *themes[] = {"red", "green", "blue", "dark", "pink"};
        for (int i = 0; i < 5; i++) {
            if (strcmp(value, themes[i]) == 0) {
                active_theme_index = i;
                change_color_theme(i);    // apply directly, no cycling needed
                buttonui_refresh();       // update label colors
                break;
            }
        }
    }
    // Sleep timeout
    else if (strcmp(id, "5.1") == 0) {
        // TODO: awakeTime is currently a #define in init.h
        // Change to: extern uint32_t awakeTime; in init.h to make it writable
        ESP_LOGI("MENU", "Sleep timeout changed to %s (not yet applied)", value);
    }
}