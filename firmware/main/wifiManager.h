#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MGR_MODE_OFF = 0,
    WIFI_MGR_MODE_AP,
    WIFI_MGR_MODE_STA,
} wifi_mgr_mode_t;

typedef struct {
    bool        connected;
    bool        ap_active;
    char        ip[16];
    char        ssid[33];
    int8_t      rssi;
    uint16_t    port;
} wifi_mgr_status_t;

esp_err_t         wifimanager_init(void);
esp_err_t         wifimanager_start_ap(void);
esp_err_t         wifimanager_start_sta(const char *ssid, const char *pass);
esp_err_t         wifimanager_stop(void);
esp_err_t         wifimanager_connect(void);
wifi_mgr_status_t wifimanager_get_status(void);
bool              wifimanager_is_active(void);

#ifdef __cplusplus
}
#endif