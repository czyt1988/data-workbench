#
# DAWorkbench 声明式构建 API
#
# 一个函数调用完成模块的 target 创建、Qt 组件查找链接、DA 模块链接、第三方库导入、
# Windows 版本资源生成、安装与导出、头文件安装。取代原 damacro_lib_setting /
# damacro_set_lib_properties / damacro_lib_install / damacro_app_setting / damacro_app_install /
# damacro_plugin_setting / damacro_plugin_install / damacro_setup_test 三段式宏调用。
#
# 设计原则：
# - 全部使用 function（真实变量作用域），杜绝 macro 文本替换语义
# - 所有数据经参数传入，函数内部不依赖调用者预先设置的 DA_LIB_* 隐式变量
# - 不在子目录调用 project()（子目录 project() 会重置 PROJECT_NAME/PROJECT_VERSION）
# - 全局设置（C++17 / DEBUG_POSTFIX / /utf-8 / /MP 等）只在根 CMakeLists.txt 设置一次
#
# 依赖的根 CMakeLists.txt 全局变量：
#   DA_PROJECT_NAME  导出命名空间（DAWorkbench::）
#   DA_TARGET_NAME   导出集名（DAWorkbenchTargets）
#   DA_VERSION       项目统一版本号（模块未显式指定 VERSION 时的默认值）
#   DA_CMAKE_DIR     cmake 工具目录（定位 create_win32_resource_version.cmake）
#   DA_MIN_QT_VERSION 最低 Qt 版本（默认 5.14）
#

