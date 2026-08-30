# 获取默认安装目录
function(dafun_set_bin_name _var)
    set(_MIN_QT_VERSION 5.14)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
    ########################################################
    # 平台判断
    ########################################################
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(_platform_name "x86")
    else()
        set(_platform_name "x64")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER_ID OR CMAKE_CXX_COMPILER_ID STREQUAL "")
        message(WARNING "CMAKE_CXX_COMPILER_ID is NULL,maybe your cmake env error")
        set(_CXX_COMPILER_ID "MSVC")
    else()
        set(_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID})
    endif()
    ########################################################
    # 构建类型：
    # 单配置生成器 (Ninja/Makefiles) → CMAKE_BUILD_TYPE（如 Release/Debug）
    # 多配置生成器 (Visual Studio)   → CMAKE_BUILD_TYPE 为空，用缓存变量 DA_INSTALL_CONFIG_TYPE
    if(CMAKE_CONFIGURATION_TYPES)
        set(_config_type "${DA_INSTALL_CONFIG_TYPE}" CACHE STRING
            "安装目录使用的构建类型（仅多配置生成器生效，如 Visual Studio）")
        if(NOT _config_type)
            set(_config_type "Release")
            set(DA_INSTALL_CONFIG_TYPE "Release" CACHE STRING
                "安装目录使用的构建类型（仅多配置生成器生效，如 Visual Studio）" FORCE)
        endif()
    else()
        set(_config_type "${CMAKE_BUILD_TYPE}")
    endif()
    ########################################################
    # 安装路径设置 设置变量值，并传递到父作用域
    ########################################################
    set(${_var} "bin_${_config_type}_qt${QT_VERSION}_${_CXX_COMPILER_ID}_${_platform_name}" PARENT_SCOPE)
endfunction()



