================================================================================
ESP_MKB - BLE MACRO KEYBOARD - DEVELOPMENT STATE SNAPSHOT
================================================================================
Date: February 26, 2026
ESP-IDF: v5.4.3
Arduino: espressif__arduino-esp32 (managed component)
Hardware: ESP32, ST7789 TFT, TCA8418 Keypad, BQ27441 Battery Gauge, WS2812 LEDs
Status: PAIRING WORKS | BATTERY WORKS | HID TRANSMISSION ISSUE

================================================================================
PROJECT STRUCTURE (CURRENT)
================================================================================

main/
  main.cpp                    - Entry point, main loop
  init.h                      - Type definitions, constants, extern declarations
  init.cpp                    - Global variable definitions, encoder ISR
  ble_config.h                - BLE service declarations
  ble_config.cpp              - BLE descriptors, characteristics, advertising data
  bluetoothfunctions.h        - BLE function declarations
  bluetoothfunctions.cpp      - GAP/GATTS handlers, send_consumer_key, send_keyboard_key
  setupfunctions.h            - Setup function declarations
  setupfunctions.cpp          - Hardware init (I2C, SPI, ADC, BLE, etc)
  dispfunctions.h             - Display function declarations
  dispfunctions.cpp           - TFT rendering, pairing screen, icons
  filefunctions.h             - File I/O declarations
  filefunctions.cpp           - LittleFS, JSON parsing, button mapping loader
  menufunctions.h             - Menu system declarations
  menufunctions.cpp           - Menu parsing, navigation, action callbacks
  functions.h                 - Utility function declarations
  functions.cpp               - keyPressed handler, LED control, encoder
  pinDef.h                    - GPIO pin definitions
  icons.h                     - Custom GFX font (iconFont)
  buttons.json                - Button mapping config (embedded in firmware)
  menu.json                   - Menu structure config (embedded in firmware)
  CMakeLists.txt              - Component build configuration

  UNUSED FILES (safe to delete):
    hid_dev.h, webFunctions.h, index.html, Playwrite.ttf

components/
  TFT_eSPI/                   - Display driver
  Adafruit_TCA8418/           - Keypad driver
  esp-idf-bq27441/            - Battery gauge driver
  led_strip/                  - WS2812 LED driver
  ESP32-A2DP/                 - Included but not used

managed_components/
  espressif__arduino-esp32/   - Arduino framework

Root:
  CMakeLists.txt              - Main build config with component patches
  partitions.csv              - Partition table

================================================================================
WHAT IS WORKING
================================================================================

BLUETOOTH:
  - BLE GATT Server with 3 services (HID, Battery, Media Control)
  - Pairing via Numeric Comparison (ESP_IO_CAP_IO)
  - Passkey displays correctly on TFT (6 digits with leading zeros)
  - Bonding persists across reboots
  - Security: LE Secure Connections with MITM protection
  - Windows recognizes as HID Keyboard device
  - Connection stable, no disconnects

HARDWARE:
  - TFT Display rendering (ST7789, 240x280)
  - TCA8418 keypad scanning (3x4 matrix)
  - BQ27441 battery SOC reporting (updates to Windows)
  - WS2812 LED control (18 LEDs)
  - Rotary encoder (interrupt-based)
  - 2x Potentiometers (ADC with averaging)
  - Charging detection (MCP73871 status pins)

SOFTWARE:
  - LittleFS mounted on 1984K partition
  - JSON files embedded in firmware and copied to storage
  - Button mapping system (10 profiles, 8 buttons each)
  - Menu system (hierarchical navigation)
  - Display queue (thread-safe BLE event handling)

================================================================================
CURRENT ISSUE - HID KEYS NOT WORKING
================================================================================

SYMPTOMS:
  - Buttons detected: "Executing command: 177 for key: a"
  - NO transmission logs: Missing "→ Sending CONSUMER key" messages
  - Windows does not respond to key presses

