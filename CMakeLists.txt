# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================


cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(datalogger C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT_NAME} datalogger.c
        lib/button/button.c # Button library
        lib/led/led.c # LED library
        lib/ssd1306/ssd1306.c # SSD1306 library
        lib/ssd1306/display.c # Display library
        lib/buzzer/buzzer.c # Buzzer library)
        lib/matrix_leds/neopixel.c # Matrix LEDs library
        lib/sensors/mpu6050/mpu6050.c # MPU6050 sensor library
        lib/sd/hw_config.c # SD Utils hardware configuration
        lib/sd/sd_utils.c # SD Utils library
)

pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Generate PIO header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/matrix_leds/ws2812b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

add_subdirectory(lib/sd/FatFs_SPI) # Add SD Utils library

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
        hardware_i2c
        FatFs_SPI # SD Utils library
        hardware_spi
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_adc
        hardware_pwm        
)

pico_add_extra_outputs(${PROJECT_NAME})
