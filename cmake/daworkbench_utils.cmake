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


# 第三方库运行时 DLL 基名白名单（构建期复制与安装期部署共用同一份，避免两处漂移）。
#
# 来源：src/3rdparty/CMakeLists.txt 中 add_subdirectory 的第三方库 + Python 运行时。
# 匹配规则：文件名（去扩展名）等于 <基名>（Release）或 <基名>d（Debug，CMAKE_DEBUG_POSTFIX）。
# 注意：
# - DAWidgets / DALiteCtk 虽以 DA 开头，但属第三方库（src/3rdparty 编译安装），必须列入
# - zlib 的产物名随构建方式不同：新版 zlib 源码自带 CMake（SUFFIX "1.dll"）产出
#   zlib1.dll（Debug 对应 zd.dll），而既有的独立/vcpkg 安装可能仍是 zlib.dll /
#   zlibd.dll。两种命名都列入，否则内网（zlib.dll）干净构建时会被静默跳过，
#   运行期 quazip 加载失败。多列不会造成误拷。
# - python311/python312 的实际文件名随 Python 版本变，这里列当前使用的两个，
#   换版本时同步改（安装期由 top-level CMakeLists 的 install(FILES) 兜底）
# - 带 Qt 版本号的库用 ${QT_VERSION_MAJOR} 适配 Qt5/Qt6，因此必须在**调用期**展开：
#   本函数不能是文件顶层的 set()，否则 include 时 QT_VERSION_MAJOR 尚未由 find_package 确定。
# - spdlog 默认构建为动态库（SPDLOG_BUILD_SHARED 默认 ON），DAMessageHandler
#   链接后运行期依赖 spdlog.dll，必须列入；pybind11/ordered-map 是头文件库，无 DLL，不列入
# 新增第三方库时，在此列表追加基名即可。
function(dafun_3rdparty_dll_basenames _outvar)
    set(_basenames
        SARibbonBar
        DAWidgets
        DALiteCtk
        qwtcore
        qwtplot
        qwtplot3d
        qtadvanceddocking-qt${QT_VERSION_MAJOR}
        quazip1-qt${QT_VERSION_MAJOR}
        zlib
        zlib1
        zd
        spdlog
        python311
        python312)
    set(${_outvar} ${_basenames} PARENT_SCOPE)
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

    # 白名单见 dafun_3rdparty_dll_basenames()（构建期与安装期共用）。
    dafun_3rdparty_dll_basenames(_3rdparty_dll_basenames)

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