ROOT CAUSE:
  Command type detection logic in functions.cpp is incorrect
  Command 177 (0xB1) decimal is in range 4-231 so treated as keyboard key
  Should be detected as consumer control key

EXPECTED BEHAVIOR:
  Press button 'a' (cmd 177) should show:
    Executing command: 177 for key: a
    I (xxx) FUNC: → Sending CONSUMER key: 0x00B1 (177)
    I (xxx) BLE: Sending consumer key: 0x00B1

LAST FIX APPLIED:
  Updated functions.cpp with explicit consumer key value checks
  Changed from range-based to value-based detection
  Added return statement after handling button mappings
  Re-added missing switch(key) statement for navigation keys

ACTION REQUIRED:
  Flash the updated functions.cpp and verify logs

================================================================================
BLE SERVICE DETAILS
================================================================================

HID SERVICE (0x1812) - Profile ID 2 - MUST BE REGISTERED FIRST
  Char 0: HID Information (0x2A4A) - READ - 4 bytes - bcdHID 1.11, RemoteWake
  Char 1: HID Report Map (0x2A4B) - READ - ~115 bytes - Dual report descriptor
  Char 2: HID Control Point (0x2A4C) - WRITE - 1 byte
  Char 3: Protocol Mode (0x2A4E) - READ/WRITE - 1 byte - Report mode
  Char 4: HID Keyboard Report (0x2A4D) - READ/NOTIFY - 8 bytes - Report ID 1
  Char 5: HID Consumer Report (0x2A4D) - READ/NOTIFY - 3 bytes - Report ID 2
  Handle mapping: char_handles[HID_APP_ID][0-5]

BATTERY SERVICE (0x180F) - Profile ID 1
  Char 0: Battery Level (0x2A19) - READ/NOTIFY - 1 byte
  Updates automatically from BQ27441

MEDIA CONTROL SERVICE (0x1848) - Profile ID 0
  Char 0: Media State (0x2BA3) - READ/WRITE
  Char 1: Media Control Point (0x2BA4) - READ/NOTIFY
  Char 2: Track Title (0x2B97) - WRITE
  Char 3: Track Duration (0x2B98) - WRITE
  Char 4: Track Position (0x2B99) - WRITE
  Char 5: Volume Control (0x2B7E) - READ/WRITE/NOTIFY

HID REPORT FORMAT:
  Report ID 1 (Keyboard): [modifier, reserved, key1, key2, key3, key4, key5, key6]
  Report ID 2 (Consumer): [0x02, keycode_low_byte, keycode_high_byte]

CCCD DESCRIPTORS:
  Auto-added for all characteristics with PROP_NOTIFY or PROP_INDICATE
  UUID: 0x2902
  Currently not storing handles (causes "unknown handle" warnings but works)

================================================================================
BUTTONS.JSON FORMAT (CURRENT)
================================================================================

Structure:
  "buttons" array contains ButtonSet objects
  Each ButtonSet has "set_name" and "content" array
  Each content item has "key_id", "icon", "name", "cmd"

Current media button set:
  a: 177 (Pause)
  b: 226 (Mute)
  c: 234 (Volume Down)
  d: 233 (Volume Up)
  e: 182 (Previous Track)
  f: 181 (Next Track)
  g: 233 (duplicate - should be different)
  h: 900 (Clear bonds - should change to 255)

Button mapping type:
  typedef struct {
      char key_id;      // 'a'-'h'
      char icon;        // Icon font character
      uint16_t cmd;     // HID command code (0-65535)
  } ButtonMapping;

================================================================================
HARDWARE CONNECTIONS
================================================================================

I2C BUS (I2C_NUM_0, 100kHz):
  TCA8418 at 0x34 - Keypad scanner
  BQ27441 at 0x55 - Battery fuel gauge

SPI BUS (TFT Display):
  ST7789 240x280 via TFT_eSPI library
  Pins defined in TFT_eSPI User_Setup

