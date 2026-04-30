# LoRa P2P — BitDogLab (RP2040 + SX1276)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(LoRa_P2P C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Dependência externa: SSD1306 OLED
include(FetchContent)
FetchContent_Declare(
    pico_ssd1306
    GIT_REPOSITORY https://github.com/daschr/pico-ssd1306.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(pico_ssd1306)
add_library(ssd1306_lib INTERFACE)
target_include_directories(ssd1306_lib INTERFACE ${pico_ssd1306_SOURCE_DIR})
target_sources(ssd1306_lib INTERFACE ${pico_ssd1306_SOURCE_DIR}/ssd1306.c)

# Executável principal
add_executable(LoRa_P2P
    main.c
    drivers/lora/lora_radio.c
    drivers/display/display.c
    drivers/buttons/buttons.c
)

pico_set_program_name(LoRa_P2P "LoRa_P2P")
pico_set_program_version(LoRa_P2P "0.2")

pico_enable_stdio_uart(LoRa_P2P 0)
pico_enable_stdio_usb(LoRa_P2P 1)

target_link_libraries(LoRa_P2P
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_watchdog
    ssd1306_lib
    m
)

target_include_directories(LoRa_P2P PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/drivers/lora
    ${CMAKE_CURRENT_LIST_DIR}/drivers/display
    ${CMAKE_CURRENT_LIST_DIR}/drivers/buttons
)

pico_add_extra_outputs(LoRa_P2P)
