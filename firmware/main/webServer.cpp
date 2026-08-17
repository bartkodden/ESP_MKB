// webServer.cpp
#include "webServer.h"
#include "menuFunctions.h"
#include "fileFunctions.h"
#include "buttonUI.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG       = "WEBSERVER";
static httpd_handle_t s_server = NULL;

// Forward declarations
extern void buttonui_refresh(void);
extern bool saveMenuValue(const char *id, const char *value);
static esp_err_t handle_root(httpd_req_t *req);
static esp_err_t handle_status(httpd_req_t *req);
static esp_err_t handle_file_get(httpd_req_t *req);
static esp_err_t handle_file_post(httpd_req_t *req);
static esp_err_t handle_icon_upload(httpd_req_t *req);
static esp_err_t handle_favicon(httpd_req_t *req);
static esp_err_t handle_reboot(httpd_req_t *req);
static esp_err_t handle_advertise(httpd_req_t *req);
extern void start_ble_advertising(void);
extern void loadButtonMappings(void);
extern void buttonui_refresh(void);

// ── Helpers ───────────────────────────────────────────────────────────────────

static const char* mime_type(const char *path) {
    if (strstr(path, ".json")) return "application/json";
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".bin"))  return "application/octet-stream";
    return "text/plain";
}

// Build /storage/<filename> from URI like /api/files/buttons.json
static void uri_to_path(const char *uri, const char *prefix,
                         char *out, size_t out_len) {
    const char *filename = uri + strlen(prefix);
    // strip leading slash if present
    if (filename[0] == '/') filename++;
    snprintf(out, out_len, "/storage/%s", filename);
}

// ── GET / → serve editor.html from LittleFS ───────────────────────────────────
static esp_err_t handle_root(httpd_req_t *req) {
    FILE *f = fopen("/storage/editor.html", "r");
    if (!f) {
        // Friendly error if editor.html not uploaded yet
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req,
            "<h2>ESP-MKB Web Editor</h2>"
            "<p>editor.html not found on LittleFS.</p>"
            "<p>Upload it via: <code>idf.py flash</code></p>");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, n);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ── GET /api/status → device status JSON ─────────────────────────────────────
