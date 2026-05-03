# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
list(APPEND PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_version/boards)
set(PICO_BOARD azbyn_rp2350b CACHE STRING "Board type")
# set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(../pico_sdk_import.cmake)

project(azbyn_keyboard C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(azbyn_keyboard
            main.cpp
            usb_hid.cpp
            keyboard_logic.cpp
            )
#tusb shows a lot of warnings with -Wpedantic
target_compile_options(azbyn_keyboard PUBLIC -Wall -Wextra)
# target_compile_options(azbyn_keyboard PUBLIC -Wall -Wextra -Wpedantic)

pico_set_program_name(azbyn_keyboard "azbyn_keyboard")
pico_set_program_version(azbyn_keyboard "1.1.1")

# Modify the below lines to enable/disable output over UART/USB
# pico_enable_stdio_uart(azbyn_keyboard 0)
# pico_enable_stdio_usb(azbyn_keyboard 1)



# Add the standard library to the build
target_link_libraries(azbyn_keyboard
        hardware_spi
        pico_multicore
        tinyusb_device
        tinyusb_board
        pico_stdlib)


# Add the standard include files to the build
target_include_directories(azbyn_keyboard PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(azbyn_keyboard)

