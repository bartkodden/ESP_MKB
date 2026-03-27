#include "mcs_client.h"
#include "ui/ui.h"
#include "ui/vars.h"
#include "init.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_bt.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "MCS_CLIENT";

// ═══════════════════════════════════════════════════════════
// SERVICE & CHARACTERISTIC UUIDs
// ═══════════════════════════════════════════════════════════
#define MCS_SERVICE_UUID         0x1848
#define MEDIA_STATE_UUID         0x2BA3
#define TRACK_TITLE_UUID         0x2B97
#define TRACK_DURATION_UUID      0x2B98
#define TRACK_POSITION_UUID      0x2B99
#define MCS_VOLUME_UUID          0x2B7E

#define VCS_SERVICE_UUID         0x1844
#define VCS_VOLUME_STATE_UUID    0x2B7D

#define MICROPHONE_UUID          0x2C7F

#define ALBUM_ART_CHUNK_COUNT 16
#define CHUNK_SIZE 512

static const uint16_t ALBUM_CHUNK_UUIDS[16] = {
    0x2BF0, 0x2BF1, 0x2BF2, 0x2BF3,
    0x2BF4, 0x2BF5, 0x2BF6, 0x2BF7,
    0x2BF8, 0x2BF9, 0x2BFA, 0x2BFB,
    0x2BFC, 0x2BFD, 0x2BFE, 0x2BFF
};

// ═══════════════════════════════════════════════════════════
// CONNECTION STATE
// ═══════════════════════════════════════════════════════════
static esp_gatt_if_t mcs_gattc_if = ESP_GATT_IF_NONE;
static uint16_t mcs_conn_id = 0;
static esp_bd_addr_t server_bda;
static bool is_connected = false;
static bool is_scanning = false;
static bool connection_in_progress = false;
static bool using_shared_connection = false;
static int64_t last_notify_time = 0;
static bool album_transfer_in_progress = false;

// ═══════════════════════════════════════════════════════════
// SERVICE HANDLES
// ═══════════════════════════════════════════════════════════
static uint16_t mcs_service_start_handle = 0;
static uint16_t mcs_service_end_handle = 0;
static uint16_t vcs_service_start_handle = 0;
static uint16_t vcs_service_end_handle = 0;

// ═══════════════════════════════════════════════════════════
// CHARACTERISTIC HANDLES
// ═══════════════════════════════════════════════════════════
static uint16_t media_state_handle = 0;
static uint16_t track_title_handle = 0;
static uint16_t track_duration_handle = 0;
static uint16_t track_position_handle = 0;
static uint16_t mcs_volume_handle = 0;
static uint16_t vcs_volume_handle = 0;
static uint16_t microphone_handle = 0;
static uint16_t album_chunk_handles[ALBUM_ART_CHUNK_COUNT] = {0};

// ═══════════════════════════════════════════════════════════
// CLIENT METADATA
// ═══════════════════════════════════════════════════════════
static uint8_t client_media_state = 0x00;
static uint32_t client_track_position = 0;
static uint32_t client_track_duration = 0;
static uint8_t client_album_art[8192] = {0};
static bool album_art_valid = false;
static size_t album_art_received = 0;

static int64_t playback_start_time_ms = 0;
static bool is_playing = false;

static char client_artist[64] = "Unknown";
static char client_title[64] = "Unknown";
static char client_track_title[128] = "No Media";

// ═══════════════════════════════════════════════════════════
// UI UPDATE QUEUE
// ═══════════════════════════════════════════════════════════
typedef enum {
    UI_UPDATE_TRACK_TITLE,
    UI_UPDATE_MEDIA_STATE,
    UI_UPDATE_TRACK_POSITION,
    UI_UPDATE_ALBUM_ART
} ui_update_type_t;

typedef struct {
    ui_update_type_t type;
    union {
        char track_title[128];
        uint8_t media_state;
        uint32_t track_position;
    } data;
} ui_update_msg_t;

