#ifndef MCS_CLIENT_H
#define MCS_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_gap_ble_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize MCS client
void mcs_client_init(void);

// Use existing HID connection for MCS/VCS
void mcs_use_existing_connection(uint16_t conn_id, esp_bd_addr_t bda);

// Handle scan results
void mcs_handle_scan_result(esp_ble_gap_cb_param_t *param);

// Process UI updates
void mcs_process_ui_updates(void);

// Fetch album art
void mcs_fetch_album_art(void);

// Volume control - sends to VCS or MCS depending on what's available
void mcs_set_volume(uint8_t volume_percent);
void mcs_set_microphone(uint8_t mic_percent);

// Get current media info
const char* mcs_get_track_title(void);
const char* mcs_get_artist(void);
const char* mcs_get_title(void);
uint8_t mcs_get_media_state(void);
uint32_t mcs_get_track_position(void);
uint32_t mcs_get_track_duration(void);
uint32_t mcs_get_estimated_position(void);
bool mcs_is_connected(void);
bool mcs_is_album_transfer_in_progress(void);

#ifdef __cplusplus
}
#endif

#endif // MCS_CLIENT_H