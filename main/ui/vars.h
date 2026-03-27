#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_BLEPAIRINGCODE = 0,
    FLOW_GLOBAL_VARIABLE_BLEDEVICENAME = 1,
    FLOW_GLOBAL_VARIABLE_BUTTONSET = 2,
    FLOW_GLOBAL_VARIABLE_MCS_NC_SHOW = 3,
    FLOW_GLOBAL_VARIABLE_LOADING_STATUS = 4
};

// Native global variables

extern int32_t get_var_screen();
extern void set_var_screen(int32_t value);
extern int32_t get_var_batperc();
extern void set_var_batperc(int32_t value);
extern const char *get_var_blepairingcode();
extern void set_var_blepairingcode(const char *value);
extern const char *get_var_bledevicename();
extern void set_var_bledevicename(const char *value);
extern const char *get_var_buttonset();
extern void set_var_buttonset(const char *value);
extern bool get_var_mcs_nc_show();
extern void set_var_mcs_nc_show(bool value);
extern const char *get_var_loading_status();
extern void set_var_loading_status(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/