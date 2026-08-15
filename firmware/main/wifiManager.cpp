#include "wifiManager.h"
#include "webServer.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "mcs_client.h"
#include "ui/screens.h"
#include <string.h>
extern const char* getMenuValue(const char *id);
static const char *TAG = "WIFI_MGR";

static wifi_mgr_mode_t  s_mode      = WIFI_MGR_MODE_OFF;
static wifi_mgr_status_t s_status   = {};
static bool             s_netif_init = false;
static esp_netif_t     *s_netif_ap  = nullptr;
static esp_netif_t     *s_netif_sta = nullptr;

extern "C" {
    extern const lv_image_dsc_t img_wifi_on;
    extern const lv_image_dsc_t img_wifi_off;
}

#define AP_SSID     "ESP-MKB"
#define AP_PASS     "esp-mkb-config"    // min 8 chars for WPA2
#define AP_CHANNEL  1
#define WEB_PORT    80

static void update_wifi_icon() {
    if (!objects.wifiicon) return;

    bool active = (s_mode == WIFI_MGR_MODE_AP) ||
                  (s_mode == WIFI_MGR_MODE_STA && s_status.connected);

    lv_image_set_src(objects.wifiicon,
                     active ? &img_wifi_on : &img_wifi_off);
}

// ── Event handler ─────────────────────────────────────────────────────────────
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data) {
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "STA disconnected — retrying...");
            s_status.connected = false;
            s_status.ip[0] = '\0';
            update_wifi_icon();
            esp_wifi_connect();
        } else if (id == WIFI_EVENT_AP_STACONNECTED) {
            ESP_LOGI(TAG, "Client connected to AP");
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t*)data;
        snprintf(s_status.ip, sizeof(s_status.ip), IPSTR,
                 IP2STR(&e->ip_info.ip));
        s_status.connected = true;
        s_status.port      = WEB_PORT;
        ESP_LOGI(TAG, "STA connected — IP: %s", s_status.ip);
        ESP_LOGI(TAG, "Open: http://%s", s_status.ip);
        webserver_start(WEB_PORT);
        update_wifi_icon();
        extern void mcs_client_init(void);
        extern void start_mcs_scanning(void);
        extern bool ble_conn;
        if (ble_conn) {
            vTaskDelay(pdMS_TO_TICKS(2000));   // ← was 500ms, give WiFi 2s to stabilize
            mcs_client_init();
            vTaskDelay(pdMS_TO_TICKS(500));
            start_mcs_scanning();
        }
    }
}
// ── Netif init (once only) ────────────────────────────────────────────────────
static void ensure_netif_init() {
    if (s_netif_init) return;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, nullptr));
    s_netif_init = true;
}

// ── AP mode ───────────────────────────────────────────────────────────────────
esp_err_t wifimanager_start_ap() {
    ensure_netif_init();
    ESP_LOGI(TAG, "Starting AP: %s", AP_SSID);

    if (!s_netif_ap) s_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t ap_cfg = {};
    strncpy((char*)ap_cfg.ap.ssid,     AP_SSID, sizeof(ap_cfg.ap.ssid));
    strncpy((char*)ap_cfg.ap.password, AP_PASS,  sizeof(ap_cfg.ap.password));
    ap_cfg.ap.ssid_len       = strlen(AP_SSID);
    ap_cfg.ap.channel        = AP_CHANNEL;
    ap_cfg.ap.authmode       = WIFI_AUTH_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;

    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();

    strncpy(s_status.ip,   "192.168.4.1", sizeof(s_status.ip));
    strncpy(s_status.ssid, AP_SSID,       sizeof(s_status.ssid));
    s_status.ap_active = true;
    s_status.port      = WEB_PORT;
    s_mode             = WIFI_MGR_MODE_AP;

    ESP_LOGI(TAG, "AP started — connect to '%s' pass '%s' then open http://192.168.4.1",
             AP_SSID, AP_PASS);

    webserver_start(WEB_PORT);
    return ESP_OK;
}

// ── STA mode ──────────────────────────────────────────────────────────────────
esp_err_t wifimanager_start_sta(const char *ssid, const char *pass) {
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGW(TAG, "No SSID configured — falling back to AP");
        return wifimanager_start_ap();
    }
    ensure_netif_init();
    ESP_LOGI(TAG, "Connecting STA to: %s", ssid);

    if (!s_netif_sta) s_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t sta_cfg = {};
    strncpy((char*)sta_cfg.sta.ssid,     ssid, sizeof(sta_cfg.sta.ssid));
    strncpy((char*)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password));

    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_start();

    strncpy(s_status.ssid, ssid, sizeof(s_status.ssid));
    s_mode = WIFI_MGR_MODE_STA;

    // Web server starts when IP is obtained (in event handler)
    webserver_start(WEB_PORT);
    return ESP_OK;
}

// ── Stop ──────────────────────────────────────────────────────────────────────
esp_err_t wifimanager_stop() {
    webserver_stop();
    esp_wifi_stop();
    esp_wifi_deinit();
    s_mode             = WIFI_MGR_MODE_OFF;
    s_status.connected = false;
    s_status.ap_active = false;
    ESP_LOGI(TAG, "WiFi stopped");
    return ESP_OK;
}

// ── Connect (reads credentials from menu.json) ────────────────────────────────
esp_err_t wifimanager_connect() {
    const char *enabled = getMenuValue("3.1");
    if (!enabled || strcmp(enabled, "On") != 0) {
        ESP_LOGI(TAG, "WiFi disabled in settings");
        return ESP_OK;
    }

    const char *mode = getMenuValue("3.2");
    const char *ssid = getMenuValue("3.3");
    const char *pass = getMenuValue("3.4");

    if (mode && strcmp(mode, "Client") == 0) {
        return wifimanager_start_sta(ssid ? ssid : "", pass ? pass : "");
    } else {
        return wifimanager_start_ap();
    }
}

// ── Init (called once at boot) ────────────────────────────────────────────────
esp_err_t wifimanager_init() {
    return wifimanager_connect();
}

// ── Status ────────────────────────────────────────────────────────────────────
wifi_mgr_status_t wifimanager_get_status() { return s_status; }
bool wifimanager_is_active() { return s_mode != WIFI_MGR_MODE_OFF; }