static esp_err_t handle_status(httpd_req_t *req) {
    extern int batPerc;
    extern bool    ble_conn;

    char json[256];
    snprintf(json, sizeof(json),
        "{"
        "\"device\":\"ESP-MKB\","
        "\"battery\":%d,"
        "\"ble_connected\":%s,"
        "\"heap_free\":%lu"
        "}",
        batPerc,
        ble_conn ? "true" : "false",
        (unsigned long)esp_get_free_heap_size()
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

// ── GET /api/files/* → serve file from LittleFS ──────────────────────────────
static esp_err_t handle_file_get(httpd_req_t *req) {
    char path[64];
    uri_to_path(req->uri, "/api/files", path, sizeof(path));
    ESP_LOGI(TAG, "GET: %s", path);

    struct stat st;
    if (stat(path, &st) != 0) { httpd_resp_send_404(req); return ESP_FAIL; }

    FILE *f = fopen(path, "rb");
    if (!f) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "open failed"); return ESP_FAIL; }

    httpd_resp_set_type(req, mime_type(path));

    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ── POST /api/files/* → save file to LittleFS ────────────────────────────────
static esp_err_t handle_file_post(httpd_req_t *req) {
    char path[64];
    uri_to_path(req->uri, "/api/files", path, sizeof(path));

    // Reject paths with .. to prevent directory traversal
    if (strstr(path, "..")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot write: %s", path);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Cannot write file");
        return ESP_FAIL;
    }

    char buf[512];
    int remaining = req->content_len;
    int received;
    while (remaining > 0) {
        received = httpd_req_recv(req, buf, (remaining < (int)sizeof(buf) ? remaining : (int)sizeof(buf)));
        if (received <= 0) {
            fclose(f);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Receive error");
            return ESP_FAIL;
        }
        fwrite(buf, 1, received, f);
        remaining -= received;
    }
    fclose(f);

    ESP_LOGI(TAG, "Saved: %s (%d bytes)", path, req->content_len);

    // Hot-reload if config files changed
    if (strstr(path, "buttons.json")) {
        loadButtonMappings();
        buttonui_refresh();
        ESP_LOGI(TAG, "Button mappings reloaded");
    }
    if (strstr(path, "menu.json")) {
        // Re-parse menu — WiFi/theme changes apply immediately
        extern esp_err_t setupMenu(void);
        setupMenu();
        ESP_LOGI(TAG, "Menu reloaded");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ── POST /api/icons/upload → save subset font bin ────────────────────────────
static esp_err_t handle_icon_upload(httpd_req_t *req) {
    // Get filename from query string: /api/icons/upload?name=foo.bin
    char query[64] = {};
    char fname[32] = "material_icons_used_24.bin";  // default name

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(query, "name", param, sizeof(param)) == ESP_OK) {
            strncpy(fname, param, sizeof(fname) - 1);
        }
    }

    char path[64];
    snprintf(path, sizeof(path), "/storage/icons/%s", fname);

    FILE *f = fopen(path, "wb");
    if (!f) {
        // Try creating the icons dir first
        mkdir("/storage/icons", 0755);
        f = fopen(path, "wb");
        if (!f) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Cannot write icon file");
            return ESP_FAIL;
        }
    }

    char buf[512];
    int remaining = req->content_len;
    int received;
    while (remaining > 0) {
        received = httpd_req_recv(req, buf, (remaining < (int)sizeof(buf) ? remaining : (int)sizeof(buf)));
        if (received <= 0) { fclose(f); return ESP_FAIL; }
        fwrite(buf, 1, received, f);
        remaining -= received;
    }
    fclose(f);

    ESP_LOGI(TAG, "Icon uploaded: %s (%d bytes)", path, req->content_len);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// ── Start / Stop ──────────────────────────────────────────────────────────────
esp_err_t webserver_start(uint16_t port) {
    if (s_server) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }

    httpd_config_t config    = HTTPD_DEFAULT_CONFIG();
    config.server_port       = port;
    config.uri_match_fn      = httpd_uri_match_wildcard;
    config.max_uri_handlers  = 8;
    config.stack_size        = 8192;
    config.send_wait_timeout = 10;    // ← add: wait up to 10s for send buffer
    config.recv_wait_timeout = 10;

    if (httpd_start(&s_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    // Route table
    static const httpd_uri_t routes[] = {
        { "/",                  HTTP_GET,  handle_root,        NULL },
        { "/api/status",        HTTP_GET,  handle_status,      NULL },
        { "/api/files/*",       HTTP_GET,  handle_file_get,    NULL },
        { "/api/files/*",       HTTP_POST, handle_file_post,   NULL },
        { "/api/icons/upload",  HTTP_POST, handle_icon_upload, NULL },
        { "/favicon.ico",       HTTP_GET,  handle_favicon,     NULL },
        { "/api/reboot",        HTTP_POST, handle_reboot,      NULL },
        { "/api/action/advertise", HTTP_POST, handle_advertise,  NULL },
    };
    for (size_t i = 0; i < sizeof(routes)/sizeof(routes[0]); i++) {
        httpd_register_uri_handler(s_server, &routes[i]);
    }

    ESP_LOGI(TAG, "Web server running on port %d", port);
    ESP_LOGI(TAG, "Open: http://192.168.4.1 (AP mode)");
    return ESP_OK;
}

esp_err_t webserver_stop(void) {
    if (!s_server) return ESP_OK;
    httpd_stop(s_server);
    s_server = NULL;
    ESP_LOGI(TAG, "Web server stopped");
    return ESP_OK;
}

static esp_err_t handle_favicon(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t handle_reboot(httpd_req_t *req) {
    httpd_resp_sendstr(req, "{\"status\":\"rebooting\"}");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

static esp_err_t handle_advertise(httpd_req_t *req) {
    start_ble_advertising();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}