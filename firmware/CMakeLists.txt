cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS 
    "components/esp-idf-lib/components/encoder"
    "components/idf-extra-components/led_strip"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/patches.cmake)

# Custom partition table — must be set before project.cmake is included
set(CONFIG_PARTITION_TABLE_CUSTOM y)
set(PARTITION_TABLE_CSV_FILEPATH "${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP_MKB)