cmake_minimum_required(VERSION 3.13)

set(projectName "blink")


include(pico_sdk_import.cmake)

project(${projectName} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#set(PICO_BARE_METAL 1)

pico_sdk_init()


add_executable(${projectName} ${projectName}.c)

pico_add_extra_outputs(${projectName})

set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "-Wall")

pico_set_linker_script(${projectName} ${CMAKE_CURRENT_SOURCE_DIR}/noboot2.ld)

target_link_libraries(${projectName} pico_stdlib_headers pico_stdlib hardware_watchdog hardware_structs)

pico_enable_stdio_usb(${projectName} 1)
pico_enable_stdio_uart(${projectName} 0)