GPIO ASSIGNMENTS:
  LEDS (GPIO 5) - WS2812 data line, 18 LEDs
  ENC_A, ENC_B - Rotary encoder with interrupts
  POT_1 (ADC_CHANNEL_3) - Volume pot
  POT_2 (ADC_CHANNEL_0) - Generic pot
  CHRG, PWR, DONE - MCP73871 charger status
  SLP_IO - Power management
  MINT - TCA8418 interrupt input
  MRES - TCA8418 reset output
  SPI_BL - TFT backlight control

KEYPAD LAYOUT:
  3 rows x 4 columns
  Keymap: [a,e,j] [b,f,i] [c,g,k] [d,h,l]
  Visual: a-d (left set), e-h (right set), i-l (navigation)

MISSING HARDWARE:
  DTR/RTS auto-reset circuit - requires manual BOOT+EN press to flash
  Recommended: DTR→100nF→EN+10kΩ↓, RTS→100nF→GPIO0+10kΩ↓

================================================================================
KEY FUNCTIONS REFERENCE
================================================================================

BLE FUNCTIONS (bluetoothfunctions.cpp):
  setupBluetooth() - Init Bluedroid stack, set security params, register services
  gap_event_handler() - Handle pairing, passkey display, auth completion
  gatts_event_handler() - Main GATT server dispatcher
  gatts_profile_event_handler() - Generic service handler
  send_consumer_key(uint16_t) - Send media key HID report (Report ID 2)
  send_keyboard_key(uint8_t, uint8_t) - Send keyboard key (Report ID 1)
  update_battery_level(uint8_t) - Notify battery change
  update_volume(uint8_t) - Update and notify volume
  clearAllBonds() - Remove all paired devices from NVS

DISPLAY FUNCTIONS (dispfunctions.cpp):
  screenDefault() - Main grid screen with status icons
  blePairingCode() - Show 6-digit passkey during pairing
  showGrid() - 8-button icon grid with highlighting
  displayTrackInfo() - Media playback info overlay
  drawBatteryIcon() - Battery level with percentage

INPUT HANDLING (functions.cpp):
  keyPressed(char) - Main button handler, detects command type, sends HID
  readEncoder() - Debounced encoder reading
  handle_ble_display_events() - Process display queue (in main.cpp)

FILE OPERATIONS (filefunctions.cpp):
  copy_embedded_json_to_littlefs() - Extract embedded JSON to filesystem
  loadButtonMappings() - Parse buttons.json into ButtonSet array
  getActiveButtonMappings() - Return current button profile

================================================================================
PARTITION TABLE
================================================================================

nvs      (0x9000)   - 20K   - BLE bonds, WiFi credentials
otadata  (0xE000)   - 8K    - OTA boot selection
app0     (0x10000)  - 1984K - Firmware slot
storage  (0x200000) - 1984K - LittleFS (buttons.json, menu.json)
coredump (0x3F0000) - 64K   - Crash dumps

================================================================================
KNOWN ISSUES & SOLUTIONS
================================================================================

ISSUE: Unknown handle errors during pairing
  Logs: "Read request for unknown handle: 77"
  Cause: Windows reading CCCD descriptors not tracked
  Impact: Warnings only, doesn't break functionality
  Fix: Store CCCD handles in ESP_GATTS_ADD_CHAR_DESCR_EVT

ISSUE: Crash when calling display from BLE callback
  Solution: Use ble_display_queue for thread-safe updates

ISSUE: Command 900 out of uint8_t range
  Solution: Changed ButtonMapping.cmd to uint16_t

ISSUE: Passkey shows 5 digits instead of 6
  Solution: Use snprintf with "%06d" format

ISSUE: Commitment check fails / Enumerate ceremonies error
  Solution: Changed ESP_IO_CAP_OUT to ESP_IO_CAP_IO

================================================================================
CRITICAL CODE SNIPPETS
================================================================================

BLE DISPLAY QUEUE (thread-safe):
  typedef struct {
      enum { DISPLAY_PASSKEY, DISPLAY_SUCCESS, DISPLAY_FAILED, DISPLAY_CLEAR } type;
      uint32_t passkey;
  } ble_display_event_t;
  QueueHandle_t ble_display_queue = NULL;

