
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 顶层Cmake文件
# 在运行次CMakeLists.txt之前，请确保3rdparty已经运行并进行了install，3rdparty位于/src/3rdparty/CMakeLists.txt
# 请先单独编译第三方库后，安装，并把安装目录设置到和此安装目录一致

#3.16是qt6要求的最低版本
cmake_minimum_required(VERSION 3.10)

########################################################
# 版本定义
########################################################
set(DA_VERSION_MAJOR 0)
set(DA_VERSION_MINOR 1)
set(DA_VERSION_PATCH 1)
set(DA_VERSION "${DA_VERSION_MAJOR}.${DA_VERSION_MINOR}.${DA_VERSION_PATCH}")
string(TIMESTAMP DA_COMPILE_DATETIME %y%m%d)
string(TIMESTAMP DA_COMPILE_DATETIME_YEAR %y)
string(TIMESTAMP DA_COMPILE_DATETIME_MONTH %m)
string(TIMESTAMP DA_COMPILE_DATETIME_DAY %d)
message(STATUS "DA Version is ${DA_VERSION}")
########################################################
# 定义工程
########################################################
set(DA_PROJECT_NAME "DAWorkbench")
# 这是所有DAWorkbench模块的总targets，所有模块都向这个target导出
set(DA_TARGET_NAME "DAWorkbenchTargets")

project(${DA_PROJECT_NAME}
        VERSION ${DA_VERSION}
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : A Workflow Integration Workbench"
        )
########################################################
# include
########################################################

include(${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_utils.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_3rdparty.cmake)
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
# 此选项将让da自动把结果安装到本地目录下
option(DA_AUTO_INSTALL_PREFIX
    "This option will allow DA to automatically install the results to the local directory"
    ON)
# 此选项将让da自动生成DAConfig.h文件，此选项仅库开发者需要设置为ON，库使用者默认应该为OFF
option(DA_AUTO_GENERATE_CONFIG_INFO
    "This option will allow DA to automatically generate DAConfig.h, This option only needs to be set to ON by library developers, and library users should default to OFF"
    OFF)
# 此选项将让da自动生成构建plugin
option(DA_BUILD_PLUGINS
    "This option will build plugin"
    ON)
# 此选项将让da运行测试
option(DA_ENABLE_TESTING
    "This option will enable testing"
    OFF)
if(DA_AUTO_INSTALL_PREFIX)
    message(STATUS "DA Auto Install")
    set(DA_BIN_DIR_NAME)
    dafun_set_bin_name(DA_BIN_DIR_NAME)
    set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/${DA_BIN_DIR_NAME}")
    set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR} CACHE PATH "Installation prefix" FORCE)
    message(STATUS "DA_BIN_DIR_NAME=${DA_BIN_DIR_NAME}")
    message(STATUS "DAWorkbench_INSTALL_DIR=${DAWorkbench_INSTALL_DIR}")
endif()



########################################################
# 一些常规设置
########################################################


set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

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

    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
    set(CMAKE_CXX_RESPONSE_FILE_PREFIX "@")
    set(CMAKE_CXX_RESPONSE_FILE_EXTENSION ".rsp")
endif()

if(CMAKE_GENERATOR STREQUAL "Ninja")
    # 针对Ninja生成器
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE "1")
endif()
########################################################
# MSVC设置
# 如果不添加会出现
# Qt5AxContainerd.lib(qaxbase.obj) : error LNK2038  MDd_DynamicDebug MTd_StaticDebug等提示
# Qt5AxBased.lib(qaxtypefunctions.obj) : error LNK2038:
# 可以通过下面方式设置
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
########################################################
if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MD>
        $<$<CONFIG:Debug>:/MDd>
        $<$<CONFIG:Release>:/MD>
    )
endif()
########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Svg
    Xml
    PrintSupport
    Concurrent
    OpenGL
    Test
    REQUIRED
)
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
        OpenGLWidgets
        Core5Compat
        REQUIRED
    )
endif()
if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
        AxContainer
        REQUIRED
    )
