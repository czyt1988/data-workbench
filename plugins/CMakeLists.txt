
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench Plugins 顶层Cmake文件
# 在运行次CMakeLists.txt之前，请确保DA-WorkBench已经进行了install，DA-WorkBench位于../CMakeLists.txt
# 请先单独编译第三方库后，安装，并把安装目录设置到和此安装目录一致

#3.16是qt6要求的最低版本
cmake_minimum_required(VERSION 3.16)

########################################################
# 版本定义
########################################################
set(DA_PLUGINS_VERSION_MAJOR 0)
set(DA_PLUGINS_VERSION_MINOR 0)
set(DA_PLUGINS_VERSION_PATCH 1)
set(DA_PLUGINS_VERSION "${DA_PLUGINS_VERSION_MAJOR}.${DA_PLUGINS_VERSION_MINOR}.${DA_PLUGINS_VERSION_PATCH}")
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    string(TIMESTAMP DA_COMPILE_DATETIME %y%m%d)
    string(TIMESTAMP DA_COMPILE_DATETIME_YEAR %y)
    string(TIMESTAMP DA_COMPILE_DATETIME_MONTH %m)
    string(TIMESTAMP DA_COMPILE_DATETIME_DAY %d)
    message(STATUS "DA Plugins Version is ${DA_PLUGINS_VERSION}")
endif()
########################################################
# 可选的变量
########################################################
# 此选项将自动调用Linguist工具对翻译文件进行翻译
option(DA_PLUGIN_ENABLE_AUTO_TRANSLATE
    "This option will automatically call the Linguist tool to translate the ts file"
    ON)
########################################################
# 定义工程
########################################################
project("DAWorkbench-Plugins"
        VERSION ${DA_PLUGINS_VERSION}
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench Plugins"
        )
# 获取默认安装文件夹

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/daworkbench_plugin_utils.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/daworkbench_utils.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/daworkbench_3rdparty.cmake)
set(DAWorkbench_INSTALL_FOLDER_NAME)
dafun_set_bin_name(DAWorkbench_INSTALL_FOLDER_NAME)

########################################################
# DA安装路径设置
########################################################
set(DA_MIN_QT_VERSION 5.14)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

    find_package(DAWorkbench QUIET)
    if(NOT DAWorkbench_FOUND)
        if(NOT DEFINED DAWorkbench_INSTALL_PATH)
            set(DAWorkbench_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../${DAWorkbench_INSTALL_FOLDER_NAME}")
            message(STATUS "NOT DEFINED DAWorkbench_INSTALL_PATH,set to ${DAWorkbench_INSTALL_PATH}")
        endif()
        set(DAWorkbench_DIR ${DAWorkbench_INSTALL_PATH}/lib/cmake/DAWorkbench)
    endif()
    set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_PATH})
    set(DA_INSTALL_LIB_CMAKE_PATH ${DAWorkbench_INSTALL_PATH}/lib/cmake)
    set(DA_INSTALL_LIB_SHARE_PATH ${DAWorkbench_INSTALL_PATH}/share/cmake)
    # 把安装目录加入 CMAKE_PREFIX_PATH,这样 find_package(xxx CONFIG) 会自动在
    # ${DAWorkbench_INSTALL_PATH}/lib/cmake/<package> 查找,无需为每个第三方库硬编码 xxx_DIR
    list(APPEND CMAKE_PREFIX_PATH ${DAWorkbench_INSTALL_PATH})
    ########################################################
    # 定义第三方库路径
    ########################################################
    # 注意: tsl-ordered-map 的安装位置在 share/cmake, CMAKE_PREFIX_PATH 的 Config 模式
    # 搜索路径不覆盖该位置,因此需要显式设置 tsl-ordered-map_DIR
    set(tsl-ordered-map_DIR ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)
    message(STATUS "tsl-ordered-map_DIR=${tsl-ordered-map_DIR}")
endif()

########################################################
# 子工程加载
########################################################
add_subdirectory(DataAnalysis)
add_subdirectory(DASystemNodes)
# DAAgentTools：平台内置 agent 工具插件（16 工具，plan-03 从 DAAgentModule 搬迁）。
# agent 工具依赖 Python（6 个工具经 DAPyDataFrame.h 用 pandas）。
add_subdirectory(DAAgentTools)
