cmake_minimum_required(VERSION 3.20)

# =========================================================
# STM32 CMake 工程配置
# MCU: STM32F103C8T6
# Compiler: arm-none-eabi-gcc
# Language: C + ASM
# =========================================================


# ---------------------------------------------------------
# 基础路径
# ---------------------------------------------------------

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})


# ---------------------------------------------------------
# 加载工具链和 CMake 配置
# ---------------------------------------------------------

# ARM GCC 编译器配置
include(${PROJECT_ROOT}/cmake/gcc-arm-none-eabi.cmake)

# 通用目标配置
include(${PROJECT_ROOT}/cmake/target.cmake)



# ---------------------------------------------------------
# 工程定义
# ---------------------------------------------------------

project(
    STM32Test
    LANGUAGES C ASM
)


# 开启 C 和汇编支持
enable_language(C ASM)



# ---------------------------------------------------------
# 创建目标文件
# ---------------------------------------------------------

add_executable(${PROJECT_NAME})



# ---------------------------------------------------------
# 链接脚本
# STM32 Flash 地址、RAM 地址等由此定义
# ---------------------------------------------------------

set(LINKER_SCRIPT
    ${PROJECT_ROOT}/stm32f103x8_flash.ld
)


# 复制 linker script 到 build 目录
configure_file(
    ${LINKER_SCRIPT}
    ${CMAKE_CURRENT_BINARY_DIR}/stm32f103x8_flash.ld
    COPYONLY
)


# 告诉 CMake：
# 如果 linker script 修改，需要重新链接
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES

    LINK_DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/stm32f103x8_flash.ld
)



# ---------------------------------------------------------
# 编译参数
# ---------------------------------------------------------

include(
    ${PROJECT_ROOT}/cmake/flags.cmake
)



# ---------------------------------------------------------
# 源文件
# ---------------------------------------------------------

target_sources(
    ${PROJECT_NAME} 

    PRIVATE

    # 用户代码
    Src/main.c


    # STM32系统初始化
    Core/system_stm32f10x.c


    # Cortex-M3启动文件
    # 包含 Reset_Handler、中断向量表等
    Startup/startup_stm32f10x_md.s
)



# ---------------------------------------------------------
# 头文件搜索路径
# ---------------------------------------------------------

target_include_directories(
    ${PROJECT_NAME}

    PRIVATE

    # CMSIS、系统头文件
    Core
)



# ---------------------------------------------------------
# MCU 宏定义
# ---------------------------------------------------------

target_compile_definitions(
    ${PROJECT_NAME}

    PRIVATE

    # STM32F103C8T6属于 Medium Density
    STM32F10X_MD
)



# ---------------------------------------------------------
# 生成 HEX / BIN 文件
# 烧录 STM32 时常用
# ---------------------------------------------------------

set(OUTPUT_DIR
    ${CMAKE_BINARY_DIR}/output
)


add_custom_command(

    TARGET ${PROJECT_NAME}

    POST_BUILD


    # 创建输出目录
    COMMAND ${CMAKE_COMMAND}
            -E make_directory
            ${OUTPUT_DIR}



    # ELF -> HEX
    COMMAND arm-none-eabi-objcopy

            -O ihex

            $<TARGET_FILE:${PROJECT_NAME}>

            ${OUTPUT_DIR}/${PROJECT_NAME}.hex



    # ELF -> BIN
    COMMAND arm-none-eabi-objcopy

            -O binary

            $<TARGET_FILE:${PROJECT_NAME}>

            ${OUTPUT_DIR}/${PROJECT_NAME}.bin



    COMMENT
    "Generating HEX and BIN files..."

)