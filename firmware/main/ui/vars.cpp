#include "vars.h"
#include "esp_log.h"
#include <string>

int32_t batperc;
std::string blepairingcode;
std::string bledevicename;
std::string buttonset;
bool mcs_nc_show;
std::string loading_status;

extern "C" int32_t get_var_batperc() {
    return batperc;
}

extern "C" void set_var_batperc(int32_t value) {
    batperc = value;
    ESP_LOGI("EEZ", "batperc updated");
}

extern "C" const char *get_var_blepairingcode() {
    return blepairingcode.c_str();
}

extern "C" void set_var_blepairingcode(const char *value) {
    blepairingcode = value;
}

extern "C" const char *get_var_bledevicename() {
    return bledevicename.c_str();
}

extern "C" void set_var_bledevicename(const char *value) {
    bledevicename = value;
}

extern "C" const char *get_var_buttonset() {
    return buttonset.c_str();
}

extern "C" void set_var_buttonset(const char *value) {
    buttonset = value;
}

extern "C" bool get_var_mcs_nc_show() {
    return mcs_nc_show;
}

extern "C" void set_var_mcs_nc_show(bool value) {
    mcs_nc_show = value;
}

extern "C" const char *get_var_loading_status() {
    return loading_status.c_str();
}

extern "C" void set_var_loading_status(const char *value) {
    loading_status = value;
}
