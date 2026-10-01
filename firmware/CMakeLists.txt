cmake_minimum_required(VERSION 3.13)

project(pi_cemaker C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Attempt to include Pico SDK
include(pico_sdk_import.cmake)

if (TARGET pico_stdlib)
    message(STATUS "Pico SDK found! Configuring RP2040 firmware target 'pi_cemaker_fw'...")
    pico_sdk_init()

    add_executable(pi_cemaker_fw
        src/main.c
        src/usb_descriptors.c
        src/telemetry.c
        src/power_mgr.c
        src/cli.c
    )

    target_include_directories(pi_cemaker_fw PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

    target_compile_definitions(pi_cemaker_fw PRIVATE
        PICO_ON_DEVICE=1
    )

    target_link_libraries(pi_cemaker_fw
        pico_stdlib
        pico_multicore
        hardware_adc
        hardware_gpio
        hardware_sync
        hardware_rosc
        hardware_clocks
        tinyusb_device
        tinyusb_board
    )

    pico_enable_stdio_usb(pi_cemaker_fw 0)
    pico_enable_stdio_uart(pi_cemaker_fw 0)

    pico_add_extra_outputs(pi_cemaker_fw)
else()
    message(STATUS "Pico SDK not active. Configuring host test target 'pi_cemaker_test'...")
endif()

# Host targets are only configured without the Pico SDK (they need a hosted libc).
if(NOT TARGET pico_stdlib)
    # Host unit test and validation suite target (can be built on any host machine)
    add_executable(pi_cemaker_test
        tests/test_hid_and_power.c
        src/usb_descriptors.c
        src/telemetry.c
        src/power_mgr.c
        src/cli.c
    )

    target_include_directories(pi_cemaker_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/tests/mock
    )

    target_compile_definitions(pi_cemaker_test PRIVATE
        PICO_ON_DEVICE=0
        HOST_TEST=1
    )

    # Host interactive CLI runner
    add_executable(pi_cemaker_cli
        tests/sim_cli_runner.c
        src/usb_descriptors.c
        src/telemetry.c
        src/power_mgr.c
        src/cli.c
    )

    target_include_directories(pi_cemaker_cli PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/tests/mock
    )

    target_compile_definitions(pi_cemaker_cli PRIVATE
        PICO_ON_DEVICE=0
        HOST_TEST=1
    )
endif()