static QueueHandle_t ui_update_queue = NULL;

// ═══════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ═══════════════════════════════════════════════════════════
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

// ═══════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════
static void force_disconnect_cleanup(void)
{
    ESP_LOGI(TAG, "Force cleanup...");
    
    static const uint8_t zero_bda[6] = {0};
    
    if (mcs_gattc_if != ESP_GATT_IF_NONE && memcmp(server_bda, zero_bda, 6) != 0) {
        esp_ble_gattc_close(mcs_gattc_if, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    is_connected = false;
    connection_in_progress = false;
    album_transfer_in_progress = false;
    mcs_conn_id = 0;
    
    media_state_handle = 0;
    track_title_handle = 0;
    track_duration_handle = 0;
    track_position_handle = 0;
    mcs_volume_handle = 0;
    vcs_volume_handle = 0;
    microphone_handle = 0;
    
    memset(album_chunk_handles, 0, sizeof(album_chunk_handles));
    mcs_service_start_handle = 0;
    mcs_service_end_handle = 0;
    vcs_service_start_handle = 0;
    vcs_service_end_handle = 0;
    memset(server_bda, 0, sizeof(server_bda));
    
    ESP_LOGI(TAG, "✓ Cleanup done");
}

// ═══════════════════════════════════════════════════════════
// METADATA UPDATES
// ═══════════════════════════════════════════════════════════

static void update_track_title(const char *title, size_t len)
{
    size_t copy_len = (len < sizeof(client_track_title) - 1) ? len : sizeof(client_track_title) - 1;
    memcpy(client_track_title, title, copy_len);
    client_track_title[copy_len] = '\0';
    
    char *separator = strstr(client_track_title, " - ");
    if (separator != NULL && separator != client_track_title) {
        size_t artist_len = separator - client_track_title;
        
        if (artist_len < sizeof(client_artist)) {
            memcpy(client_artist, client_track_title, artist_len);
            client_artist[artist_len] = '\0';
        }
        
        const char *title_start = separator + 3;
        strncpy(client_title, title_start, sizeof(client_title) - 1);
        client_title[sizeof(client_title) - 1] = '\0';
    } else {
        strncpy(client_title, client_track_title, sizeof(client_title) - 1);
        strcpy(client_artist, "Unknown");
    }
    
    ESP_LOGI(TAG, "♪ %s - %s", client_artist, client_title);
    
    if (ui_update_queue) {
        ui_update_msg_t msg;
        msg.type = UI_UPDATE_TRACK_TITLE;
        strncpy(msg.data.track_title, client_track_title, sizeof(msg.data.track_title) - 1);
        msg.data.track_title[sizeof(msg.data.track_title) - 1] = '\0';
        xQueueSend(ui_update_queue, &msg, 0);
    }
}

static void update_media_state(uint8_t state)
{
    static uint8_t last_logged_state = 0xFF;
    
    bool state_changed = (state != client_media_state);
    
    client_media_state = state;
    is_playing = (state == 0x01);
    
    if (state_changed && is_playing) {
        playback_start_time_ms = (esp_timer_get_time() / 1000) - (client_track_position * 1000);
    }
    
    if (ui_update_queue && state_changed) {
        ui_update_msg_t msg;
        msg.type = UI_UPDATE_MEDIA_STATE;
        msg.data.media_state = state;
        xQueueSend(ui_update_queue, &msg, 0);
    }
    
    if (state != last_logged_state) {
        const char *state_str = (state == 0x01) ? "▶️ Playing" : 
                               (state == 0x02) ? "⏸️ Paused" : "⏹️ Stopped";
        ESP_LOGI(TAG, "%s", state_str);
        last_logged_state = state;
    }
}

static void update_track_position(uint32_t position_sec)
{
    static uint32_t last_synced_position = 0xFFFFFFFF;
    
    bool should_sync = (position_sec != last_synced_position) || 
                       (abs((int)position_sec - (int)client_track_position) >= 3);
    
    if (should_sync) {
        ESP_LOGI(TAG, "Position synced: %lu -> %lu sec", client_track_position, position_sec);
        client_track_position = position_sec;
        playback_start_time_ms = (esp_timer_get_time() / 1000) - (position_sec * 1000);
        last_synced_position = position_sec;
    }
}

static void update_track_duration(uint32_t duration_sec)
{
    static uint32_t last_logged_duration = 0;
    
    if (duration_sec > 7200) {
        ESP_LOGW(TAG, "Invalid duration: %lu (0x%08lX)", duration_sec, duration_sec);
        duration_sec = 0;
    }
    
    client_track_duration = duration_sec;
    
    if (duration_sec != last_logged_duration) {
        ESP_LOGI(TAG, "Duration: %lu sec", duration_sec);
        last_logged_duration = duration_sec;
    }
    
    // Don't call LVGL here - main loop will handle it
}

// ═══════════════════════════════════════════════════════════
// GATT CLIENT CALLBACK
// ═══════════════════════════════════════════════════════════

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTC_REG_EVT:
        if (param->reg.status == ESP_GATT_OK) {
            mcs_gattc_if = gattc_if;
            ESP_LOGI(TAG, "GATT Client registered");
        }
        break;
        
    case ESP_GATTC_OPEN_EVT:
        connection_in_progress = false;
        
        if (!using_shared_connection) {
            is_scanning = false;
            
            if (param->open.status == ESP_GATT_OK) {
                ESP_LOGI(TAG, "Connected");
                
                mcs_conn_id = param->open.conn_id;
                is_connected = true;
                last_notify_time = esp_timer_get_time() / 1000;
                
                esp_ble_gattc_send_mtu_req(gattc_if, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                
                ESP_LOGI(TAG, "Searching services...");
                esp_ble_gattc_search_service(gattc_if, 0, NULL);  // Search all services
            } else {
                ESP_LOGE(TAG, "Connect failed: %d", param->open.status);
                is_connected = false;
            }
        }
        break;
        
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "MTU: %d", param->cfg_mtu.mtu);
        }
        break;
        
    case ESP_GATTC_SEARCH_RES_EVT:
        if (param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16) {
            uint16_t uuid = param->search_res.srvc_id.uuid.uuid.uuid16;
            
            if (uuid == MCS_SERVICE_UUID) {
                mcs_service_start_handle = param->search_res.start_handle;
                mcs_service_end_handle = param->search_res.end_handle;
                ESP_LOGI(TAG, "✓ MCS Service found");
            }
            else if (uuid == VCS_SERVICE_UUID) {
                vcs_service_start_handle = param->search_res.start_handle;
                vcs_service_end_handle = param->search_res.end_handle;
                ESP_LOGI(TAG, "✓ VCS Service found");
            }
        }
        break;
        
    case ESP_GATTC_SEARCH_CMPL_EVT: {
        if (mcs_service_start_handle == 0 && vcs_service_start_handle == 0) {
            ESP_LOGW(TAG, "No services found - retrying...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_ble_gattc_search_service(gattc_if, 0, NULL);
            break;
        }
        
        ESP_LOGI(TAG, "Discovering characteristics...");
        
        // Discover MCS characteristics
        if (mcs_service_start_handle > 0) {
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count(
                gattc_if, 0, ESP_GATT_DB_CHARACTERISTIC,
                mcs_service_start_handle, mcs_service_end_handle, 0, &count);
                
            if (status == ESP_GATT_OK && count > 0) {
                esp_gattc_char_elem_t *chars = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
                
                if (chars) {
                    status = esp_ble_gattc_get_all_char(gattc_if, 0,
                                                       mcs_service_start_handle, mcs_service_end_handle,
                                                       chars, &count, 0);
                    
                    if (status == ESP_GATT_OK) {
                        for (int i = 0; i < count; i++) {
                            if (chars[i].uuid.len != ESP_UUID_LEN_16) continue;
                            
                            uint16_t uuid = chars[i].uuid.uuid.uuid16;
                            
                            if (uuid == MEDIA_STATE_UUID) {
                                media_state_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->Media State: %d", media_state_handle);
                                esp_ble_gattc_register_for_notify(gattc_if, server_bda, media_state_handle);
                            } 
                            else if (uuid == TRACK_TITLE_UUID) {
                                track_title_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->Track Title: %d", track_title_handle);
                                esp_ble_gattc_register_for_notify(gattc_if, server_bda, track_title_handle);
                            }
                            else if (uuid == TRACK_DURATION_UUID) {
                                track_duration_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->Track Duration: %d", track_duration_handle);
                                esp_ble_gattc_register_for_notify(gattc_if, server_bda, track_duration_handle);
                            }
                            else if (uuid == TRACK_POSITION_UUID) {
                                track_position_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->Track Position: %d", track_position_handle);
                                esp_ble_gattc_register_for_notify(gattc_if, server_bda, track_position_handle);
                            }
                            else if (uuid == MCS_VOLUME_UUID) {
                                mcs_volume_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->MCS Volume: %d", mcs_volume_handle);
                            }
                            else {
                                for (int c = 0; c < ALBUM_ART_CHUNK_COUNT; c++) {
                                    if (uuid == ALBUM_CHUNK_UUIDS[c]) {
                                        album_chunk_handles[c] = chars[i].char_handle;
                                        if (c == 0) ESP_LOGI(TAG, "  ->Album chunks found");
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    free(chars);
                }
            }
        }
        
        // Discover VCS characteristics
        if (vcs_service_start_handle > 0) {
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count(
                gattc_if, 0, ESP_GATT_DB_CHARACTERISTIC,
                vcs_service_start_handle, vcs_service_end_handle, 0, &count);
                
            if (status == ESP_GATT_OK && count > 0) {
                esp_gattc_char_elem_t *chars = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
                
                if (chars) {
                    status = esp_ble_gattc_get_all_char(gattc_if, 0,
                                                       vcs_service_start_handle, vcs_service_end_handle,
                                                       chars, &count, 0);
                    
                    if (status == ESP_GATT_OK) {
                        for (int i = 0; i < count; i++) {
                            if (chars[i].uuid.len != ESP_UUID_LEN_16) continue;
                            
                            uint16_t uuid = chars[i].uuid.uuid.uuid16;
                            
                            if (uuid == VCS_VOLUME_STATE_UUID) {
                                vcs_volume_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->VCS Volume: %d", vcs_volume_handle);
                            }
                            else if (uuid == MICROPHONE_UUID) {
                                microphone_handle = chars[i].char_handle;
                                ESP_LOGI(TAG, "  ->Microphone: %d", microphone_handle);
                            }
                        }
                    }
                    
                    free(chars);
                }
            }
        }
        
        is_connected = true;
        break;
    }
        
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGD(TAG, "Subscribed: %d", param->reg_for_notify.handle);
        
        esp_ble_gattc_read_char(gattc_if, 0, param->reg_for_notify.handle, ESP_GATT_AUTH_REQ_NONE);
        
        uint16_t cccd_handle = param->reg_for_notify.handle + 1;
        uint8_t notify_en[2] = {0x01, 0x00};
        
        esp_ble_gattc_write_char_descr(gattc_if, 0, cccd_handle, sizeof(notify_en), notify_en,
                                       ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        break;
    }

    case ESP_GATTC_WRITE_DESCR_EVT:
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (param->write.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "✓ Write OK: handle=%d", param->write.handle);
        } else {
            ESP_LOGE(TAG, "✗ Write FAILED: handle=%d, status=%d", param->write.handle, param->write.status);
        }
        break;

    case ESP_GATTC_NOTIFY_EVT:
        last_notify_time = esp_timer_get_time() / 1000;
        
        if (param->notify.handle == media_state_handle && param->notify.value_len >= 1) {
            update_media_state(param->notify.value[0]);
        } 
        else if (param->notify.handle == track_title_handle) {
            update_track_title((char*)param->notify.value, param->notify.value_len);
        }
        else if (param->notify.handle == track_duration_handle && param->notify.value_len >= 4) {
            uint32_t duration = param->notify.value[0] | (param->notify.value[1] << 8) | 
                               (param->notify.value[2] << 16) | (param->notify.value[3] << 24);
            update_track_duration(duration);
        }
        else if (param->notify.handle == track_position_handle && param->notify.value_len >= 4) {
            uint32_t pos = param->notify.value[0] | (param->notify.value[1] << 8) | 
                          (param->notify.value[2] << 16) | (param->notify.value[3] << 24);
            update_track_position(pos);
        }
        break;
        
    case ESP_GATTC_READ_CHAR_EVT:
        if (param->read.status == ESP_GATT_OK) {
            
            if (param->read.handle == track_title_handle) {
                update_track_title((char*)param->read.value, param->read.value_len);
            } 
            else if (param->read.handle == media_state_handle && param->read.value_len >= 1) {
                update_media_state(param->read.value[0]);
            }
            else if (param->read.handle == track_duration_handle && param->read.value_len >= 4) {
                uint32_t duration = param->read.value[0] | (param->read.value[1] << 8) | 
                                   (param->read.value[2] << 16) | (param->read.value[3] << 24);
                update_track_duration(duration);
            }
            else if (param->read.handle == track_position_handle && param->read.value_len >= 4) {
                uint32_t pos = param->read.value[0] | (param->read.value[1] << 8) | 
                              (param->read.value[2] << 16) | (param->read.value[3] << 24);
                update_track_position(pos);
            }
            else {
                // Album chunks
                for (int c = 0; c < ALBUM_ART_CHUNK_COUNT; c++) {
                    if (param->read.handle == album_chunk_handles[c]) {
                        ESP_LOGD(TAG, "Chunk %d: %d bytes", c + 1, param->read.value_len);
                        
                        size_t offset = c * CHUNK_SIZE;
                        size_t bytes_to_copy = (param->read.value_len > CHUNK_SIZE) ? CHUNK_SIZE : param->read.value_len;
                        
                        if (offset + bytes_to_copy > sizeof(client_album_art)) {
                            bytes_to_copy = sizeof(client_album_art) - offset;
                        }
                        
                        if (bytes_to_copy > 0) {
                            memcpy(client_album_art + offset, param->read.value, bytes_to_copy);
                            album_art_received += bytes_to_copy;
                        }
                        
                        if (c < ALBUM_ART_CHUNK_COUNT - 1 && album_chunk_handles[c + 1] != 0) {
                            vTaskDelay(pdMS_TO_TICKS(50));
                            esp_ble_gattc_read_char(mcs_gattc_if, 0, album_chunk_handles[c + 1], ESP_GATT_AUTH_REQ_NONE);
                        } else {
                            ESP_LOGI(TAG, "Album: %d bytes", album_art_received);
                            album_art_valid = true;
                            album_transfer_in_progress = false;
                            
                            if (ui_update_queue) {
                                ui_update_msg_t msg;
                                msg.type = UI_UPDATE_ALBUM_ART;
                                xQueueSend(ui_update_queue, &msg, 0);
                            }
                        }
                        
                        break;
                    }
                }
            }
        }
        break;
        
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Disconnected");
        force_disconnect_cleanup();
        
        strcpy(client_track_title, "No Media");
        strcpy(client_artist, "Unknown");
        strcpy(client_title, "Unknown");
        album_art_valid = false;
        is_playing = false;
        
        if (device_mode == MODE_MCS_ONLY) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_ble_gap_start_scanning(30);
        }
        break;
        
    default:
        break;
    }
}

// ═══════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════

void mcs_client_init(void)
{
    ESP_LOGI(TAG, "Initializing MCS Client...");
    ui_update_queue = xQueueCreate(10, sizeof(ui_update_msg_t));
    esp_ble_gattc_register_callback(esp_gattc_cb);
    esp_ble_gattc_app_register(10);
    ESP_LOGI(TAG, "Ready");
}

void mcs_use_existing_connection(uint16_t conn_id, esp_bd_addr_t bda)
{
    mcs_conn_id = conn_id;
    memcpy(server_bda, bda, sizeof(esp_bd_addr_t));
    using_shared_connection = true;
    
    if (mcs_gattc_if != ESP_GATT_IF_NONE) {
        ESP_LOGI(TAG, "Searching MCS & VCS services...");
        esp_ble_gattc_search_service(mcs_gattc_if, 0, NULL);
    }
}

void mcs_set_volume(uint8_t volume_percent)
{
    if (mcs_gattc_if == ESP_GATT_IF_NONE) return;
    
    uint8_t vol = (volume_percent > 100) ? 100 : volume_percent;
    
    uint16_t target_handle = (mcs_volume_handle != 0) ? mcs_volume_handle : vcs_volume_handle;
    
    if (target_handle != 0) {
        ESP_LOGI(TAG, "Setting volume: %d%% (handle: %d)", vol, target_handle);
        esp_ble_gattc_write_char(mcs_gattc_if, 0, target_handle, 1, &vol, 
                                ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    } else {
        ESP_LOGW(TAG, "No volume handle available");
    }
}

void mcs_set_microphone(uint8_t mic_percent)
{
    if (mcs_gattc_if == ESP_GATT_IF_NONE) return;
    
    uint8_t mic = (mic_percent > 100) ? 100 : mic_percent;
    
    if (microphone_handle != 0) {
        ESP_LOGI(TAG, "Setting microphone: %d%%", mic);
        esp_ble_gattc_write_char(mcs_gattc_if, 0, microphone_handle, 1, &mic, 
                                ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    }
}

void mcs_fetch_album_art(void)
{
    if (album_transfer_in_progress) {
        ESP_LOGD(TAG, "Album transfer already in progress");
        return;
    }
    
    bool all_ready = true;
    for (int i = 0; i < ALBUM_ART_CHUNK_COUNT; i++) {
        if (album_chunk_handles[i] == 0) {
            all_ready = false;
            break;
        }
    }
    
    if (all_ready && mcs_gattc_if != ESP_GATT_IF_NONE) {
        ESP_LOGI(TAG, "Fetching album art...");
        
        album_transfer_in_progress = true;
        memset(client_album_art, 0, sizeof(client_album_art));
        album_art_received = 0;
        album_art_valid = false;
        
        esp_ble_gattc_read_char(mcs_gattc_if, 0, album_chunk_handles[0], ESP_GATT_AUTH_REQ_NONE);
    }
}

void mcs_handle_scan_result(esp_ble_gap_cb_param_t *param)
{
    static esp_bd_addr_t last_attempt_bda = {0};
    static int64_t last_attempt_time = 0;
    
    if (is_connected || connection_in_progress) return;
    if (param->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT) return;
    
    uint8_t *adv = param->scan_rst.ble_adv;
    uint8_t len = param->scan_rst.adv_data_len;
    
    int pos = 0;
    bool found_mcs = false;
    
    while (pos < len) {
        uint8_t field_len = adv[pos];
        if (field_len == 0 || pos + field_len >= len) break;
        
        uint8_t field_type = adv[pos + 1];
        
        if (field_type == 0x02 || field_type == 0x03) {
            for (int i = 2; i <= field_len; i += 2) {
                if (pos + i + 1 < len) {
                    uint16_t uuid = adv[pos + i] | (adv[pos + i + 1] << 8);
                    if (uuid == 0x1848) {  // MCS Service
                        found_mcs = true;
                        break;
                    }
                }
            }
        }
        
        pos += field_len + 1;
    }
    
    if (!found_mcs) return;
    
    extern esp_bd_addr_t hid_connected_device;
    extern bool hid_device_connected;
    
    if (!hid_device_connected) return;
    if (memcmp(param->scan_rst.bda, hid_connected_device, 6) != 0) return;
    
    int64_t now = esp_timer_get_time() / 1000;
    if (memcmp(param->scan_rst.bda, last_attempt_bda, 6) == 0 && (now - last_attempt_time) < 10000) {
        return;
    }
    
    ESP_LOGI(TAG, "Found MCS");
    ESP_LOG_BUFFER_HEX(TAG, param->scan_rst.bda, 6);
    
    memcpy(last_attempt_bda, param->scan_rst.bda, 6);
    last_attempt_time = now;
    
    esp_ble_gap_stop_scanning();
    is_scanning = false;
    connection_in_progress = true;
    
    memcpy(server_bda, param->scan_rst.bda, sizeof(esp_bd_addr_t));
    esp_ble_gattc_open(mcs_gattc_if, server_bda, param->scan_rst.ble_addr_type, true);
}

void mcs_process_ui_updates(void)
{
    if (ui_update_queue == NULL) return;
    ui_update_msg_t msg;
    
    while (xQueueReceive(ui_update_queue, &msg, 0) == pdTRUE) {
        switch (msg.type) {
        case UI_UPDATE_TRACK_TITLE:
            if (objects.trackname) {
                lv_label_set_text(objects.trackname, client_title);
                lv_obj_invalidate(objects.trackname);
            }
            
            if (objects.artistname) {
                lv_label_set_text(objects.artistname, client_artist);
                lv_obj_invalidate(objects.artistname);
            }
            break;
            
        case UI_UPDATE_MEDIA_STATE:
            {
                const char *state_text = (msg.data.media_state == 0x01) ? "Playing" : 
                                        (msg.data.media_state == 0x02) ? "Paused" : "Stopped";
                
                if (objects.albumname) {
                    lv_label_set_text(objects.albumname, state_text);
                    lv_obj_invalidate(objects.albumname);
                }
            }
            break;
            
        case UI_UPDATE_TRACK_POSITION:
            break;
            
        case UI_UPDATE_ALBUM_ART:
            if (objects.albumart && album_art_valid) {
                static lv_image_dsc_t album_img;
                album_img.header.cf = LV_COLOR_FORMAT_RGB565;
                album_img.header.w = 64;
                album_img.header.h = 64;
                album_img.data = client_album_art;
                album_img.data_size = 8192;
                
                lv_image_set_src(objects.albumart, &album_img);
                lv_obj_invalidate(objects.albumart);
                
                ESP_LOGI(TAG, "Album displayed");
            }
            break;
        }
    }
}

// Getters
const char* mcs_get_track_title(void) { return client_track_title; }
const char* mcs_get_artist(void) { return client_artist; }
const char* mcs_get_title(void) { return client_title; }
uint8_t mcs_get_media_state(void) { return client_media_state; }
uint32_t mcs_get_track_position(void) { return client_track_position; }
uint32_t mcs_get_track_duration(void) { return client_track_duration; }

uint32_t mcs_get_estimated_position(void)
{
    if (!is_playing) {
        return client_track_position;
    }
    
    int64_t now_ms = esp_timer_get_time() / 1000;
    int64_t elapsed_ms = now_ms - playback_start_time_ms;
    return (uint32_t)(elapsed_ms / 1000);
}

bool mcs_is_connected(void)
{ 
    if (!is_connected) return false;
    
    int64_t now = esp_timer_get_time() / 1000;
    
    if (last_notify_time == 0) {
        return true;
    }
    
    if ((now - last_notify_time) > 20000) {
        ESP_LOGW(TAG, "No heartbeat for 20s");
        force_disconnect_cleanup();
        return false;
    }
    
    return true;
}

bool mcs_is_album_transfer_in_progress(void)
{
    return album_transfer_in_progress;
}