SEND CONSUMER KEY (bluetoothfunctions.cpp):
  void send_consumer_key(uint16_t key_code) {
      uint16_t char_handle = char_handles[HID_APP_ID][5];
      uint8_t report[3] = {0x02, (uint8_t)(key_code & 0xFF), (uint8_t)(key_code >> 8)};
      esp_ble_gatts_send_indicate(..., 3, report, false);
      vTaskDelay(pdMS_TO_TICKS(100));
      uint8_t release[3] = {0x02, 0x00, 0x00};
      esp_ble_gatts_send_indicate(..., 3, release, false);
  }

COMMAND DETECTION LOGIC (functions.cpp - LATEST FIX):
  Explicit value checks for consumer keys (177, 181, 182, 205, 226, 233, 234)
  Range 4-101 for keyboard keys
  Range 224-231 for modifier keys
  Special: 255/900 for clear bonds

CCCD AUTO-ADD (bluetoothfunctions.cpp):
  In ESP_GATTS_ADD_CHAR_EVT:
    uint16_t properties = service->characteristics[current_char_idx[profile_id]].properties;
    bool needs_cccd = (properties & PROP_NOTIFY) || (properties & PROP_INDICATE);
    if (needs_cccd) { /* add descriptor 0x2902 */ }

================================================================================
CURRENT DEBUG STATE
================================================================================

LAST SERIAL OUTPUT:
  W (916) BT_BTM: BTM_BleWriteAdvData, Partial data write into ADV
  File: buttons.json
  File: menu.json
  Executing command: 177 for key: a
  Executing command: 226 for key: b
  Executing command: 900 for key: h

MISSING FROM LOGS (expected after fix):
  I (xxx) FUNC: → Sending CONSUMER key: 0x00B1 (177)
  I (xxx) BLE: Sending consumer key: 0x00B1

WINDOWS DEVICE STATUS:
  Device: BTHLEDevice\{00001812-0000-1000-8000-00805f9b34fb}_2cbcbbdfd5b2
  Driver: hidbthle.inf (Microsoft HID over Bluetooth LE)
  Class GUID: {745a17a0-74d3-11d0-b6fe-00a0c90f57da} (HID)
  Status: Paired and connected successfully
  Problem: Keys not responding (HID reports not received)

================================================================================
IMMEDIATE TODO (PRIORITY ORDER)
================================================================================

1. FLASH UPDATED functions.cpp
   - Contains fixed command type detection
   - Uses explicit value checks instead of ranges
   - Should show "→ Sending CONSUMER key" in logs

2. VERIFY HID TRANSMISSION LOGS
   - Must see both "Executing command" AND "→ Sending" messages
   - Check handle for char_handles[HID_APP_ID][5] is not 0

3. TEST WINDOWS HID RECEPTION
   - Use HID monitor tool OR
   - Check Event Viewer: Microsoft → Windows → Bluetooth → Operational

4. FIX CCCD HANDLE TRACKING
   - Update ESP_GATTS_ADD_CHAR_DESCR_EVT case
   - Store descriptor handles to eliminate warnings

5. UPDATE buttons.json
   - Change cmd 900 → 255 (clear bonds)
   - Verify all command codes are correct

6. ADD AUTO-RESET CIRCUIT
   - DTR → 100nF cap → EN + 10kΩ pull-down
   - RTS → 100nF cap → GPIO0 + 10kΩ pull-down

================================================================================
BUILD CONFIGURATION
================================================================================

main/CMakeLists.txt:
  SRCS: main.cpp init.cpp ble_config.cpp bluetoothfunctions.cpp setupfunctions.cpp
        dispfunctions.cpp filefunctions.cpp menufunctions.cpp functions.cpp
  EMBED_TXTFILES: menu.json buttons.json
  REQUIRES: json joltwallet__littlefs bt nvs_flash led_strip esp-idf-bq27441
            Adafruit_TCA8418 TFT_eSPI esp_adc espressif__arduino-esp32

