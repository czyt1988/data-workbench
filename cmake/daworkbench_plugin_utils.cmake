#
# DAWorkbench 插件构建支持（standalone 模式）
#
# 本文件随 DAWorkbench 安装到 lib/cmake/DAWorkbench/ 下，供插件脱离主工程独立构建时
# include。提供 da_plugin_bootstrap() 完成 standalone 引导：
#   1. project() 先行（确保 CMAKE_CXX_COMPILER_ID / CMAKE_SIZEOF_VOID_P 已定义，
#      否则安装目录名公式会出现空段，见历史 bug）
#   2. 计算 DAWorkbench 安装目录并设置 CMAKE_INSTALL_PREFIX / CMAKE_PREFIX_PATH /
#      DAWorkbench_DIR，使 find_package(DAWorkbench) 与第三方库查找均指向该目录
#   3. include DAWorkbench.cmake 与 daworkbench_3rdparty.cmake（提供 da_add_plugin /
#      da_link_3rdparty）
#
# 用法（插件 CMakeLists 顶部，代替原 27 行手写引导块）：
#   cmake_minimum_required(VERSION 3.16)
#   include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake")
#   da_plugin_bootstrap("MyPlugin" "My plugin description")
#   # 此后可用 da_add_plugin(...)
#
# 顶层构建时（作为主工程 add_subdirectory 的子目录）不需要调用本函数：
# 主工程根 CMakeLists 已 include 全部工具文件并设置好环境。
#

