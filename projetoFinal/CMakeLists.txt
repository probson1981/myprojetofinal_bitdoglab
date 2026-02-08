# Generated Cmake Pico project file

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

# FreeRTOS Kernel import
include(${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Nome do projeto/target
project(projetoFinal C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(projetoFinal
    ${SRC_DIR}/projetoFinal.c

    ${SRC_DIR}/app_tasks.c
    ${SRC_DIR}/mqtt_app.c
    ${SRC_DIR}/net_dns.c
    ${SRC_DIR}/net_wifi.c
    ${SRC_DIR}/serial_rpc.c
    ${SRC_DIR}/json_simple.c
    ${SRC_DIR}/matrix_control.c
    ${SRC_DIR}/system_hooks.c

    ${SRC_DIR}/matrix_led_lib.c
    ${SRC_DIR}/bh1750.c
    ${SRC_DIR}/aht10.c
    ${SRC_DIR}/auto_brightness.c
    ${SRC_DIR}/ssd1306.c
)


# Gera o header a partir do programa PIO do WS2812
pico_generate_pio_header(projetoFinal ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_set_program_name(projetoFinal "projetoFinal")
pico_set_program_version(projetoFinal "0.1")

# stdio (USB ligado, UART desligado)
pico_enable_stdio_uart(projetoFinal 0)
pico_enable_stdio_usb(projetoFinal 1)

# Includes do projeto
target_include_directories(projetoFinal PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link libraries (CONSOLIDADO + MQTT)
target_link_libraries(projetoFinal
    pico_stdlib

    # Hardware usados
    hardware_i2c
    hardware_pio
    hardware_clocks

    # FreeRTOS
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4

    # Wi-Fi + lwIP (threadsafe background)
    pico_cyw43_arch_lwip_threadsafe_background

    # >>> MQTT do lwIP (ESSENCIAL para mqtt_client_new/mqtt_publish etc.)
    pico_lwip_mqtt
    pico_lwip_api
    pico_lwip_core
)

# Outputs (.uf2/.bin/.hex)
pico_add_extra_outputs(projetoFinal)