Root CMakeLists.txt patches:
  - TFT_eSPI: arduino → espressif__arduino-esp32
  - BQ27441: add espressif__arduino-esp32 dependency
  - Adafruit_TCA8418: create CMakeLists with arduino dependency
  - Arduino I2C: i2c_ll_slave_set_fifo_mode → i2c_ll_enable_fifo_mode
  - ESP Insights/RainMaker: disable git_describe

================================================================================
BLE PAIRING FLOW
================================================================================

1. Advertisement starts on boot (HID UUID in adv data, appearance 0x03C1)
2. Windows initiates connection
3. ESP_GAP_BLE_NC_REQ_EVT fired with 6-digit passkey
4. Event queued to ble_display_queue
5. Main task calls handle_ble_display_events()
6. blePairingCode(true, "Verify match:", passkey) displays on TFT
7. User verifies passkey matches on Windows and clicks "Yes"
8. esp_ble_confirm_reply() auto-accepts on ESP32 side
9. ESP_GAP_BLE_AUTH_CMPL_EVT fires with success=true
10. DISPLAY_SUCCESS queued, shows "Connected!" for 2 seconds
11. Bond saved to NVS, returns to screenDefault()

SECURITY PARAMETERS:
  auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND
  iocap = ESP_IO_CAP_IO
  key_size = 16
  init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK
  rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK

================================================================================
HID COMMAND REFERENCE
================================================================================

CONSUMER CONTROL COMMANDS (USB HID Consumer Page):
  176 (0xB0) - Play
  177 (0xB1) - Pause
  178 (0xB2) - Record
  179 (0xB3) - Fast Forward
  180 (0xB4) - Rewind
  181 (0xB5) - Scan Next Track
  182 (0xB6) - Scan Previous Track
  183 (0xB7) - Stop
  184 (0xB8) - Eject
  205 (0xCD) - Play/Pause (most compatible)
  206 (0xCE) - Play/Skip
  226 (0xE2) - Mute
  233 (0xE9) - Volume Up (Volume Increment)
  234 (0xEA) - Volume Down (Volume Decrement)

KEYBOARD COMMANDS (USB HID Keyboard Page):
  4-29   (0x04-0x1D) - A-Z
  30-39  (0x1E-0x27) - 1-9, 0
  40     (0x28) - Enter
  41     (0x29) - Escape
  42     (0x2A) - Backspace
  43     (0x2B) - Tab
  44     (0x2C) - Spacebar
  58-69  (0x3A-0x45) - F1-F12
  74-82  (0x4A-0x52) - Home, PgUp, Del, End, PgDn, Right, Left, Down, Up

MODIFIER KEYS (Bitmask in byte 0):
  224 (0xE0) - Left Ctrl   - Bit 0
  225 (0xE1) - Left Shift  - Bit 1
  226 (0xE2) - Left Alt    - Bit 2
  227 (0xE3) - Left GUI    - Bit 3
  228 (0xE4) - Right Ctrl  - Bit 4
  229 (0xE5) - Right Shift - Bit 5
  230 (0xE6) - Right Alt   - Bit 6
  231 (0xE7) - Right GUI   - Bit 7

SPECIAL COMMANDS:
  255 (0xFF) - Clear BLE bonds (internal command)
  900 - Legacy clear bonds (update JSON to use 255)

================================================================================
COMMON DEVELOPMENT COMMANDS
================================================================================

BUILD & FLASH:
  idf.py build
  idf.py flash monitor
  idf.py fullclean  (clean rebuild)

MONITORING:
  idf.py monitor
  idf.py monitor --print-filter="BLE:I FUNC:I SETUP:I"

PARTITION MANAGEMENT:
  idf.py erase-flash (erase everything)
  idf.py erase-nvs (clear bonds only)

CONFIGURATION:
  idf.py menuconfig
  idf.py reconfigure