# ===========================================================================
# da_add_library —— DA 库模块一站式声明
#
# da_add_library(
#     NAME <name>                       # 模块名（target 名）
#     BUILD_DEFINE <define>             # 构建 DLL 时的私有宏（C++ 源码 #ifdef 依赖，
#                                       #   如 DAUTILS_BUILD / DAGUI_BUILDLIB，必须逐字一致）
#     [VERSION <x.y.z>]                 # 默认取根 DA_VERSION（版本统一）
#     [TYPE <SHARED|INTERFACE>]         # 默认 SHARED；INTERFACE 用于纯头文件模块（DAShared）
#     SOURCES <src...>                  # 源文件（模块自行 GLOB 或显式清单后传入）
#     [QT_PUBLIC <comp...>]             # find_package + PUBLIC 链接 Qt${QT_VERSION_MAJOR}::comp
#     [QT_PRIVATE <comp...>]
#     [LINK_PUBLIC <mod...>]            # PUBLIC 链接 DAWorkbench::<mod>
#     [LINK_PRIVATE <mod...>]
#     [THIRDPARTY <lib...>]             # 经 da_link_3rdparty 导入（SARibbonBar/qwt/...）
#     [THIRDPARTY_PUBLIC <lib...>]      # 同上，但 PUBLIC 链接（下游需要其头文件路径）
#     [HEADERS <files...>]              # 显式头文件清单（安装到 include/DAWorkbench/<name>）
#     [HEADERS_DIRS <dir...>]           # 相对子目录列表（'.'=模块根），GLOB 收集 *.h/*.hpp 安装
#     [PUBLIC_SUBDIRS <dir...>]         # 同 HEADERS_DIRS，且额外生成 PUBLIC include 路径
#                                       #   （ui 窗口提升场景，如 DAGui/Dialog）
#     [COMPILE_DEFINITIONS_PRIVATE <def...>]
#     [COMPILE_OPTIONS_PRIVATE <opt...>]
#     [WIN32_LINK_PRIVATE <lib...>]     # 仅 WIN32 下私有链接系统库（Crypt32 等）
#     [NO_RC]                           # 不生成 Windows dll 版本资源
#     [NO_EXPORT]                       # 不加入 DAWorkbenchTargets 导出集
#     [NO_INSTALL_HEADERS]              # 不安装头文件
# )
# ===========================================================================
function(da_add_library)
    set(_opts NO_RC NO_EXPORT NO_INSTALL_HEADERS)
    set(_one NAME VERSION TYPE BUILD_DEFINE DESCRIPTION)
    set(_multi SOURCES QT_PUBLIC QT_PRIVATE LINK_PUBLIC LINK_PRIVATE THIRDPARTY THIRDPARTY_PUBLIC
        HEADERS HEADERS_DIRS PUBLIC_SUBDIRS COMPILE_DEFINITIONS_PRIVATE COMPILE_OPTIONS_PRIVATE
        WIN32_LINK_PRIVATE)
    cmake_parse_arguments(DA_AL "${_opts}" "${_one}" "${_multi}" ${ARGN})

    if(NOT DA_AL_NAME)
        message(FATAL_ERROR "da_add_library: NAME is required")
    endif()
    if(NOT DA_AL_VERSION)
        set(DA_AL_VERSION ${DA_VERSION})
    endif()
    if(NOT DA_AL_TYPE)
        set(DA_AL_TYPE SHARED)
    endif()
    if(NOT DA_MIN_QT_VERSION)
        set(DA_MIN_QT_VERSION 5.14)
    endif()

    set(_name ${DA_AL_NAME})

    # Qt 组件统一查找（PUBLIC/PRIVATE 去重后一次 find_package）
    if(DA_AL_QT_PUBLIC OR DA_AL_QT_PRIVATE)
        set(_qt_comps ${DA_AL_QT_PUBLIC} ${DA_AL_QT_PRIVATE})
        list(REMOVE_DUPLICATES _qt_comps)
        find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS ${_qt_comps} REQUIRED)
    endif()

    if(DA_AL_TYPE STREQUAL "INTERFACE")
        add_library(${_name} INTERFACE ${DA_AL_SOURCES})
        target_include_directories(${_name} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${_name}>
        )
        foreach(_m ${DA_AL_QT_PUBLIC})
            target_link_libraries(${_name} INTERFACE Qt${QT_VERSION_MAJOR}::${_m})
        endforeach()
        foreach(_m ${DA_AL_LINK_PUBLIC})
            target_link_libraries(${_name} INTERFACE ${DA_PROJECT_NAME}::${_m})
        endforeach()
    else()
        if(NOT DA_AL_BUILD_DEFINE)
            message(FATAL_ERROR "da_add_library(${_name}): BUILD_DEFINE is required for non-INTERFACE library")
        endif()
        add_library(${_name} SHARED ${DA_AL_SOURCES})
        # 别名让 DAWorkbench::<name> 在构建树内可用（与导出的安装目标同名）
        add_library(${DA_PROJECT_NAME}::${_name} ALIAS ${_name})
        target_compile_definitions(${_name} PRIVATE ${DA_AL_BUILD_DEFINE})
        set_target_properties(${_name} PROPERTIES
            AUTOMOC ON
            AUTOUIC ON
            AUTORCC ON
            CXX_EXTENSIONS OFF
            DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
            VERSION ${DA_AL_VERSION}
            EXPORT_NAME ${_name}
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
        foreach(_m ${DA_AL_QT_PUBLIC})
            target_link_libraries(${_name} PUBLIC Qt${QT_VERSION_MAJOR}::${_m})
        endforeach()
        foreach(_m ${DA_AL_QT_PRIVATE})
            target_link_libraries(${_name} PRIVATE Qt${QT_VERSION_MAJOR}::${_m})
        endforeach()
        foreach(_m ${DA_AL_LINK_PUBLIC})
            target_link_libraries(${_name} PUBLIC ${DA_PROJECT_NAME}::${_m})
        endforeach()
        foreach(_m ${DA_AL_LINK_PRIVATE})
            target_link_libraries(${_name} PRIVATE ${DA_PROJECT_NAME}::${_m})
        endforeach()
        if(DA_AL_COMPILE_DEFINITIONS_PRIVATE)
            target_compile_definitions(${_name} PRIVATE ${DA_AL_COMPILE_DEFINITIONS_PRIVATE})
        endif()
        if(DA_AL_COMPILE_OPTIONS_PRIVATE)
            target_compile_options(${_name} PRIVATE ${DA_AL_COMPILE_OPTIONS_PRIVATE})
        endif()
        if(WIN32 AND DA_AL_WIN32_LINK_PRIVATE)
            target_link_libraries(${_name} PRIVATE ${DA_AL_WIN32_LINK_PRIVATE})
        endif()
        if(DA_AL_THIRDPARTY)
            da_link_3rdparty(${_name} LIBS ${DA_AL_THIRDPARTY})
        endif()
        if(DA_AL_THIRDPARTY_PUBLIC)
            da_link_3rdparty(${_name} LIBS ${DA_AL_THIRDPARTY_PUBLIC} SCOPE PUBLIC)
        endif()

        # Qt 日志带上代码位置（原 damacro_lib_install_no_rc 行为）
        target_compile_definitions(${_name} PRIVATE QT_MESSAGELOGCONTEXT)
        # 模块自身 + DAGlobals.h 所在 src 根 + DAShared 三组 include 路径
        target_include_directories(${_name} PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${_name}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        )
        target_include_directories(${_name} PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>
        )
        target_include_directories(${_name} PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/DAShared>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../DAShared>
        )
        # ui 窗口提升等场景需要的子目录 PUBLIC include
        foreach(_sub ${DA_AL_PUBLIC_SUBDIRS})
            target_include_directories(${_name} PUBLIC
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${_name}/${_sub}>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/${_sub}>
            )
        endforeach()

        # Windows dll 版本资源
        if(WIN32 AND NOT DA_AL_NO_RC)
            include("${DA_CMAKE_DIR}/create_win32_resource_version.cmake")
            create_win32_resource_version(
                TARGET ${_name}
                FILENAME ${_name}
                VERSION ${DA_AL_VERSION}
                EXT "dll"
                DESCRIPTION ${DA_AL_DESCRIPTION}
            )
        endif()

        include(GNUInstallDirs)
        if(NOT DA_AL_NO_EXPORT)
            install(TARGETS ${_name}
                EXPORT ${DA_TARGET_NAME}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endif()

    # 头文件安装：HEADERS_DIRS / PUBLIC_SUBDIRS 按目录 GLOB，HEADERS 显式清单
    if(NOT DA_AL_NO_INSTALL_HEADERS)
        foreach(_sub ${DA_AL_HEADERS_DIRS} ${DA_AL_PUBLIC_SUBDIRS})
            if(_sub STREQUAL ".")
                set(_src_dir "${CMAKE_CURRENT_LIST_DIR}")
                set(_dest_sub "")
            else()
                set(_src_dir "${CMAKE_CURRENT_LIST_DIR}/${_sub}")
                set(_dest_sub "/${_sub}")
            endif()
            file(GLOB _hdrs "${_src_dir}/*.h" "${_src_dir}/*.hpp")
            if(_hdrs)
                install(FILES ${_hdrs}
                    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${_name}${_dest_sub}
                    COMPONENT headers
                )
            endif()
        endforeach()
        if(DA_AL_HEADERS)
            install(FILES ${DA_AL_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${_name}
                COMPONENT headers
            )
        endif()
    endif()

    message(STATUS "${DA_PROJECT_NAME}::${_name} ${DA_AL_VERSION} (${DA_AL_TYPE})")
endfunction()

# ===========================================================================
# da_add_executable —— 主程序一站式声明（合并原 damacro_app_setting /
# damacro_set_app_properties / damacro_app_install 及 APP 手写块）
#
# da_add_executable(
#     NAME <name>
#     SOURCES <src...>
#     [VERSION <x.y.z>]                 # 默认取根 DA_VERSION
#     [QT_PUBLIC <comp...>]
#     [QT_WIN32_PUBLIC <comp...>]       # 仅 WIN32 下链接（AxContainer）
#     [LINK_PUBLIC <mod...>]            # DAWorkbench::<mod>
#     [LINK_PRIVATE <mod...>]
#     [THIRDPARTY <lib...>]
#     [COMPILE_DEFINITIONS_PUBLIC <def...>]
#     [COMPILE_DEFINITIONS_PRIVATE <def...>]
#     [WIN32_LINK_PRIVATE <lib...>]     # 仅 WIN32 下私有链接（Dbghelp 等）
#     [ICON <file>]                     # exe 图标（生成 rc 版本资源）
#     [COMPANY_NAME <s>] [COPYRIGHT <s>] [DESCRIPTION <s>]
#     [DEPLOY_3RDPARTY_DLLS]            # 构建后复制第三方 DLL 到输出目录
#     [PLUGIN_DIR <dir>]                # 配合 DEPLOY_3RDPARTY_DLLS，同时复制到插件目录
#     [CONSOLE_ON_DEBUG]                # Debug 用控制台子系统、Release 用窗口子系统
# )
# ===========================================================================
function(da_add_executable)
    set(_opts DEPLOY_3RDPARTY_DLLS CONSOLE_ON_DEBUG)
    set(_one NAME VERSION ICON COMPANY_NAME COPYRIGHT DESCRIPTION PLUGIN_DIR)
    set(_multi SOURCES QT_PUBLIC QT_WIN32_PUBLIC LINK_PUBLIC LINK_PRIVATE THIRDPARTY
        COMPILE_DEFINITIONS_PUBLIC COMPILE_DEFINITIONS_PRIVATE WIN32_LINK_PRIVATE)
    cmake_parse_arguments(DA_AE "${_opts}" "${_one}" "${_multi}" ${ARGN})

    if(NOT DA_AE_NAME)
        message(FATAL_ERROR "da_add_executable: NAME is required")
    endif()
    if(NOT DA_AE_VERSION)
        set(DA_AE_VERSION ${DA_VERSION})
    endif()
    if(NOT DA_MIN_QT_VERSION)
        set(DA_MIN_QT_VERSION 5.14)
    endif()

    set(_name ${DA_AE_NAME})

    if(DA_AE_QT_PUBLIC OR DA_AE_QT_WIN32_PUBLIC)
        set(_qt_comps ${DA_AE_QT_PUBLIC})
        if(WIN32)
            list(APPEND _qt_comps ${DA_AE_QT_WIN32_PUBLIC})
        endif()
        list(REMOVE_DUPLICATES _qt_comps)
        find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS ${_qt_comps} REQUIRED)
    endif()

    add_executable(${_name} ${DA_AE_SOURCES})

    # Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1
    set(_bundle_id_props "")
    if(QT_VERSION VERSION_LESS 6.1.0)
        set(_bundle_id_props MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${_name})
    endif()
    set_target_properties(${_name} PROPERTIES
        ${_bundle_id_props}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${DA_AE_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${DA_AE_VERSION}
        WIN32_EXECUTABLE TRUE
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        VERSION ${DA_AE_VERSION}
        EXPORT_NAME ${_name}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    if(MSVC)
        target_compile_options(${_name} PRIVATE "/utf-8")
        target_compile_definitions(${_name} PRIVATE "_UNICODE" "UNICODE")
        # 禁止生成清单文件：某些操作系统（杀毒软件）下会遇到
        # general error c101008d: Failed to write the updated manifest
        target_link_options(${_name} PRIVATE "/MANIFEST:NO")
    endif()

    foreach(_m ${DA_AE_QT_PUBLIC})
        target_link_libraries(${_name} PUBLIC Qt${QT_VERSION_MAJOR}::${_m})
    endforeach()
    if(WIN32)
        foreach(_m ${DA_AE_QT_WIN32_PUBLIC})
            target_link_libraries(${_name} PUBLIC Qt${QT_VERSION_MAJOR}::${_m})
        endforeach()
    endif()
    foreach(_m ${DA_AE_LINK_PUBLIC})
        target_link_libraries(${_name} PUBLIC ${DA_PROJECT_NAME}::${_m})
    endforeach()
    foreach(_m ${DA_AE_LINK_PRIVATE})
        target_link_libraries(${_name} PRIVATE ${DA_PROJECT_NAME}::${_m})
    endforeach()
    if(WIN32)
        foreach(_m ${DA_AE_WIN32_LINK_PRIVATE})
            target_link_libraries(${_name} PRIVATE ${_m})
        endforeach()
    endif()
    if(DA_AE_COMPILE_DEFINITIONS_PUBLIC)
        target_compile_definitions(${_name} PUBLIC ${DA_AE_COMPILE_DEFINITIONS_PUBLIC})
    endif()
    if(DA_AE_COMPILE_DEFINITIONS_PRIVATE)
        target_compile_definitions(${_name} PRIVATE ${DA_AE_COMPILE_DEFINITIONS_PRIVATE})
    endif()
    if(DA_AE_THIRDPARTY)
        da_link_3rdparty(${_name} LIBS ${DA_AE_THIRDPARTY})
    endif()

    if(WIN32 AND DA_AE_CONSOLE_ON_DEBUG)
        # Debug 用控制台子系统便于看输出，Release 用窗口子系统（无控制台窗口）
        target_link_options(${_name} PRIVATE
            "$<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>"
            "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
        )
    endif()

    include(GNUInstallDirs)
    install(TARGETS ${_name}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if(QT_VERSION_MAJOR EQUAL 6)
        qt_finalize_executable(${_name})
    endif()

    # Windows exe 版本资源（图标 + 版本信息）
    if(WIN32)
        include("${DA_CMAKE_DIR}/create_win32_resource_version.cmake")
        create_win32_resource_version(
            TARGET ${_name}
            VERSION ${DA_AE_VERSION}
            COMPANY_NAME "${DA_AE_COMPANY_NAME}"
            COPYRIGHT "${DA_AE_COPYRIGHT}"
            DESCRIPTION "${DA_AE_DESCRIPTION}"
            ICONS "${DA_AE_ICON}"
            FILE_EXTENSION exe
        )
    endif()

    if(DA_AE_DEPLOY_3RDPARTY_DLLS)
        if(DA_AE_PLUGIN_DIR)
            dafun_deploy_3rdparty_dlls(${_name} PLUGIN_DIR "${DA_AE_PLUGIN_DIR}")
        else()
            dafun_deploy_3rdparty_dlls(${_name})
        endif()
    endif()

    message(STATUS "${_name} ${DA_AE_VERSION} (executable)")
endfunction()

# ===========================================================================
# da_add_plugin —— 插件一站式声明（合并原 damacro_plugin_setting /
# damacro_plugin_install 及各插件手写属性块）
#
# 使用前插件 CMakeLists 顶部需自行完成引导（cmake_minimum_required / project /
# 计算 DAWorkbench_INSTALL_DIR 并设置 CMAKE_INSTALL_PREFIX / include 工具文件）。
# 顶层构建时（CMAKE_SOURCE_DIR != 项目自身目录）由顶层已设置好相关环境。
#
# da_add_plugin(
#     NAME <name>
#     BUILD_DEFINE <define>             # 如 DATAANALYSIS_PLUGIN_BUILD
#     SOURCES <src...>
#     [VERSION <x.y.z>]                 # 默认取根 DA_VERSION
#     [QT_PUBLIC <comp...>] [QT_PRIVATE <comp...>]
#     [QT_WIN32_PUBLIC <comp...>]
#     [LINK_PUBLIC <mod...>] [LINK_PRIVATE <mod...>]   # DAWorkbench::<mod>
#     [THIRDPARTY <lib...>]             # 自动以 INSTALL_DIR=DAWorkbench_INSTALL_DIR 导入
#     [INCLUDE_PRIVATE <dir...>]        # 私有 include 目录（绝对或相对路径）
# )
# 输出：构建树 lib/plugins 与 bin/plugins，安装到 bin/plugins（不进导出集）
# ===========================================================================
function(da_add_plugin)
    set(_opts "")
    set(_one NAME VERSION BUILD_DEFINE)
    set(_multi SOURCES QT_PUBLIC QT_PRIVATE QT_WIN32_PUBLIC LINK_PUBLIC LINK_PRIVATE
        THIRDPARTY INCLUDE_PRIVATE COMPILE_DEFINITIONS_PRIVATE)
    cmake_parse_arguments(DA_AP "${_opts}" "${_one}" "${_multi}" ${ARGN})

    if(NOT DA_AP_NAME)
        message(FATAL_ERROR "da_add_plugin: NAME is required")
    endif()
    if(NOT DA_AP_BUILD_DEFINE)
        message(FATAL_ERROR "da_add_plugin(${DA_AP_NAME}): BUILD_DEFINE is required")
    endif()
    if(NOT DA_AP_VERSION)
        set(DA_AP_VERSION ${DA_VERSION})
    endif()
    if(NOT DA_MIN_QT_VERSION)
        set(DA_MIN_QT_VERSION 5.14)
    endif()

    set(_name ${DA_AP_NAME})

    if(DA_AP_QT_PUBLIC OR DA_AP_QT_PRIVATE OR DA_AP_QT_WIN32_PUBLIC)
        set(_qt_comps ${DA_AP_QT_PUBLIC} ${DA_AP_QT_PRIVATE})
        if(WIN32)
            list(APPEND _qt_comps ${DA_AP_QT_WIN32_PUBLIC})
        endif()
        if(_qt_comps)
            list(REMOVE_DUPLICATES _qt_comps)
            find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS ${_qt_comps} REQUIRED)
        endif()
    endif()

    add_library(${_name} SHARED ${DA_AP_SOURCES})
    target_compile_definitions(${_name} PRIVATE ${DA_AP_BUILD_DEFINE})
    set_target_properties(${_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        VERSION ${DA_AP_VERSION}
        EXPORT_NAME ${_name}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/plugins"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins"
    )

    foreach(_m ${DA_AP_QT_PUBLIC})
        target_link_libraries(${_name} PUBLIC Qt${QT_VERSION_MAJOR}::${_m})
    endforeach()
    foreach(_m ${DA_AP_QT_PRIVATE})
        target_link_libraries(${_name} PRIVATE Qt${QT_VERSION_MAJOR}::${_m})
    endforeach()
    if(WIN32)
        foreach(_m ${DA_AP_QT_WIN32_PUBLIC})
            target_link_libraries(${_name} PUBLIC Qt${QT_VERSION_MAJOR}::${_m})
        endforeach()
    endif()
    foreach(_m ${DA_AP_LINK_PUBLIC})
        target_link_libraries(${_name} PUBLIC ${DA_PROJECT_NAME}::${_m})
    endforeach()
    foreach(_m ${DA_AP_LINK_PRIVATE})
        target_link_libraries(${_name} PRIVATE ${DA_PROJECT_NAME}::${_m})
    endforeach()
    if(DA_AP_COMPILE_DEFINITIONS_PRIVATE)
        target_compile_definitions(${_name} PRIVATE ${DA_AP_COMPILE_DEFINITIONS_PRIVATE})
    endif()
    foreach(_d ${DA_AP_INCLUDE_PRIVATE})
        if(IS_ABSOLUTE "${_d}")
            target_include_directories(${_name} PRIVATE "${_d}")
        else()
            target_include_directories(${_name} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/${_d}")
        endif()
    endforeach()
    if(DA_AP_THIRDPARTY)
        da_link_3rdparty(${_name} LIBS ${DA_AP_THIRDPARTY}
            INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
    endif()

    include(GNUInstallDirs)
    install(TARGETS ${_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins
    )
    message(STATUS "plugin ${_name} ${DA_AP_VERSION} -> ${CMAKE_INSTALL_PREFIX}/bin/plugins")
endfunction()

# ===========================================================================
# da_add_test —— 测试一站式声明（合并原 damacro_setup_test 与各测试手写块）
#
# da_add_test(
#     NAME <name>
#     SOURCES <src...>                  # 通常为 main.cpp
#     [QT_COMPONENTS <comp...>]         # find_package + 链接（Test 等）
#     [LINK_PUBLIC <target...>]         # 完整目标名（DAWorkbench::DAAgent、Qt5::Test）
#     [LINK_PRIVATE <target...>]
#     [INCLUDE_PRIVATE <dir...>]        # 私有 include 目录
#     [DEPLOY_QT]                       # 构建后 windeployqt 部署 Qt DLL
#     [DEPLOY_TARGETS <t...>]           # 传给 windeployqt 额外扫描的 target（捕获 DA DLL 的 Qt 传递依赖）
#     [COPY_DLL_TARGETS <t...>]         # 复制第三方 DLL（SHARED_LIBRARY imported target）
# )
# 附带行为：exe 输出到 build/bin（与 DA DLL 同目录）、WIN32_EXECUTABLE OFF（保留控制台输出）、
# add_test 注册、生成 run_<name> 目标、登记到全局属性 DA_TEST_TARGETS（供 run_all_tests 聚合）
# ===========================================================================
function(da_add_test)
    set(_opts DEPLOY_QT)
    set(_one NAME)
    set(_multi SOURCES QT_COMPONENTS LINK_PUBLIC LINK_PRIVATE INCLUDE_PRIVATE
        DEPLOY_TARGETS COPY_DLL_TARGETS)
    cmake_parse_arguments(DA_AT "${_opts}" "${_one}" "${_multi}" ${ARGN})

    if(NOT DA_AT_NAME)
        message(FATAL_ERROR "da_add_test: NAME is required")
    endif()

    set(_name ${DA_AT_NAME})

    if(DA_AT_QT_COMPONENTS)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${DA_AT_QT_COMPONENTS} REQUIRED)
    endif()

    add_executable(${_name} ${DA_AT_SOURCES})
    set_target_properties(${_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        # 测试保留控制台输出
        WIN32_EXECUTABLE OFF
        # 输出到 build/bin，与 DA DLLs 同目录
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    foreach(_t ${DA_AT_QT_COMPONENTS})
        target_link_libraries(${_name} PRIVATE Qt${QT_VERSION_MAJOR}::${_t})
    endforeach()
    foreach(_t ${DA_AT_LINK_PUBLIC})
        target_link_libraries(${_name} PUBLIC ${_t})
    endforeach()
    foreach(_t ${DA_AT_LINK_PRIVATE})
        target_link_libraries(${_name} PRIVATE ${_t})
    endforeach()
    foreach(_d ${DA_AT_INCLUDE_PRIVATE})
        target_include_directories(${_name} PRIVATE "${_d}")
    endforeach()

    if(WIN32)
        # windeployqt 只扫描传入文件的直接依赖，不递归扫描同目录的 DA DLL 的二次 Qt 依赖，
        # 故须把携带 Qt 依赖的 DA DLL 都显式传入（DEPLOY_TARGETS），否则测试 exe 启动
        # 报 0xC0000135（DLL not found）
        if(DA_AT_DEPLOY_QT)
            get_target_property(_qmake_exe Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
            find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir}")
            if(WINDEPLOYQT_EXE)
                set(_deploy_files "$<TARGET_FILE:${_name}>")
                foreach(_dt ${DA_AT_DEPLOY_TARGETS})
                    if(TARGET ${_dt})
                        list(APPEND _deploy_files "$<TARGET_FILE:${_dt}>")
                    endif()
                endforeach()
                add_custom_command(TARGET ${_name} POST_BUILD
                    COMMAND "${WINDEPLOYQT_EXE}"
                            $<IF:$<CONFIG:Debug>,--debug,--release>
                            --no-translations
                            --no-system-d3d-compiler
                            --no-opengl-sw
                            ${_deploy_files}
                    COMMENT "Deploying Qt runtime DLLs for ${_name}"
                )
            else()
                message(WARNING "windeployqt not found in ${_qt_bin_dir}, Qt DLLs will not be deployed for ${_name}")
            endif()
        endif()

        # 复制第三方 DLLs（仅 SHARED_LIBRARY 类型的 imported target）
        foreach(_dll_target ${DA_AT_COPY_DLL_TARGETS})
            if(TARGET ${_dll_target})
                get_target_property(_dll_type ${_dll_target} TYPE)
                if(_dll_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET ${_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "$<TARGET_FILE:${_dll_target}>"
                                "$<TARGET_FILE_DIR:${_name}>"
                        COMMENT "Copying ${_dll_target} for ${_name}"
                    )
                endif()
            endif()
        endforeach()
    endif()

    add_test(NAME ${_name} COMMAND ${_name})

    # 登记到全局属性，src/tst/CMakeLists.txt 据此聚合 run_all_tests
    set_property(GLOBAL APPEND PROPERTY DA_TEST_TARGETS ${_name})

    add_custom_target(run_${_name}
        COMMAND $<TARGET_FILE:${_name}>
        DEPENDS ${_name}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${_name}>
        COMMENT "Running ${_name}"
    )
    message(STATUS "test ${_name} registered")
endfunction()