endif()
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
find_package(OpenGL COMPONENTS OpenGL REQUIRED)
########################################################
# 默认路径设置
########################################################
set(DA_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(DA_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(DA_INSTALL_LIB_CMAKE_PATH ${CMAKE_INSTALL_PREFIX}/lib/cmake)
set(DA_INSTALL_LIB_SHARE_PATH ${CMAKE_INSTALL_PREFIX}/share/cmake)
list(APPEND CMAKE_MODULE_PATH ${DA_CMAKE_DIR} ${DA_INSTALL_LIB_CMAKE_PATH})
# 把安装目录加入 CMAKE_PREFIX_PATH,这样 find_package(xxx CONFIG) 会自动在
# ${CMAKE_INSTALL_PREFIX}/lib/cmake/<package> 查找,无需为每个第三方库硬编码 xxx_DIR
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

########################################################
# 定义第三方库路径
########################################################
# 注意: tsl-ordered-map 的安装位置在 share/cmake, CMAKE_PREFIX_PATH 的 Config 模式
# 搜索路径不覆盖该位置,因此需要显式设置 tsl-ordered-map_DIR
set(tsl-ordered-map_DIR ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)
message(STATUS "tsl-ordered-map_DIR=${tsl-ordered-map_DIR}")


########################################################
# 添加预定义宏NOMINMAX,windows系统会定义max和min宏，这个宏和std::max冲突,因此在高版本的c++，应该取消minmax宏
########################################################
if(WIN32)
    ADD_COMPILE_DEFINITIONS(NOMINMAX)
endif()


########################################################
# 源码路径
########################################################
add_subdirectory(src)

# 插件
if(DA_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

# 测试
if(DA_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(src/tst)
endif()

##################################
# 安装 install
##################################
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${DA_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DAWorkbenchConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${DA_PROJECT_NAME}
)
#Unified export of all module targets
install(EXPORT ${DA_TARGET_NAME}
    FILE ${DA_TARGET_NAME}.cmake
    NAMESPACE ${DA_PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${DA_PROJECT_NAME}
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_PROJECT_NAME}ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_PROJECT_NAME}Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${DA_PROJECT_NAME}
)
#DAGlobal.h
install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/DAGlobals.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}
    COMPONENT headers
)
#DAConfigs.h
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/src/DAConfigs.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}
        COMPONENT headers
 )