================================================================================
TROUBLESHOOTING GUIDE
================================================================================

PAIRING FAILS:
  1. Clear Windows Bluetooth cache (Settings → Remove device)
  2. Clear ESP32 bonds (press button with cmd 255)
  3. Power cycle ESP32
  4. Re-pair from scratch

HID KEYS NOT WORKING:
  1. Check logs for "→ Sending CONSUMER key" messages
  2. Verify handle: char_handles[HID_APP_ID][5] is not 0
  3. Check Windows subscribed to CCCD (should auto-happen)
  4. Try different media key codes (205 for Play/Pause is most compatible)

DISPLAY ISSUES:
  1. All display updates must go through ble_display_queue
  2. Never call tft.* functions from BLE callbacks
  3. Check handle_ble_display_events() is called in main loop

CONNECTION DROPS:
  1. Check battery level (low battery can cause disconnects)
  2. Verify advertising restarts in ESP_GATTS_DISCONNECT_EVT
  3. Check Windows power management (disable USB selective suspend)

================================================================================
CODEBASE PATTERNS
================================================================================

ADDING NEW BLE CHARACTERISTIC:
  1. Add to appropriate *_characteristics[] array in ble_config.cpp
  2. Increment char_count
  3. Handle in gatts_profile_event_handler WRITE/READ events
  4. Add CCCD if using NOTIFY/INDICATE

ADDING NEW BUTTON COMMAND:
  1. Define constant in init.h or use raw value
  2. Add to buttons.json
  3. Add detection in keyPressed() in functions.cpp
  4. Call appropriate send_* function

ADDING NEW DISPLAY SCREEN:
  1. Create function in dispfunctions.cpp
  2. Add declaration to dispfunctions.h
  3. Call from main loop or event handler

ADDING NEW MENU ITEM:
  1. Add to menu.json structure
  2. Create action function in menufunctions.cpp
  3. Link type in menu.json to function name

================================================================================
RECENT CHANGES LOG
================================================================================

Feb 26, 2026 - Late Afternoon:
  - Restructured all .h files to .h/.cpp pairs
  - Moved all implementations to .cpp files
  - Fixed thread safety issues with display queue
  - Added CCCD auto-generation for NOTIFY characteristics
  - Implemented send_consumer_key() and send_keyboard_key()
  - Fixed passkey display formatting (6 digits)
  - Changed pairing from passkey entry to numeric comparison
  - Updated command type detection logic
  - Added proper HID Report Map with dual report IDs
  - Fixed ButtonMapping.cmd type (uint8_t → uint16_t)

Feb 26, 2026 - Earlier:
  - Resolved pairing issues (ESP_IO_CAP_IO)
  - Fixed SPI task priority crashes
  - Added complete HID service descriptors
  - Verified battery reporting works

================================================================================
COMPILER WARNINGS (NON-CRITICAL)
================================================================================

Can be ignored:
  - TFT_eSPI: Invalid Reset pin, TOUCH_CS not defined
  - Arduino I2S: missing allow_pd field
  - ESPAsyncWebServer: ESP32 redefined
  - arduino-audio-tools: htons/ntohs redefined
  - NetworkClientSecure: PSK ciphersuites warning
  - Various missing-field-initializers (Arduino libraries)

Duplicate Arduino component:
  components/arduino/ exists but managed_components/ is used
  Safe to delete components/arduino/

================================================================================
ADVANCED FEATURES (NOT YET IMPLEMENTED)
================================================================================

POTENTIAL ADDITIONS:
  - Modifier key combinations (Ctrl+C, Alt+Tab, etc)
  - Multi-key macros/sequences
  - Typing full strings
  - Keyboard repeat/hold
  - WiFi configuration via menu
  - Web interface for configuration
  - OTA firmware updates
  - RGB LED animations/patterns
  - Sleep/wake optimization
  - A2DP audio output (ESP32-A2DP component available)

