# ========== PATCHES FOR ESP-IDF 5.4.1 COMPATIBILITY ==========
message(STATUS "🔧 Applying component patches...")

set(ARDUINO_I2C_FILE "${CMAKE_CURRENT_SOURCE_DIR}/managed_components/espressif__arduino-esp32/cores/esp32/esp32-hal-i2c-slave.c")
set(TFT_ESPI_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/components/TFT_eSPI/CMakeLists.txt")
set(ESP_INSIGHTS_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/managed_components/espressif__esp_insights/CMakeLists.txt")
set(ESP_RAINMAKER_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/managed_components/espressif__esp_rainmaker/CMakeLists.txt")

# Arduino I2C patch
set(ARDUINO_I2C_FILE "${CMAKE_CURRENT_SOURCE_DIR}/managed_components/espressif__arduino-esp32/cores/esp32/esp32-hal-i2c-slave.c")

if(EXISTS "${ARDUINO_I2C_FILE}")
    message(STATUS "Found Arduino I2C file: ${ARDUINO_I2C_FILE}")
    file(READ "${ARDUINO_I2C_FILE}" I2C_CONTENT)
    string(FIND "${I2C_CONTENT}" "Function doesn't exist in ESP-IDF 5.4.1" I2C_PATCH_FOUND)
    
    if(I2C_PATCH_FOUND EQUAL -1)
        message(STATUS "Applying Arduino I2C patch...")
        string(REPLACE "i2c_ll_slave_set_fifo_mode(i2c->dev, true);" 
                      "// i2c_ll_slave_set_fifo_mode(i2c->dev, true);  // Function doesn't exist in ESP-IDF 5.4.1\n  i2c_ll_enable_fifo_mode(i2c->dev, true);" 
                      I2C_CONTENT "${I2C_CONTENT}")
        string(REPLACE "i2c_ll_set_mode(i2c->dev, I2C_BUS_MODE_SLAVE);" 
                      "// i2c_ll_set_mode(i2c->dev, I2C_BUS_MODE_SLAVE);" 
                      I2C_CONTENT "${I2C_CONTENT}")
        string(REPLACE "i2c_ll_enable_pins_open_drain(i2c->dev, true);" 
                      "// i2c_ll_enable_pins_open_drain(i2c->dev, true);" 
                      I2C_CONTENT "${I2C_CONTENT}")
        file(WRITE "${ARDUINO_I2C_FILE}" "${I2C_CONTENT}")
        message(STATUS "✅ Arduino I2C patched!")
    else()
        message(STATUS "✅ Arduino I2C already patched")
    endif()
else()
    message(WARNING "❌ Arduino I2C file not found: ${ARDUINO_I2C_FILE}")
    message(WARNING "Managed components may not be downloaded yet!")
endif()

# TFT_eSPI Arduino patch
if(EXISTS "${TFT_ESPI_CMAKE}")
    file(READ "${TFT_ESPI_CMAKE}" TFT_CONTENT)
    string(FIND "${TFT_CONTENT}" "espressif__arduino-esp32" TFT_PATCH_FOUND)
    
    if(TFT_PATCH_FOUND EQUAL -1)
        message(STATUS "Applying TFT_eSPI Arduino dependency patch...")
        string(REGEX REPLACE "REQUIRES([^\n]*) arduino" 
                             "REQUIRES\\1 espressif__arduino-esp32" 
                             TFT_CONTENT "${TFT_CONTENT}")
        string(REGEX REPLACE "PRIV_REQUIRES([^\n]*) arduino" 
                             "PRIV_REQUIRES\\1 espressif__arduino-esp32" 
                             TFT_CONTENT "${TFT_CONTENT}")
        file(WRITE "${TFT_ESPI_CMAKE}" "${TFT_CONTENT}")
        message(STATUS "✅ TFT_eSPI patched!")
    else()
        message(STATUS "✅ TFT_eSPI already patched")
    endif()
endif()

# BQ27441 Arduino patch
set(BQ27441_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/components/esp32-bq27441/CMakeLists.txt")
if(EXISTS "${BQ27441_CMAKE}")
    file(READ "${BQ27441_CMAKE}" BQ27441_CONTENT)
    string(FIND "${BQ27441_CONTENT}" "espressif__arduino-esp32" BQ27441_PATCH_FOUND)
    
    if(BQ27441_PATCH_FOUND EQUAL -1)
        message(STATUS "Applying BQ27441 Arduino dependency patch...")
        string(REGEX REPLACE "REQUIRES([^\n]*) driver" 
                             "REQUIRES\\1 driver espressif__arduino-esp32" 
                             BQ27441_CONTENT "${BQ27441_CONTENT}")
        file(WRITE "${BQ27441_CMAKE}" "${BQ27441_CONTENT}")
        message(STATUS "✅ BQ27441 patched!")
    else()
        message(STATUS "✅ BQ27441 already patched")
    endif()
endif()

# Adafruit_TCA8418 Arduino patch
set(TCA8418_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/components/Adafruit_TCA8418/CMakeLists.txt")
if(EXISTS "${TCA8418_CMAKE}")
    file(READ "${TCA8418_CMAKE}" TCA8418_CONTENT)
    string(FIND "${TCA8418_CONTENT}" "espressif__arduino-esp32" TCA8418_PATCH_FOUND)
    
    if(TCA8418_PATCH_FOUND EQUAL -1)
        message(STATUS "Creating Adafruit_TCA8418 CMakeLists.txt...")
        set(TCA8418_CONTENT "idf_component_register(\n    SRCS \"Adafruit_TCA8418.cpp\"\n    INCLUDE_DIRS \".\"\n    REQUIRES espressif__arduino-esp32\n)\n")
        file(WRITE "${TCA8418_CMAKE}" "${TCA8418_CONTENT}")
        message(STATUS "✅ Adafruit_TCA8418 patched!")
    else()
        message(STATUS "✅ Adafruit_TCA8418 already patched")
    endif()
endif()

# Adafruit_BusIO patch
set(BUSIO_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/components/Adafruit_BusIO/CMakeLists.txt")
if(EXISTS "${BUSIO_CMAKE}")
    file(READ "${BUSIO_CMAKE}" BUSIO_CONTENT)
    string(FIND "${BUSIO_CONTENT}" "espressif__arduino-esp32" BUSIO_PATCH_FOUND)
    
    if(BUSIO_PATCH_FOUND EQUAL -1)
        message(STATUS "Creating Adafruit_BusIO CMakeLists.txt...")
        set(BUSIO_CONTENT "idf_component_register(\n    SRCS\n        \"Adafruit_BusIO_Register.cpp\"\n        \"Adafruit_I2CDevice.cpp\"\n        \"Adafruit_SPIDevice.cpp\"\n    INCLUDE_DIRS \".\"\n    REQUIRES espressif__arduino-esp32\n)\n")
        file(WRITE "${BUSIO_CMAKE}" "${BUSIO_CONTENT}")
        message(STATUS "✅ Adafruit_BusIO patched!")
    else()
        message(STATUS "✅ Adafruit_BusIO already patched")
    endif()
endif()

# ESP Insights git patch
if(EXISTS "${ESP_INSIGHTS_CMAKE}")
    file(READ "${ESP_INSIGHTS_CMAKE}" INSIGHTS_CONTENT)
    string(FIND "${INSIGHTS_CONTENT}" "set(ESP_INSIGHTS_VERSION \"unknown\")" INSIGHTS_PATCH_FOUND)
    
    if(INSIGHTS_PATCH_FOUND EQUAL -1)
        message(STATUS "Applying ESP Insights git patch...")
        string(REPLACE "git_describe(ESP_INSIGHTS_VERSION \${COMPONENT_DIR})" 
                      "set(ESP_INSIGHTS_VERSION \"unknown\")" 
                      INSIGHTS_CONTENT "${INSIGHTS_CONTENT}")
        file(WRITE "${ESP_INSIGHTS_CMAKE}" "${INSIGHTS_CONTENT}")
        message(STATUS "✅ ESP Insights patched!")
    else()
        message(STATUS "✅ ESP Insights already patched")
    endif()
endif()

# ESP RainMaker git patch
if(EXISTS "${ESP_RAINMAKER_CMAKE}")
    file(READ "${ESP_RAINMAKER_CMAKE}" RAINMAKER_CONTENT)
    string(FIND "${RAINMAKER_CONTENT}" "set(RMAKER_VERSION \"unknown\")" RAINMAKER_PATCH_FOUND)
    
    if(RAINMAKER_PATCH_FOUND EQUAL -1)
        message(STATUS "Applying ESP RainMaker git patch...")
        string(REPLACE "git_describe(RMAKER_VERSION \${COMPONENT_DIR})" 
                      "set(RMAKER_VERSION \"unknown\")" 
                      RAINMAKER_CONTENT "${RAINMAKER_CONTENT}")
        file(WRITE "${ESP_RAINMAKER_CMAKE}" "${RAINMAKER_CONTENT}")
        message(STATUS "✅ ESP RainMaker patched!")
    else()
        message(STATUS "✅ ESP RainMaker already patched")
    endif()
endif()

message(STATUS "🎉 All patches applied successfully!")