# 定义DA_LIB的宏
# _lib_name lib的名字，决定变量DA_LIB_NAME
# _lib_description lib的描述，决定变量DA_LIB_DESCRIPTION
# _lib_ver_major lib的主版本号，决定变量DA_LIB_VERSION_MAJOR
# _lib_ver_minor lib的次版本号，决定变量DA_LIB_VERSION_MINOR
# _lib_ver_path lib的末版本号，决定变量DA_LIB_VERSION_PATCH
# 生成：DA_LIB_VERSION，完整的版本名
# 生成：DA_LIB_FULL_DESCRIPTION，完整的描述
# 生成：DA_MIN_QT_VERSION 最低qt版本要求
macro(damacro_lib_setting _lib_name _lib_description _lib_ver_major _lib_ver_minor _lib_ver_path)
    set(DA_MIN_QT_VERSION 5.14)
    set(DA_LIB_NAME ${_lib_name})
    set(DA_LIB_DESCRIPTION ${_lib_description})
    set(DA_LIB_VERSION_MAJOR ${_lib_ver_major})
    set(DA_LIB_VERSION_MINOR ${_lib_ver_minor})
    set(DA_LIB_VERSION_PATCH ${_lib_ver_path})
    set(DA_LIB_VERSION "${DA_LIB_VERSION_MAJOR}.${DA_LIB_VERSION_MINOR}.${DA_LIB_VERSION_PATCH}")
    set(DA_LIB_FULL_DESCRIPTION "${DA_PROJECT_NAME}::${DA_LIB_NAME} ${DA_LIB_VERSION} | ${DA_LIB_DESCRIPTION}")

    project(${DA_LIB_NAME}
        VERSION ${DA_LIB_VERSION}
        LANGUAGES CXX
        DESCRIPTION ${DA_LIB_FULL_DESCRIPTION}
    )
    ########################################################
    # 通用常规设置
    ########################################################
    # C++标准要求最低C++17
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
        endif(MSVC)
    # 编译选项
    set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
    set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    ########################################################
    # MSVC设置
    ########################################################
    if(MSVC)
    # msvc utf-8
        add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()
    ########################################################
    # 目录包含
    ########################################################
    # 包含自身目录
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    # 默认的CMAKE_INSTALL_PREFIX
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../${DA_BIN_DIR_NAME}")
    set(DA_GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h)
    # DAShared目录
    set(DA_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../DAShared)
    set(${DA_PROJECT_NAME}_DIR "${CMAKE_BINARY_DIR}")
    ########################################################
    # 打印信息
    ########################################################
    message("")
    message("${DA_LIB_FULL_DESCRIPTION}")
    message(STATUS "  | => DA_LIB_NAME=${DA_LIB_NAME}")
    message(STATUS "  | => DA_GLOBAL_HEADER=${DA_GLOBAL_HEADER}")
    message(STATUS "  | => DA_SHARED_DIR=${DA_SHARED_DIR}")
    message(STATUS "  | => CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  | => CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  | => CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(damacro_lib_setting)

macro(damacro_set_lib_properties _target_name _version_str)
    set_target_properties(${_target_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        VERSION ${_version_str}
        EXPORT_NAME ${_target_name}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro(damacro_set_lib_properties)

macro(damacro_set_app_properties _target_name _version_str)
    if(${QT_VERSION} VERSION_LESS 6.1.0)
      set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${_target_name})
    endif()
    set_target_properties(${_target_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        WIN32_EXECUTABLE TRUE
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        VERSION ${_version_str}
        EXPORT_NAME ${_target_name}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    if(MSVC)
        target_compile_options(${_target_name} PRIVATE "/utf-8")
        target_compile_definitions(${_target_name} PRIVATE "_UNICODE" "UNICODE")
        # 为 MSVC 设置链接器标志以禁止生成清单文件
        # 这是因为某些操作系统下会遇到general error c101008d: Failed to write the updated manifest to the resource of file "bin\DAWorkBench.exe的错误
        # 主要是操作系统杀毒软件的原因，某些生产环节下是无法禁用杀毒软件的
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
    endif()
endmacro(damacro_set_app_properties)

# 通用的安装
macro(damacro_lib_install_no_rc)
    include(GNUInstallDirs)
    target_compile_definitions(${DA_LIB_NAME} PRIVATE QT_MESSAGELOGCONTEXT)
    ########################################################
    # 目标依赖目录
    ########################################################
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${DA_LIB_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    )
    # 这个主要是DAGlobal.h
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>
    )
    # 这个主要是DAShared
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/DAShared>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../DAShared>
    )
    # 导出到统一的目标
    install(TARGETS ${DA_LIB_NAME} # 库的名字
        EXPORT ${DA_TARGET_NAME}   # DAWorkbenchTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    message(STATUS "${DA_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_lib_install_no_rc)

# 通用的安装
macro(damacro_lib_install)
    ########################################################
    # dll资源信息添加到 target_sources中
    ########################################################
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/create_win32_resource_version.cmake)
    if(WIN32)
            create_win32_resource_version(
                    TARGET ${DA_LIB_NAME}
                    FILENAME ${DA_LIB_NAME}
                    VERSION ${DA_LIB_VERSION}
                    EXT "dll"
                    DESCRIPTION ${DA_LIB_DESCRIPTION}
            )

            # set(__rc_path "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}.rc")
            # if(NOT EXISTS "${__rc_path}")
            #     generate_win32_rc_file(
            #         PATH "${__rc_path}"
            #         VERSION "${DA_LIB_VERSION}"
            #         COMPANY "czy"
            #         DESCRIPTION "${DA_LIB_DESCRIPTION}"
            #         COPYRIGHT "LGPL License"
            #         PRODUCT "${DA_LIB_NAME}"
            #     )
            # endif()
            # target_sources(${DA_LIB_NAME} PRIVATE "${__rc_path}")
    endif()
    damacro_lib_install_no_rc()
endmacro(damacro_lib_install)



# 定义DA_APP的宏
# _app_name lib的名字，决定变量DA_APP_NAME
# _app_description lib的描述，决定变量DA_APP_DESCRIPTION
# _app_ver_major lib的主版本号，决定变量DA_APP_VERSION_MAJOR
# _app_ver_minor lib的次版本号，决定变量DA_APP_VERSION_MINOR
# _app_ver_path lib的末版本号，决定变量DA_APP_VERSION_PATCH
# 生成：DA_APP_VERSION，完整的版本名
# 生成：DA_APP_FULL_DESCRIPTION，完整的描述
# 生成：DA_MIN_QT_VERSION 最低qt版本要求
macro(damacro_app_setting _app_name _app_description _app_ver_major _app_ver_minor _app_ver_path)
    set(DA_MIN_QT_VERSION 5.14)
    set(DA_APP_NAME ${_app_name})
    set(DA_APP_DESCRIPTION ${_app_description})
    set(DA_APP_VERSION_MAJOR ${_app_ver_major})
    set(DA_APP_VERSION_MINOR ${_app_ver_minor})
    set(DA_APP_VERSION_PATCH ${_app_ver_path})
    set(DA_APP_VERSION "${DA_APP_VERSION_MAJOR}.${DA_APP_VERSION_MINOR}.${DA_APP_VERSION_PATCH}")
    set(DA_APP_FULL_DESCRIPTION "${DA_APP_DESCRIPTION}")

    project(${DA_APP_NAME}
        VERSION ${DA_APP_VERSION}
        LANGUAGES CXX
        DESCRIPTION ${DA_APP_NAME}
        HOMEPAGE_URL "https://github.com/czyt1988"
    )
    ########################################################
    # 通用常规设置
    ########################################################
    # C++标准要求最低C++17
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    # 编译选项
    set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
    set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
    ########################################################
    # MSVC设置
    ########################################################
    if(MSVC)
    # msvc utf-8
        add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()
    ########################################################
    # 目录包含
    ########################################################
    # 包含自身目录
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    # 默认的CMAKE_INSTALL_PREFIX
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../${DA_BIN_DIR_NAME}")
    set(DA_GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h)
    set(${DA_PROJECT_NAME}_DIR "${CMAKE_BINARY_DIR}")
    ########################################################
    # 打印信息
    ########################################################
    message("")
    message("${DA_APP_FULL_DESCRIPTION}")
    message(STATUS "  | => DA_APP_NAME=${DA_APP_NAME}")
    message(STATUS "  | => DA_GLOBAL_HEADER=${DA_GLOBAL_HEADER}")
    message(STATUS "  | => CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  | => CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  | => CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(damacro_app_setting)

# 通用的安装
macro(damacro_app_install)
    ########################################################
    # 目标依赖目录
    ########################################################
    # 声明导出target的名称
    install(TARGETS ${DA_APP_NAME} RUNTIME DESTINATION bin)
    message(STATUS "${DA_APP_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_app_install)


# 为测试目标设置通用属性并部署 Windows 运行时依赖
#
# 解决 Windows 下测试 exe 找不到 DA / Qt / 第三方 DLL 的问题：
# 1. 将测试 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLLs 同目录）
# 2. 可选：通过 windeployqt 部署 Qt DLLs（同时扫描 DEPLOY_TARGETS 指定的 DA DLL，
#    以捕获 exe → DA DLL → Qt DLL 的传递依赖，如 Qt6Xml / Qt6Core5Compat）
# 3. 可选：复制指定的第三方 DLL 目标（通过 TARGET_FILE 生成器表达式自动选择 Debug/Release 版本）
#
# 用法:
#   damacro_setup_test(TargetName
#       DEPLOY_QT                                      # 部署 Qt DLLs
#       DEPLOY_TARGETS DAWorkbench::DAMessageHandler   # 额外传给 windeployqt 扫描的 target
#       COPY_DLL_TARGETS spdlog::spdlog                # 复制额外第三方 DLL
#   )
macro(damacro_setup_test _target_name)
    set(options DEPLOY_QT)
    set(oneValueArgs "")
    set(multiValueArgs COPY_DLL_TARGETS DEPLOY_TARGETS)
    cmake_parse_arguments(_arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set_target_properties(${_target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        # 输出到 build/bin，与 DA DLLs 同目录
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    if(WIN32)
        # 部署 Qt DLLs
        if(_arg_DEPLOY_QT)
            get_target_property(_qmake_exe Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
            find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir}")
            if(WINDEPLOYQT_EXE)
                # windeployqt 只扫描传入文件的直接依赖，不会自动扫描同目录的 DLL
                # 因此需要同时传入 exe 和 DA DLL，让 windeployqt 发现 DA DLL 的 Qt 传递依赖
                # （如 DAUtilsd.dll → Qt6Xmld.dll / Qt6Core5Compatd.dll）
                set(_deploy_files "$<TARGET_FILE:${_target_name}>")
                foreach(_deploy_target ${_arg_DEPLOY_TARGETS})
                    if(TARGET ${_deploy_target})
                        list(APPEND _deploy_files "$<TARGET_FILE:${_deploy_target}>")
                    endif()
                endforeach()

                add_custom_command(TARGET ${_target_name} POST_BUILD
                    COMMAND "${WINDEPLOYQT_EXE}"
                            $<IF:$<CONFIG:Debug>,--debug,--release>
                            --no-translations
                            --no-system-d3d-compiler
                            --no-opengl-sw
                            ${_deploy_files}
                    COMMENT "Deploying Qt runtime DLLs for ${_target_name}"
                )
            else()
                message(WARNING "windeployqt not found in ${_qt_bin_dir}, Qt DLLs will not be deployed for ${_target_name}")
            endif()
        endif()

        # 复制第三方 DLLs（仅 SHARED_LIBRARY 类型的 imported target）
        foreach(_dll_target ${_arg_COPY_DLL_TARGETS})
            if(TARGET ${_dll_target})
                get_target_property(_dll_type ${_dll_target} TYPE)
                if(_dll_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET ${_target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "$<TARGET_FILE:${_dll_target}>"
                                "$<TARGET_FILE_DIR:${_target_name}>"
                        COMMENT "Copying ${_dll_target} for ${_target_name}"
                    )
                endif()
            endif()
        endforeach()
    endif()
endmacro(damacro_setup_test)


# 将第三方库 DLL 部署到构建输出目录，确保开发调试时能正确加载。
#
# 第三方库（DAWidgets/SARibbonBar/qwt/ads/DALiteCtk/quazip/zlib/python 等）通过
# src/3rdparty/CMakeLists.txt 编译安装到 ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/。
# 本函数在指定 target 的 POST_BUILD 阶段，按白名单把该目录下的第三方 *.dll 复制到 target
# 的输出目录，避免开发时手动复制。
#
# ⚠️ 重要：本函数只按白名单复制"第三方"DLL，不会复制项目自身的 DA 模块 DLL。
# 因为 ${CMAKE_INSTALL_PREFIX}/bin 在 DA_AUTO_INSTALL_PREFIX=ON 时同时存放第三方库和 DA 自身模块的
# 安装产物，DA 自身 DLL 由各 target 的 RUNTIME_OUTPUT_DIRECTORY 直接输出到 build/bin，
# 若从 install 目录复制旧的 DA DLL 回来会覆盖刚编译的新产物，导致符号不一致（如
# directionalCallback 缺失）的"幽灵 DLL"运行时崩溃。
#
# 采用白名单而非黑名单：第三方库相对固定（见 src/3rdparty/CMakeLists.txt 的 add_subdirectory），
# 新增第三方库时在此列表追加即可；这样比"排除 DA 模块"更明确，也避免误把以 DA 开头的第三方库
# （如 DAWidgets/DALiteCtk）当作 DA 自身模块过滤掉。
#
# 函数在配置期通过 file(GLOB) 收集 DLL 列表（第三方库相对稳定，无需每次构建重新扫描）；
# 复制使用 copy_if_different，未变更的 DLL 不会触发实际 IO。
#
# 用法:
#   dafun_deploy_3rdparty_dlls(${DA_APP_NAME})
#   dafun_deploy_3rdparty_dlls(${DA_APP_NAME} PLUGIN_DIR ${CMAKE_BINARY_DIR}/bin/plugins)
function(dafun_deploy_3rdparty_dlls _target_name)
    cmake_parse_arguments(_arg "" "PLUGIN_DIR" "" ${ARGN})

    if(NOT WIN32)
        return()
    endif()

    # 第三方库安装目录的 bin 文件夹
    set(_3rdparty_bin_dir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
    if(NOT EXISTS "${_3rdparty_bin_dir}")
        message(WARNING "dafun_deploy_3rdparty_dlls: 3rdparty bin dir not found: ${_3rdparty_bin_dir}")
        return()
    endif()

    # 配置期收集所有 DLL（第三方库相对稳定，无需每次构建重新扫描）
    file(GLOB _all_dlls "${_3rdparty_bin_dir}/*.dll")
    if(NOT _all_dlls)
        message(STATUS "dafun_deploy_3rdparty_dlls: no DLLs found in ${_3rdparty_bin_dir}")
        return()
    endif()

    # 第三方库 DLL 基名白名单（不含 .dll 扩展名，不含 Debug 'd' 后缀）。
    # 来源：src/3rdparty/CMakeLists.txt 中 add_subdirectory 的第三方库 + Python 运行时。
    # 匹配规则：文件名（去扩展名）等于 <基名>（Release）或 <基名>d（Debug，CMAKE_DEBUG_POSTFIX）。
    # 注意：
    # - DAWidgets / DALiteCtk 虽以 DA 开头，但属第三方库（src/3rdparty 编译安装），必须列入
    # - zlib 同时有 zlib.dll 和 zlibd.dll，基名 "zlib" 匹配 zlib + zlibd，正好覆盖两个文件
    # - python311 不区分 Debug/Release，基名 "python311" 仅匹配自身
    # - 带 Qt 版本号的库用 ${QT_VERSION_MAJOR} 适配 Qt5/Qt6
    # - spdlog 默认构建为动态库（SPDLOG_BUILD_SHARED 默认 ON），DAMessageHandler
    #   链接后运行期依赖 spdlog.dll，必须列入；pybind11/ordered-map 是头文件库，无 DLL，不列入
    # 新增第三方库时，在此列表追加基名即可。
    set(_3rdparty_dll_basenames
        SARibbonBar
        DAWidgets
        DALiteCtk
        qwtcore
        qwtplot
        qwtplot3d
        qtadvanceddocking-qt${QT_VERSION_MAJOR}
        quazip1-qt${QT_VERSION_MAJOR}
        zlib
        spdlog
        python311)

    # 按白名单筛选：只复制基名匹配的 DLL
    set(_3rdparty_dlls)
    set(_skipped_dlls)
    foreach(_dll IN LISTS _all_dlls)
        get_filename_component(_dll_name "${_dll}" NAME_WE)
        set(_matched FALSE)
        foreach(_base IN LISTS _3rdparty_dll_basenames)
            if(_dll_name STREQUAL "${_base}" OR _dll_name STREQUAL "${_base}d")
                set(_matched TRUE)
                break()
            endif()
        endforeach()
        if(_matched)
            list(APPEND _3rdparty_dlls "${_dll}")
        else()
            list(APPEND _skipped_dlls "${_dll_name}")
        endif()
    endforeach()
    if(_skipped_dlls)
        list(JOIN _skipped_dlls ", " _skipped_msg)
        message(STATUS "dafun_deploy_3rdparty_dlls: skipped non-3rdparty DLLs: ${_skipped_msg}")
    endif()
    if(NOT _3rdparty_dlls)
        message(STATUS "dafun_deploy_3rdparty_dlls: no 3rdparty DLLs to deploy after filtering")
        return()
    endif()

    # 主输出目录：target 的输出目录
    set(_main_out_dir "$<TARGET_FILE_DIR:${_target_name}>")

    # 构建复制命令列表
    set(_copy_commands
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_3rdparty_dlls}
                ${_main_out_dir}
    )

    # 如果指定了插件目录，也复制一份（插件 DLL 需要第三方依赖）
    if(_arg_PLUGIN_DIR)
        list(APPEND _copy_commands
            # copy_if_different 不会自动创建目录,需先确保插件目录存在
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_arg_PLUGIN_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_3rdparty_dlls}
                    ${_arg_PLUGIN_DIR}
        )
    endif()

    add_custom_command(TARGET ${_target_name} POST_BUILD
        ${_copy_commands}
        COMMENT "Deploying 3rdparty DLLs to build output (target: ${_target_name})"
    )

    list(LENGTH _3rdparty_dlls _dll_count)
    message(STATUS "dafun_deploy_3rdparty_dlls: ${_target_name} will deploy ${_dll_count} DLLs from ${_3rdparty_bin_dir}")
endfunction()


# 安装阶段通过 windeployqt 把完整 Qt 运行时部署到安装目录（打包来源目录）
#
# 背景：install(TARGETS) 只安装 DA 自身产物，Qt 运行时 DLL/插件/翻译不会自动进入安装目录，
# 直接打包 bin_<Config>_qt<X>_... 目录会缺失 Qt 依赖（典型症状：找不到 Qt5WebEngineWidgets.dll——
# WebEngine/Qml 是 DAGui.dll 的传递依赖，而非主程序 exe 的直接依赖，手工 windeployqt 时
# 只传 exe 就会漏掉整条依赖链）。
# 本函数追加一条 install(CODE) 规则：安装时用 windeployqt 扫描二进制并通过 --dir 把完整
# Qt 运行时（DLL、platforms 等插件目录、翻译、WebEngine 资源）部署到安装目录的 bin/。
#
# ⚠️ 扫描目标是"构建树产物"（$<TARGET_FILE:...>）而不是安装目录中的文件：
# 顶层 cmake_install.cmake 先执行顶层自身的 install 规则、再 include 各子目录的安装脚本，
# 顶层的 install(CODE) 运行时 exe/库/插件尚未安装到安装目录；而构建产物在安装前必然存在，
# 且与 install(TARGETS) 即将拷贝的文件完全一致，天然规避执行顺序问题。
#
# ⚠️ windeployqt 只扫描传入文件的直接依赖，不递归扫描同目录文件（见 damacro_setup_test 注释），
# 因此：
# - SCAN_TARGETS 必须传入引入深层 Qt 依赖的库（如 DAGui）
# - PLUGIN_DIR 下的插件 DLL 会被自动通配扫描，捕获插件引入的额外 Qt 依赖
#
# 其他注意：
# - install(CODE) 内使用生成器表达式需要 CMP0087 NEW（CMake 3.14+），版本不足时告警跳过
# - $<TARGET_FILE:...> 按安装配置解析（Debug 产物带 d 后缀）
#
# 用法:
#   dafun_install_deploy_qt_runtime(${DA_APP_NAME}
#       SCAN_TARGETS DAWorkbench::DAGui   # 额外扫描的库（捕获其 Qt 传递依赖）
#       PLUGIN_DIR bin/plugins            # 插件目录（相对构建目录，自动适配 <CONFIG> 子目录）
#   )
function(dafun_install_deploy_qt_runtime _target_name)
    cmake_parse_arguments(_arg "" "PLUGIN_DIR" "SCAN_TARGETS" ${ARGN})

    if(NOT WIN32)
        return()
    endif()
    if(NOT POLICY CMP0087)
        message(WARNING "dafun_install_deploy_qt_runtime requires CMake 3.14+ (CMP0087), skip Qt runtime deployment at install stage")
        return()
    endif()

    get_target_property(_qmake_exe Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir}")
    if(NOT WINDEPLOYQT_EXE)
        message(WARNING "windeployqt not found in ${_qt_bin_dir}, Qt runtime will not be deployed at install for ${_target_name}")
        return()
    endif()

    # 扫描目标：主程序 + SCAN_TARGETS 指定的库（构建产物路径，安装期按安装配置解析）。
    # ⚠️ 必须用"空格分隔的带引号路径文本"拼装，不能用 CMake 列表：列表展开后形如
    # "a";"b"（引号紧贴分号），CMake 解析器会把第二个元素的引号保留为字面字符
    # （并报 Syntax Warning），导致 EXISTS 检查失败、该文件被静默丢弃
    set(_scan_files "\"$<TARGET_FILE:${_target_name}>\"")
    foreach(_scan_target ${_arg_SCAN_TARGETS})
        if(TARGET ${_scan_target})
            string(APPEND _scan_files " \"$<TARGET_FILE:${_scan_target}>\"")
        else()
            message(WARNING "dafun_install_deploy_qt_runtime: ${_scan_target} is not a valid target, skipped")
        endif()
    endforeach()

    # 补充扫描脚本片段（安装期执行）：通配 exe 同目录的 DA*.dll（兜底捕获新模块将来引入的
    # Qt 依赖）和插件目录；插件目录同时通配 <CONFIG> 子目录（VS 多配置）与平级（单配置）两种
    # 布局。插件目录中可能混入拷贝到那里的第三方 DLL（SARibbonBar/qwt 等），对 windeployqt
    # 的依赖并集无害；非 Qt 二进制（zlib/python 等）会被 windeployqt 跳过。
    # 用 list(FIND) 去重（不用 IN_LIST：安装脚本策略上下文为 CMP0057 OLD，IN_LIST 不可用）
    set(_glob_script "
    file(GLOB _globbed_da \"${CMAKE_BINARY_DIR}/bin/\${CMAKE_INSTALL_CONFIG_NAME}/DA*.dll\")
    file(GLOB _globbed_da_flat \"${CMAKE_BINARY_DIR}/bin/DA*.dll\")
    set(_all_globbed \${_globbed_da} \${_globbed_da_flat})")
    if(_arg_PLUGIN_DIR)
        string(APPEND _glob_script "
    file(GLOB _globbed_pl \"${CMAKE_BINARY_DIR}/${_arg_PLUGIN_DIR}/*.dll\")
    file(GLOB _globbed_pl_cfg \"${CMAKE_BINARY_DIR}/${_arg_PLUGIN_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/*.dll\")
    list(APPEND _all_globbed \${_globbed_pl} \${_globbed_pl_cfg})")
    endif()
    string(APPEND _glob_script "
    foreach(_g IN LISTS _all_globbed)
        list(FIND _wdq_scan_files \"\${_g}\" _g_idx)
        if(_g_idx EQUAL -1)
            list(APPEND _wdq_scan_files \"\${_g}\")
        endif()
    endforeach()")

    cmake_policy(SET CMP0087 NEW)
    install(CODE "
    set(_wdq_scan_files ${_scan_files})${_glob_script}
    # 过滤掉不存在的文件，避免个别产物缺失时 windeployqt 整体失败
    set(_wdq_existing)
    foreach(_f IN LISTS _wdq_scan_files)
        if(EXISTS \"\${_f}\")
            list(APPEND _wdq_existing \"\${_f}\")
        endif()
    endforeach()
    if(NOT _wdq_existing)
        message(WARNING \"windeployqt: no scan targets found, skip Qt runtime deployment\")
    else()
        if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"Debug\")
            set(_wdq_mode --debug)
        else()
            set(_wdq_mode --release)
        endif()
        message(STATUS \"Deploying Qt runtime (\${_wdq_mode}) to \${CMAKE_INSTALL_PREFIX}/bin ...\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXE}\" \${_wdq_mode} --dir \"\${CMAKE_INSTALL_PREFIX}/bin\" \${_wdq_existing}
            RESULT_VARIABLE _wdq_result
        )
        if(NOT _wdq_result EQUAL 0)
            message(WARNING \"windeployqt exited with code \${_wdq_result}, Qt runtime in install dir may be incomplete\")
        endif()
    endif()
")

    message(STATUS "dafun_install_deploy_qt_runtime: ${_target_name} will deploy Qt runtime at install stage via ${WINDEPLOYQT_EXE}")
endfunction()