================================================================================
QUICK REFERENCE - BUTTON FUNCTIONS
================================================================================

MAPPED BUTTONS (a-h):
  Defined in active ButtonSet from buttons.json
  Current set: "media" (button set 0)
  Change sets with 'i' and 'l' keys

NAVIGATION BUTTONS (i-l):
  i - Next button set (cycle through ButtonSet array)
  j - Enter menu / select item (push to menu stack)
  k - Back / exit menu (pop from menu stack)
  l - Previous button set + print battery stats

ENCODER:
  Rotate - Navigate menu items (when menu active)
  Value range set dynamically based on menu size

POTENTIOMETERS:
  POT_1 - Volume control (updates BLE volume characteristic)
  POT_2 - User-defined (currently just LED feedback)

================================================================================
WINDOWS DEBUGGING TOOLS
================================================================================

EVENT VIEWER:
  Applications and Services → Microsoft → Windows → Bluetooth → Operational
  Look for pairing events and HID report events

DEVICE MANAGER:
  Find ESP-MKB under "Bluetooth" or "Human Interface Devices"
  Check Properties → Events tab for driver installation history
  Check Properties → Details → Hardware IDs

SETUP API LOG:
  C:\Windows\INF\setupapi.dev.log
  Search for ESP-MKB or MAC address
  Shows detailed driver installation process

POWERSHELL HID MONITOR:
  Get-PnpDevice | Where-Object {$_.FriendlyName -like "*ESP*"}
  Shows device status and problem codes

RECOMMENDED TOOLS:
  USB Device Tree Viewer (better than Spy++)
  RawInput Monitor (for HID event inspection)

================================================================================
FILE DEPENDENCIES GRAPH
================================================================================

main.cpp includes:
  → init.h → ble_config.h → bluetoothfunctions.h
  → filefunctions.h → menufunctions.h → dispfunctions.h → functions.h

init.h includes:
  → Arduino headers (Wire.h, TFT_eSPI.h, etc)
  → ESP-IDF headers (esp_gatts_api.h, esp_gap_ble_api.h, etc)
  → pinDef.h

All .cpp files include their corresponding .h file first
bluetoothfunctions.cpp needs dispfunctions.h (for queue types)
functions.cpp needs all headers (central integration point)

================================================================================
CONFIGURATION FILES
================================================================================

buttons.json structure:
  {
    "buttons": [
      {
        "set_name": "media",
        "set_icon": "n",
        "content": [
          { "key_id": "a", "icon": "?", "name": "Play/Pause", "cmd": 177 }
        ]
      }
    ]
  }

menu.json structure:
  {
    "menu": [
      {
        "name": "Bluetooth",
        "type": "submenu",
        "submenu": [
          { "name": "Turn on/off", "type": "toggle" },
          { "name": "Back", "type": "back" }
        ]
      }
    ]
  }

================================================================================
NEXT SESSION CHECKLIST
================================================================================

BEFORE STARTING:
  [ ] Verify latest functions.cpp is flashed
  [ ] Check serial logs for "→ Sending CONSUMER key" messages
  [ ] Confirm Windows still paired and connected
  [ ] Have HID monitor tool ready (optional)

FIRST STEPS:
  [ ] Test button 'a' - should send Pause (177/0xB1)
  [ ] Test button 'd' - should send Volume Up (233/0xE9)
  [ ] Check if Windows media responds
  [ ] If not working, check CCCD subscription

INVESTIGATION PATHS:
  A. HID transmission - Why aren't keys reaching Windows?
  B. CCCD handles - How to eliminate unknown handle warnings?
  C. Auto-reset - How to implement DTR/RTS circuit?
  D. Advanced features - Modifiers, macros, sequences?

================================================================================
END OF STATE DOCUMENT
================================================================================

This document captures the complete state of the ESP_MKB project as of
February 26, 2026. All code is properly structured in .h/.cpp pairs,
BLE pairing works reliably, and we are debugging HID key transmission.

Copy this entire document into your next chat session to restore context.
================================================================================