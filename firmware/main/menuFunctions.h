#ifndef MENUFUNCTIONS_H
#define MENUFUNCTIONS_H

#include "init.h"
#include "cJSON.h"
extern cJSON *s_menu_root;
extern const char* getMenuValue(const char *id);
extern bool        saveMenuValue(const char *id, const char *value);
extern void        applyMenuValue(const char *id, const char *value);
// Menu parsing functions
void getNextMenu(cJSON *menuArray, MenuItem *menuItems, int &menuIndex);
bool parseMenu(cJSON *json, MenuItem *&menuItems, int &menuSize);

// Menu action functions
void toggleBluetooth();
void enterPairingMode();
void toggleWiFi();
void setWiFiMode();
void inputSSID();
void inputPassword();
void setBrightness();
void setColor();
void resetSystem();
void updateSystem();

#endif // MENUFUNCTIONS_HC