# 本文件自身所在目录（函数内 CMAKE_CURRENT_LIST_FILE 指向调用者目录，须在顶层捕获；
# 随 DAWorkbench 安装后与 DAWorkbench.cmake / daworkbench_3rdparty.cmake 同目录）
get_filename_component(_DA_PLUGIN_UTILS_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

# MSVC utf-8：DAWorkbench 安装的头文件含中文注释（无 BOM UTF-8），
# 缺此选项会被按本地编码解析导致语法错误（include 本文件即生效）
if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

function(da_plugin_bootstrap _plugin_name _plugin_description)
    project(${_plugin_name} LANGUAGES CXX DESCRIPTION ${_plugin_description})

    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

    # 平台判断（project() 之后 CMAKE_SIZEOF_VOID_P 才有值）
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(_platform_name "x86")
    else()
        set(_platform_name "x64")
    endif()

    # 构建类型：多配置生成器（Visual Studio）下 CMAKE_BUILD_TYPE 为空，
    # 用缓存变量 DA_INSTALL_CONFIG_TYPE（与主工程 dafun_set_bin_name 同一公式）
    if(CMAKE_CONFIGURATION_TYPES)
        set(_config_type "${DA_INSTALL_CONFIG_TYPE}" CACHE STRING
            "安装目录使用的构建类型（仅多配置生成器生效，如 Visual Studio）")
        if(NOT _config_type)
            set(_config_type "Release")
        endif()
    else()
        set(_config_type "${CMAKE_BUILD_TYPE}")
    endif()

    # DAWorkbench 安装目录：与主工程安装布局一致（bin_<Config>_qt<X>_<Compiler>_<Arch>），
    # 可通过 DAWorkbench_INSTALL_PATH 覆盖
    if(NOT DEFINED DAWorkbench_INSTALL_PATH)
        set(DAWorkbench_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../bin_${_config_type}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name}")
        message(STATUS "da_plugin_bootstrap: DAWorkbench_INSTALL_PATH not defined, set to ${DAWorkbench_INSTALL_PATH}")
    endif()
    set(DAWorkbench_INSTALL_DIR ${DAWorkbench_INSTALL_PATH})
    set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")
    set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})
    set(DA_INSTALL_LIB_CMAKE_PATH ${DAWorkbench_INSTALL_DIR}/lib/cmake)
    set(DA_INSTALL_LIB_SHARE_PATH ${DAWorkbench_INSTALL_DIR}/share/cmake)
    # 把安装目录加入 CMAKE_PREFIX_PATH，find_package(xxx CONFIG) 会自动在
    # ${DAWorkbench_INSTALL_DIR}/lib/cmake/<package> 查找
    list(APPEND CMAKE_PREFIX_PATH ${DAWorkbench_INSTALL_DIR})
    # tsl-ordered-map 的安装位置在 share/cmake，CMAKE_PREFIX_PATH 的 Config 模式
    # 搜索路径不覆盖该位置，因此需要显式设置
    set(tsl-ordered-map_DIR ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)

    # 本文件随 DAWorkbench 安装在 lib/cmake/DAWorkbench/ 下，同目录带有 DAWorkbench.cmake
    # 与 daworkbench_3rdparty.cmake（提供 da_add_plugin / da_link_3rdparty）
    include("${_DA_PLUGIN_UTILS_DIR}/DAWorkbench.cmake")
    include("${_DA_PLUGIN_UTILS_DIR}/daworkbench_3rdparty.cmake")

    # 先 find DAWorkbench（取得 DAWorkbench_VERSION 供 da_add_plugin 缺省版本），
    # 并创建全部导出目标
    find_package(DAWorkbench REQUIRED)

    # DAWorkbench 导出目标（DAWorkbenchTargets.cmake）的 INTERFACE 传递依赖了一批
    # Qt 组件与第三方库目标，standalone 构建必须先 find 它们，否则 set_target_properties
    # 报 "target was not found"。Qt5/Qt6 组件差异按大版本区分。
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
        Core Gui Widgets Xml Svg PrintSupport Concurrent OpenGL Network REQUIRED)
    if(WIN32)
        find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS AxContainer REQUIRED)
    endif()
    if(QT_VERSION_MAJOR EQUAL 6)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineCore WebEngineWidgets WebChannel REQUIRED)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core5Compat QUIET)
    else()
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngine WebEngineWidgets WebChannel REQUIRED)
    endif()
    if(Qt5_POSITION_INDEPENDENT_CODE)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    endif()
    # qwt::plot3d 的传递依赖需要 OpenGL::GLU
    find_package(OpenGL COMPONENTS OpenGL REQUIRED)
    # 第三方库（pybind11/ads/qwt/DAWidgets/quazip/spdlog 等）已在 CMAKE_PREFIX_PATH
    # 覆盖范围内，逐一预 find 以创建 IMPORTED 目标
    da_link_3rdparty_find_all()

    set(DA_PROJECT_NAME "DAWorkbench" PARENT_SCOPE)
    set(DA_VERSION "${DAWorkbench_VERSION}" PARENT_SCOPE)
    set(DAWorkbench_INSTALL_DIR ${DAWorkbench_INSTALL_DIR} PARENT_SCOPE)
    set(DAWorkbench_DIR "${DAWorkbench_DIR}" PARENT_SCOPE)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" PARENT_SCOPE)
    set(DA_INSTALL_LIB_CMAKE_PATH "${DA_INSTALL_LIB_CMAKE_PATH}" PARENT_SCOPE)
    set(DA_INSTALL_LIB_SHARE_PATH "${DA_INSTALL_LIB_SHARE_PATH}" PARENT_SCOPE)
    set(tsl-ordered-map_DIR "${tsl-ordered-map_DIR}" PARENT_SCOPE)
    # find_package(QT NAMES Qt6 Qt5) 的结果对后续 da_add_plugin 的
    # find_package(Qt${QT_VERSION_MAJOR} ...) 必需
    set(QT_VERSION_MAJOR "${QT_VERSION_MAJOR}" PARENT_SCOPE)
    set(QT_VERSION "${QT_VERSION}" PARENT_SCOPE)

    # C++17 与 Debug 后缀（原 damacro_plugin_setting 的职责）
    set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_DEBUG_POSTFIX "d")
    set(CMAKE_DEBUG_POSTFIX "d" PARENT_SCOPE)

    message(STATUS "da_plugin_bootstrap: ${_plugin_name} -> install prefix ${CMAKE_INSTALL_PREFIX}")
endfunction()
