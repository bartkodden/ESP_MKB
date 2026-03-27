cmake_minimum_required(VERSION 3.5)

# Add esp-idf-lib components directory
set(EXTRA_COMPONENT_DIRS 
    "components/esp-idf-lib/components/encoder"
    "components/idf-extra-components/led_strip"
)

# Apply patches (will be executed before project())
include(${CMAKE_CURRENT_SOURCE_DIR}/patches.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP_MKB)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-format")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error -Wno-format")