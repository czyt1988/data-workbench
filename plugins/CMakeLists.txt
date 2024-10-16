
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench Plugins 顶层Cmake文件
# 在运行次CMakeLists.txt之前，请确保DA-WorkBench已经进行了install，DA-WorkBench位于../CMakeLists.txt
# 请先单独编译第三方库后，安装，并把安装目录设置到和此安装目录一致

#3.16是qt6要求的最低版本
cmake_minimum_required(VERSION 3.12)

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
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(DAWorkbench QUIET)
if(NOT DAWorkbench_FOUND)
    if(NOT DEFINED DAWorkbench_INSTALL_PATH)
        set(DAWorkbench_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../${DAWorkbench_INSTALL_FOLDER_NAME}")
        message(STATUS "NOT DEFINED DAWorkbench_INSTALL_PATH,set to ${DAWorkbench_INSTALL_PATH}")
    endif()
    set(DAWorkbench_DIR ${DAWorkbench_INSTALL_PATH}/lib/cmake/DAWorkbench)
endif()
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(DA_ENABLE_PYTHON ON)
endif()


set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_PATH})

########################################################
# 子工程加载
########################################################
if(DA_ENABLE_PYTHON)
    add_subdirectory(Base)
    add_subdirectory(DataAnalysis)
endif()


##################################
# 翻译
##################################

# find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools QUIET)

# # 命名规则一定要按照qt的命名规则，否则无法找到xx_zh_CN或者xxx_en_US
# set(DA_TS_FILES
#     "${CMAKE_CURRENT_LIST_DIR}/da_plugins_zh_CN.ts"
#     "${CMAKE_CURRENT_LIST_DIR}/da_plugins_en_US.ts"
# )
# # DA 在 初始化的时候会查找translations和translations_user两个目录的翻译，插件的翻译建议在translations_user中
# set(DA_BUILD_TRANSLATIONS_DIR ${DAWorkbench_INSTALL_DIR}/bin/translations_user)

# # 让qm文件直接生成到DA_BUILD_TRANSLATIONS_DIR下
# set_source_files_properties(${DA_TS_FILES} PROPERTIES OUTPUT_LOCATION ${DA_BUILD_TRANSLATIONS_DIR})

# if("${QT_VERSION_MAJOR}" STREQUAL "5")
#     qt5_create_translation(DA_QM_FILES ${CMAKE_CURRENT_LIST_DIR} ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
# else()
#     qt6_create_translation(DA_QM_FILES ${CMAKE_CURRENT_LIST_DIR} ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
# endif()
# # 添加更新翻译的目标
# add_custom_target(daplugins_update_translations DEPENDS ${DA_TS_FILES})

# # 添加发布翻译的目标
# add_custom_target(daplugins_release_translations DEPENDS ${DA_QM_FILES})

# # 安装翻译 - 在install上使用
# set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})
# install(DIRECTORY ${DA_BUILD_TRANSLATIONS_DIR} DESTINATION bin)