if(DA_ENABLE_PYTHON)
    # 把PyScripts文件夹复制过去
    install(DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/src/PyScripts
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    # 为了能调试的时候运行，需要把src/PyScripts文件夹复制到${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/src/PyScripts" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
    message(STATUS "COPY ${CMAKE_CURRENT_LIST_DIR}/src/PyScripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
    if(DA_ENABLE_AUTO_INSTALL_PYTHON_ENV)
        # 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        if(${Python3_FOUND})
            if(WIN32)
                set(DA_PYTHON_DLL_PATH ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll)
                # 把dll复制到bin中否则无法运行
                file(COPY ${DA_PYTHON_DLL_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
                install(FILES
                        ${DA_PYTHON_DLL_PATH}
                        DESTINATION
                        ${CMAKE_INSTALL_BINDIR}
                 )
            endif()
        endif()
    endif()
endif()


install(FILES
    "${CMAKE_CURRENT_LIST_DIR}/cmake/daworkbench_plugin_utils.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/create_win32_resource_version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${DA_PROJECT_NAME}
)


##################################
# 翻译
##################################
if(DA_ENABLE_AUTO_TRANSLATE)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools REQUIRED)

    # 如果你有新的翻译需求，你需要在此添加新的ts文件，qt会自动生成新的翻译文件
    set(DA_TS_FILES
        ${CMAKE_CURRENT_LIST_DIR}/src/i18n/da_zh_CN.ts
        ${CMAKE_CURRENT_LIST_DIR}/src/i18n/da_en_US.ts
    )

    # 确保i18n目录存在
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/i18n)

    # 如果.ts文件不存在，创建有效的空占位符文件，避免首次构建时lrelease因找不到.ts文件而失败
    # 后续可通过 cmake --build <dir> --target update_translations 填充实际翻译内容
    foreach(_ts_file ${DA_TS_FILES})
        if(NOT EXISTS ${_ts_file})
            get_filename_component(_ts_name ${_ts_file} NAME_WE)
            # 从文件名推断语言代码，如 da_zh_CN -> zh_CN
            string(REGEX REPLACE "^da_" "" _lang_code ${_ts_name})
            string(REPLACE "_" "-" _lang_tag ${_lang_code})
            message(STATUS "Creating placeholder translation file: ${_ts_file}")
            file(WRITE ${_ts_file}
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<!DOCTYPE TS>\n\
<TS version=\"2.1\" language=\"${_lang_tag}\">\n\
</TS>\n")
        endif()
    endforeach()

    # 扫描的源码目录（按需增删）
    set(DA_LUPDATE_SOURCE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/APP
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAAgent
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAAxOfficeWrapper
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAData
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAFigure
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAGraphicsView
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAGui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAInterface
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAMessageHandler
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAPluginSupport
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAPyCommonWidgets
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAPyScripts
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAUtils
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DAPyWorkFlow
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DataAnalysis
    )

    # 统一的 lupdate/lrelease 翻译流程（兼容 Qt5.3+ 和 Qt6.2+）
    # - update_translations: 手动目标，运行 lupdate 从源码提取 tr() → .ts
    # - da_translations: ALL 目标，运行 lrelease 编译 .ts → .qm，并部署到运行时目录
    # .qm 文件显式依赖 .ts 文件，确保编辑 .ts 后 .qm 一定会重新生成

    # 确认 lrelease/lupdate 导入目标可用
    if(NOT TARGET Qt${QT_VERSION_MAJOR}::lrelease OR NOT TARGET Qt${QT_VERSION_MAJOR}::lupdate)
        message(FATAL_ERROR
            "Qt${QT_VERSION_MAJOR}::lrelease or Qt${QT_VERSION_MAJOR}::lupdate target not found. "
            "Ensure Qt Linguist tools are installed for Qt${QT_VERSION_MAJOR}.")
    endif()

    # 运行时翻译输出目录（构建时从 build 目录运行程序时加载 .qm 的位置）
    # VS 多配置生成器下 exe 在 bin/<config>/，translations 在 bin/translations/，
    # 运行时通过 <exe>/../translations 回退路径加载（见 DATranslatorManeger.cpp）
    set(DA_QM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/translations")
    file(MAKE_DIRECTORY "${DA_QM_OUTPUT_DIR}")

    # lrelease: 编译 .ts → .qm（显式依赖 .ts），同时部署到运行时翻译目录
    # 将 copy_if_different 内联到生成步骤中，确保 .qm 重新生成时一定会重新部署
    set(DA_QM_FILES)
    foreach(ts_file ${DA_TS_FILES})
        get_filename_component(ts_name ${ts_file} NAME_WE)
        set(qm_file "${CMAKE_CURRENT_BINARY_DIR}/${ts_name}.qm")
        list(APPEND DA_QM_FILES ${qm_file})

        add_custom_command(OUTPUT ${qm_file}
            COMMAND Qt${QT_VERSION_MAJOR}::lrelease ${ts_file} -qm ${qm_file}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${qm_file}" "${DA_QM_OUTPUT_DIR}/"
            DEPENDS ${ts_file}
            COMMENT "Compiling ${ts_name}.ts -> ${ts_name}.qm and deploying to ${DA_QM_OUTPUT_DIR}"
            VERBATIM
        )
    endforeach()

    # 手动触发的 lupdate 目标（不加入 ALL，避免每次构建重写 .ts）
    # 新增 tr() 字符串后运行: cmake --build <dir> --target update_translations
    add_custom_target(update_translations
        COMMAND Qt${QT_VERSION_MAJOR}::lupdate
            -locations relative -no-obsolete
            ${DA_LUPDATE_SOURCE_DIRS}
            -ts ${DA_TS_FILES}
        COMMENT "Updating translation source files (.ts) - run manually when new tr() strings are added"
        VERBATIM
    )

    # 自动构建的翻译目标（加入 ALL，编译 .ts → .qm 并部署到运行时目录）
    add_custom_target(da_translations ALL DEPENDS ${DA_QM_FILES})

    # 复制 3rdparty 安装目录中已有的 SARibbon .qm 文件到构建输出 translations/ 目录
    # 注：file(GLOB) 在配置时执行，3rdparty 重新编译后需重新配置 CMake 才能拾取新文件
    set(_3rdparty_tr_dir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/translations")
    if(EXISTS "${_3rdparty_tr_dir}")
        file(GLOB _saribbon_qm_files "${_3rdparty_tr_dir}/SARibbon_*.qm")
        foreach(_qm_file ${_saribbon_qm_files})
            add_custom_command(TARGET da_translations POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qm_file}" "${DA_QM_OUTPUT_DIR}/"
                COMMENT "Copying 3rdparty translation ${_qm_file}"
            )
        endforeach()
    endif()

    # 将翻译目标绑定到 App，确保构建 App 时翻译也会生成
    add_dependencies(${DA_APP_NAME} da_translations)

    message(STATUS "Translation enabled (unified lupdate/lrelease API)")
    message(STATUS "  QM files: ${DA_QM_FILES}")
    message(STATUS "  To update .ts files: cmake --build <dir> --target update_translations")

    install(FILES ${DA_QM_FILES}
                DESTINATION ${CMAKE_INSTALL_BINDIR}/translations
                COMPONENT translations)
endif()
