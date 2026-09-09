cmake_minimum_required(VERSION 3.20)


include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)


project(STM32Test)


enable_language(C ASM)


add_executable(${PROJECT_NAME})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32f103x8_flash.ld"
    "${CMAKE_CURRENT_BINARY_DIR}/stm32f103x8_flash.ld"
    COPYONLY
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32f103x8_flash.ld"
)


include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)


target_sources(${PROJECT_NAME} PRIVATE

    Src/main.c
 
    Core/system_stm32f10x.c

    Startup/startup_stm32f10x_md.s

)


target_include_directories(${PROJECT_NAME} PRIVATE

    Core

)


target_compile_definitions(${PROJECT_NAME} PRIVATE

    STM32F10X_MD

)

# 生成 HEX 和 BIN 文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD

    COMMAND arm-none-eabi-objcopy
    -O ihex
    $<TARGET_FILE:${PROJECT_NAME}>
    ${PROJECT_NAME}.hex

    COMMAND arm-none-eabi-objcopy
    -O binary
    $<TARGET_FILE:${PROJECT_NAME}>
    ${PROJECT_NAME}.bin

    COMMENT "Generating HEX and BIN files..."
)