# 安装阶段部署第三方库运行时 DLL，保证 `cmake --install` 产物本身就是可直接运行/打包的目录。
#
# 背景（评审 D4）：dafun_deploy_3rdparty_dlls() 只在 POST_BUILD 把第三方 DLL 复制到 build/bin；
# `cmake --install` 不会安装它们，干净安装目录缺少 SARibbonBar/qwt/DAWidgets/ads/quazip/
# zlib/spdlog 等运行期依赖，启动即 STATUS_DLL_NOT_FOUND（发布流水线此前靠「补齐运行时 DLL」
# 步骤手工兜底）。把部署下沉到安装期后，安装目录自身完整，流水线兜底退化为幂等冗余。
#
# 说明：
# - 「来源目录」在配置期固定（= 第三方库安装前缀），不随 `cmake --install --prefix` 变化；
#   目标目录取安装期的 ${CMAKE_INSTALL_PREFIX}，因此支持装到任意干净目录。
# - Release 安装只取无后缀名；Debug 安装取 <基名>d（个别库如 zlib 的 Debug 名是 zd.dll，
#   基名以 d 结尾时直接取其本身）。
# - 安装后显式校验关键第三方 DLL 是否落盘，缺一个即安装失败（与 Qt 运行时校验同思路）。
#
# 用法（顶层 CMakeLists，include(GNUInstallDirs) 之后、dafun_install_deploy_qt_runtime 之前）：
#   dafun_install_3rdparty_runtime()
function(dafun_install_3rdparty_runtime)
    if(NOT WIN32)
        return()
    endif()
    set(_3rdparty_bin_dir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
    if(NOT EXISTS "${_3rdparty_bin_dir}")
        message(WARNING "dafun_install_3rdparty_runtime: 3rdparty bin dir not found: ${_3rdparty_bin_dir}")
        return()
    endif()

    dafun_3rdparty_dll_basenames(_3rdparty_dll_basenames)
    # VC++ 运行库来源：windeployqt 的 --compiler-runtime 在 install(CODE) 里可能静默跳过
    # （没有 VCINSTALLDIR），这里显式定位：优先第三方库安装前缀（本工程构建流程会在其中内置
    # msvcp140*/vcruntime140*），其次 VS Redist 目录。找不到只告警——发布流水线还有补齐步骤，
    # 最终由 scripts/verify_package.ps1 的导入表校验强校验（避免在无 VS 环境下误堵安装）。
    set(_vc_runtime_src "")
    if(EXISTS "${_3rdparty_bin_dir}/msvcp140.dll")
        set(_vc_runtime_src "${_3rdparty_bin_dir}")
    elseif(DEFINED ENV{VCToolsRedistDir} AND EXISTS "$ENV{VCToolsRedistDir}")
        file(GLOB _vc_crt_dirs "$ENV{VCToolsRedistDir}x64/Microsoft.VC*.CRT" "$ENV{VCToolsRedistDir}Microsoft.VC*.CRT")
        if(_vc_crt_dirs)
            list(GET _vc_crt_dirs 0 _vc_runtime_src)
        endif()
    endif()

    install(CODE "
    set(_src_bin \"${_3rdparty_bin_dir}\")
    set(_dst_bin \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
    if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"Debug\")
        set(_postfix \"d\")
    else()
        set(_postfix \"\")
    endif()
    set(_install_dlls)
    foreach(_base ${_3rdparty_dll_basenames})
        if(_postfix)
            if(EXISTS \"\${_src_bin}/\${_base}d.dll\")
                list(APPEND _install_dlls \"\${_src_bin}/\${_base}d.dll\")
            elseif(_base MATCHES \"d$\" AND EXISTS \"\${_src_bin}/\${_base}.dll\")
                # 基名本身以 d 结尾（如 zlib 的 Debug 产物 zd.dll）
                list(APPEND _install_dlls \"\${_src_bin}/\${_base}.dll\")
            endif()
        else()
            if(EXISTS \"\${_src_bin}/\${_base}.dll\")
                list(APPEND _install_dlls \"\${_src_bin}/\${_base}.dll\")
            endif()
        endif()
    endforeach()
    if(_install_dlls)
        file(INSTALL DESTINATION \"\${_dst_bin}\" TYPE FILE FILES \${_install_dlls})
    else()
        message(WARNING \"dafun_install_3rdparty_runtime: no 3rdparty DLL found in \${_src_bin}\")
    endif()
    # 关键第三方运行时校验：缺任何一个都意味着安装产物启动不了。
    set(_required_3rdparty SARibbonBar DAWidgets qwtplot qtadvanceddocking-qt${QT_VERSION_MAJOR} quazip1-qt${QT_VERSION_MAJOR} spdlog)
    set(_missing_3rdparty)
    foreach(_req \${_required_3rdparty})
        if(NOT EXISTS \"\${_dst_bin}/\${_req}\${_postfix}.dll\")
            list(APPEND _missing_3rdparty \"\${_req}\${_postfix}.dll\")
        endif()
    endforeach()
    if(_missing_3rdparty)
        string(JOIN \", \" _missing_3rdparty_txt \${_missing_3rdparty})
        message(FATAL_ERROR \"3rdparty runtime deployment incomplete, missing in install dir: \${_missing_3rdparty_txt}\")
    endif()
    # VC++ 运行库：目标机器不一定装 Redist，必须随包（verify_package 也会按导入表复查）。
    # Debug 配置用 *d.dll；来源目录为空时只告警，由打包自检兜底。
    set(_vc_src \"${_vc_runtime_src}\")
    if(_vc_src)
        set(_vc_bases msvcp140 msvcp140_1 msvcp140_2 vcruntime140 vcruntime140_1 concrt140)
        set(_vc_dlls)
        foreach(_vc IN LISTS _vc_bases)
            if(EXISTS \"\${_vc_src}/\${_vc}\${_postfix}.dll\")
                list(APPEND _vc_dlls \"\${_vc_src}/\${_vc}\${_postfix}.dll\")
            endif()
        endforeach()
        if(_vc_dlls)
            file(INSTALL DESTINATION \"\${_dst_bin}\" TYPE FILE FILES \${_vc_dlls})
        endif()
    else()
        message(WARNING \"dafun_install_3rdparty_runtime: VC++ runtime source not found, install dir may miss msvcp140/vcruntime140 (verify_package.ps1 will catch it at packaging time)\")
    endif()
")
    message(STATUS "dafun_install_3rdparty_runtime: 3rdparty runtime DLLs will be deployed at install stage from ${_3rdparty_bin_dir}")
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
#       REQUIRED_QT_MODULES Multimedia    # 安装后必须存在的 Qt 模块（缺失即安装失败）
#   )
function(dafun_install_deploy_qt_runtime _target_name)
    cmake_parse_arguments(_arg "" "PLUGIN_DIR" "SCAN_TARGETS;REQUIRED_QT_MODULES" ${ARGN})

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

    # 安装后必须存在的 Qt 运行时 DLL：新增 Qt 模块（如信号分析插件的 Multimedia）若被
    # windeployqt 漏扫，打包阶段不会报错、只在运行时缺 DLL（评审 D1），因此安装期显式校验。
    set(_required_qt_dlls
        "Qt${QT_VERSION_MAJOR}Core"
        "Qt${QT_VERSION_MAJOR}Gui"
        "Qt${QT_VERSION_MAJOR}Widgets")
    foreach(_qm ${_arg_REQUIRED_QT_MODULES})
        list(APPEND _required_qt_dlls "Qt${QT_VERSION_MAJOR}${_qm}")
    endforeach()

    # Multimedia 播放还依赖 mediaservice/ 下的后端插件（QMediaPlayer 运行时按平台从中加载），
    # 只校验主 DLL 不够：后端目录为空时安装期同样判失败（评审 N-D1）。
    set(_mediaservice_guard "")
    list(FIND _arg_REQUIRED_QT_MODULES "Multimedia" _multimedia_idx)
    if(NOT _multimedia_idx EQUAL -1)
        string(APPEND _mediaservice_guard "
    file(GLOB _mediaservice_dlls \"\${CMAKE_INSTALL_PREFIX}/bin/mediaservice/*.dll\")
    if(NOT _mediaservice_dlls)
        list(APPEND _qt_missing \"mediaservice/<backend>.dll（Qt Multimedia 播放依赖的后端目录为空或不存在）\")
    endif()")
    endif()

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
        # --compiler-runtime: 把 msvcp140/vcruntime140/vcruntime140_1/concrt140 一起拷进安装目录。
        # 不加这一项，windeployqt 只处理 Qt6*.dll，装出来的包在没装 VC++ 运行库的机器上
        # 会直接 0xc000007b / STATUS_DLL_NOT_FOUND（实测 exe 依赖 MSVCP140/VCRUNTIME140）。
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXE}\" \${_wdq_mode} --compiler-runtime --dir \"\${CMAKE_INSTALL_PREFIX}/bin\" \${_wdq_existing}
            RESULT_VARIABLE _wdq_result
        )
        if(NOT _wdq_result EQUAL 0)
            message(WARNING \"windeployqt exited with code \${_wdq_result}, Qt runtime in install dir may be incomplete\")
        endif()
    endif()
    # 校验关键 Qt DLL 是否真的落盘（Debug 产物带 d 后缀）
    if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"Debug\")
        set(_qt_dll_suffix \"d\")
    else()
        set(_qt_dll_suffix \"\")
    endif()
    set(_qt_missing)
    foreach(_qm ${_required_qt_dlls})
        if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/bin/\${_qm}\${_qt_dll_suffix}.dll\")
            list(APPEND _qt_missing \"\${_qm}\${_qt_dll_suffix}.dll\")
        endif()
    endforeach()${_mediaservice_guard}
    if(_qt_missing)
        string(JOIN \", \" _qt_missing_txt \${_qt_missing})
        message(FATAL_ERROR \"Qt runtime deployment incomplete, missing in install dir: \${_qt_missing_txt}\")
    endif()
")

    message(STATUS "dafun_install_deploy_qt_runtime: ${_target_name} will deploy Qt runtime at install stage via ${WINDEPLOYQT_EXE}")
endfunction()
