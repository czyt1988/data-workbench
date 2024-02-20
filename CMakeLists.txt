
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 顶层Cmake文件
# 在运行次CMakeLists.txt之前，请确保3rdparty已经运行并进行了install，3rdparty位于/src/3rdparty/CMakeLists.txt
# 请先单独编译第三方库后，安装，并把安装目录设置到和此安装目录一致

#3.16是qt6要求的最低版本
cmake_minimum_required(VERSION 3.16)

########################################################
# 版本定义
########################################################
set(DA_VERSION_MAJOR 0)
set(DA_VERSION_MINOR 0)
set(DA_VERSION_PATCH 2)
set(DA_VERSION "${DA_VERSION_MAJOR}.${DA_VERSION_MINOR}.${DA_VERSION_PATCH}")
string(TIMESTAMP DA_COMPILE_DATETIME %y%m%d)
string(TIMESTAMP DA_COMPILE_DATETIME_YEAR %y)
string(TIMESTAMP DA_COMPILE_DATETIME_MONTH %m)
string(TIMESTAMP DA_COMPILE_DATETIME_DAY %d)
message(STATUS "DA Version is ${DA_VERSION}")
########################################################
# 可选的变量
########################################################
# 此选项将设置是否开启python
option(DA_ENABLE_PYTHON
    "enable python|if ON will add DAPythonBind DAPyScript DAPyCommonWidgets COMPONENTS"
    ON)
option(DA_ENABLE_AUTO_INSTALL_PYTHON_ENV
    "This parameter allows cmake to automatically search for the Python environment and copy the necessary DLLs from the Python environment to the bin directory without manual deployment. It is recommended to enable this option for Windows users who have not set the Python directory as a system environment variable"
    ON)
# 此选项将自动调用Linguist工具对翻译文件进行翻译
option(DA_ENABLE_AUTO_TRANSLATE
    "This option will automatically call the Linguist tool to translate the ts file"
    ON)
########################################################
# 定义工程
########################################################
set(DA_PROJECT_NAME "DAWorkbench")

project(${DA_PROJECT_NAME} 
        VERSION ${DA_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : A Workflow Integration Workbench"
        )


########################################################
# 一些常规设置
########################################################


set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# c++标准，要求为17
set(CMAKE_CXX_STANDARD 17)
# 强制要求
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)
########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # make sure __cplusplus is defined when using msvc and enable parallel build
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()


set(DA_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${DA_PLATFORM_NAME})

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

########################################################
# 平台判断
########################################################
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(DA_PLATFORM_NAME "x86")
else()
    set(DA_PLATFORM_NAME "x64")
endif()
########################################################
# 安装路径设置
########################################################
set(DA_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${DA_PLATFORM_NAME})
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/${DA_BIN_DIR_NAME}")

########################################################
# 默认路径设置
########################################################
set(DA_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(DA_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(DA_INSTALL_LIB_CMAKE_PATH ${CMAKE_INSTALL_PREFIX}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${DA_CMAKE_DIR} ${DA_INSTALL_LIB_CMAKE_PATH})

########################################################
# 定义第三方库路径
########################################################
set(SARibbonBar_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
set(DALiteCtk_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/DALiteCtk)
set(qwt_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qwt)
set(DAWorkbench_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/${DA_PROJECT_NAME})
set(qt${QT_VERSION_MAJOR}advanceddocking_DIR  ${DA_INSTALL_LIB_CMAKE_PATH}/qt${QT_VERSION_MAJOR}advanceddocking)

##################################
# 初步安装 install 通用内容
##################################
message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "${DA_PROJECT_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${DA_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${DA_PROJECT_NAME}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    DESTINATION lib/cmake/${DA_PROJECT_NAME}
)
########################################################
# 引入用到的通用函数和宏
########################################################
include(${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_utils.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_3rdparty.cmake)


########################################################
# 自动化生成配置头文件
########################################################
configure_file (
  "${CMAKE_CURRENT_LIST_DIR}/src/DAConfigs.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/src/DAConfigs.h"
)
#DAGlobal.h
install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/DAGlobals.h
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)
#DAConfigs.h
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/src/DAConfigs.h
        DESTINATION include/${DA_PROJECT_NAME}
        COMPONENT headers
 )
########################################################
# 添加预定义宏NOMINMAX,windows系统会定义max和min宏，这个宏和std::max冲突
########################################################
if(WIN32)
    ADD_COMPILE_DEFINITIONS(NOMINMAX)
endif()


########################################################
# 源码路径
########################################################
add_subdirectory(src)


##################################
# 最终安装 install
##################################
if(DA_ENABLE_PYTHON)
    # 把PyScripts文件夹复制过去
    install(DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/src/PyScripts
        DESTINATION bin
    )
    # 为了能调试的时候运行，需要把src/PyScripts文件夹复制到${CMAKE_CURRENT_BINARY_DIR}/bin
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/src/PyScripts" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)
    if(DA_ENABLE_AUTO_INSTALL_PYTHON_ENV)
        # 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        if(${Python3_FOUND})
            if(WIN32)
                set(DA_PYTHON_DLL_PATH ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll)
                # 把dll复制到bin中否则无法运行
                file(COPY ${DA_PYTHON_DLL_PATH} DESTINATION ${CMAKE_BINARY_DIR}/bin)
                install(FILES
                        ${DA_PYTHON_DLL_PATH}
                        DESTINATION
                        bin
                 )
            endif()
        endif()
    endif()
endif()


install(FILES
    "${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_plugin_utils.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/create_win32_resource_version.cmake"
    DESTINATION lib/cmake/${DA_PROJECT_NAME}
)

##################################
# 翻译
##################################
if(DA_ENABLE_AUTO_TRANSLATE)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools QUIET)
    set(DA_TS_FILES
        ${CMAKE_CURRENT_LIST_DIR}/src/da_zh_CN.ts
        ${CMAKE_CURRENT_LIST_DIR}/src/da_en_US.ts
    )
    set(DA_BUILD_TRANSLATIONS_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin/translations)
    # 更新翻译
    if(COMMAND qt${QT_VERSION_MAJOR}_create_translation)
        # 让qm文件直接生成到DA_BUILD_TRANSLATIONS_DIR下
        set_source_files_properties(${DA_TS_FILES} PROPERTIES OUTPUT_LOCATION ${DA_BUILD_TRANSLATIONS_DIR})

        if("${QT_VERSION_MAJOR}" STREQUAL "5")
            qt5_create_translation(DA_QM_FILES "${CMAKE_CURRENT_LIST_DIR}/src" ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
        else()
            qt6_create_translation(DA_QM_FILES "${CMAKE_CURRENT_LIST_DIR}/src" ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
        endif()
        # 添加更新翻译的目标
        add_custom_target(da_update_translations DEPENDS ${DA_TS_FILES})

        # 添加发布翻译的目标
        add_custom_target(da_release_translations DEPENDS ${DA_QM_FILES})

        # 安装翻译 - 在install上使用
        install(DIRECTORY ${DA_BUILD_TRANSLATIONS_DIR} DESTINATION bin)
    endif()
endif()
