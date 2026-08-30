#
# DAWorkbench 构建工具（部署相关）
#
# 历史上本文件还包含 damacro_lib_setting / damacro_set_lib_properties / damacro_lib_install /
# damacro_app_setting / damacro_set_app_properties / damacro_app_install / damacro_setup_test
# 等三段式构建宏，已全部被 cmake/DAWorkbench.cmake 的声明式 API
# （da_add_library / da_add_executable / da_add_plugin / da_add_test）取代并删除。
#
# 本文件现只保留三个与"构建产物部署"相关的 function。
#

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
# ⚠️ windeployqt 只扫描传入文件的直接依赖，不递归扫描同目录文件（见 da_add_test 注释），
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
