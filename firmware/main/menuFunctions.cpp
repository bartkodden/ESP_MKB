#include "menufunctions.h"

/*
Menu Structure:
1 - Bluetooth (submenu)
  1.1 - Turn on/off (toggle)
  1.2 - Pairing mode (go in pairing loop)
  1.3 - Back (go back to main menu)
2 - WiFi (submenu)
  2.1 - Turn On/Off (toggle)
  2.2 - WiFi Mode (AP/Station)
  2.3 - WiFi SSID (input)
  2.4 - WiFi Password (input)
  2.5 - Back (go back to main menu)
3 - Display (submenu)
  3.1 - Brightness (Low, Medium, High)
  3.2 - Color (Red, Green, Blue, White, Yellow, Cyan, Magenta)
  3.3 - Back (go back to main menu)
4 - System (submenu)
  4.1 - Reset
  4.2 - Update
  4.3 - Back (go back to main menu)
5 - Exit (exit the menu)
*/

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