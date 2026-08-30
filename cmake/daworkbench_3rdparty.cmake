#
# DAWorkbench 第三方库统一导入
#
# da_link_3rdparty(<target>
#     LIBS <SARibbonBar|DALiteCtk|DAWidgets|ads|qwt|spdlog|quazip|python|pybind11|orderedmap ...>
#     [INSTALL_DIR <dir>]      # 插件 standalone 构建时传入 DAWorkbench 安装目录
#     [SCOPE <PUBLIC|PRIVATE>] # 链接作用域，默认 PRIVATE（pybind11/orderedmap 为 PUBLIC，见下）
# )
#
# 替代原 damacro_import_* 宏族（daworkbench_3rdparty.cmake 单参版与
# daworkbench_plugin_utils.cmake 双参版），消除两套同名宏靠 include 顺序区分的问题。
#
# 查找策略与原宏保持一致：
# - lib/cmake 常规包：find_package(CONFIG QUIET) → DA_INSTALL_LIB_CMAKE_PATH 下
#   glob ${pkg}* 目录按字母逆序取最新版本回退
# - share/cmake 包（pybind11 / tsl-ordered-map 安装位置特殊）：find_package() →
#   DA_INSTALL_LIB_SHARE_PATH 回退
# - INSTALL_DIR 模式（插件）：find_package(PATHS <INSTALL_DIR> ...) 在 DAWorkbench
#   安装目录内直接查找
#
# 默认链接作用域沿用原宏行为：常规库 PRIVATE；pybind11 / orderedmap 是 header-only
# 且下游模块编译需要其头文件搜索路径（如 DAPyBindQt 以 PUBLIC 链接 pybind11，
# DAGui/插件经传递获得 pybind11.h），故默认 PUBLIC。
#

# 在 lib/cmake 下查找包：CONFIG 优先，失败后 glob ${pkg}* 目录取最新版本
# （对应原 damacro_import_xxx 的回退逻辑）
function(_da_3rdparty_find_in_libcmake _pkg _install_dir)
    if(_install_dir)
        find_package(${_pkg} PATHS "${_install_dir}" REQUIRED)
        return()
    endif()
    find_package(${_pkg} CONFIG QUIET)
    if(${_pkg}_FOUND)
        return()
    endif()
    if(NOT DEFINED DA_INSTALL_LIB_CMAKE_PATH)
        message(FATAL_ERROR "da_link_3rdparty: can not find ${_pkg}, and DA_INSTALL_LIB_CMAKE_PATH is not defined")
    endif()
    file(GLOB _da_cand LIST_DIRECTORIES true "${DA_INSTALL_LIB_CMAKE_PATH}/${_pkg}*")
    if(NOT _da_cand)
        message(FATAL_ERROR "da_link_3rdparty: no ${_pkg}* directories found in: ${DA_INSTALL_LIB_CMAKE_PATH}")
    endif()
    # 若存在多个版本，按字母逆序取最新（假设版本号在目录名尾部且递增）
    list(SORT _da_cand)
    list(REVERSE _da_cand)
    list(GET _da_cand 0 _da_dir)
    message(STATUS "  |-try to find ${_pkg} in ${_da_dir}")
    find_package(${_pkg} CONFIG PATHS ${_da_dir} NO_DEFAULT_PATH REQUIRED)
endfunction()

# 在 share/cmake 下查找包（pybind11 / tsl-ordered-map 的安装位置）
function(_da_3rdparty_find_in_sharecmake _pkg _install_dir)
    find_package(${_pkg})
    if(${_pkg}_FOUND)
        return()
    endif()
    if(_install_dir)
        message(STATUS "  |-try to find ${_pkg} in ${_install_dir}/share/cmake")
        find_package(${_pkg} PATHS "${_install_dir}/share/cmake" REQUIRED)
        return()
    endif()
    if(DEFINED DA_INSTALL_LIB_SHARE_PATH)
        message(STATUS "  |-try to find ${_pkg} in ${DA_INSTALL_LIB_SHARE_PATH}/${_pkg}")
        find_package(${_pkg} PATHS "${DA_INSTALL_LIB_SHARE_PATH}/${_pkg}" REQUIRED)
        return()
    endif()
    message(FATAL_ERROR "da_link_3rdparty: can not find ${_pkg}")
endfunction()

function(da_link_3rdparty _target)
    set(_da_opts "")
    set(_da_one INSTALL_DIR SCOPE)
    set(_da_multi LIBS)
    cmake_parse_arguments(_da "${_da_opts}" "${_da_one}" "${_da_multi}" ${ARGN})

    if(NOT _da_LIBS)
        message(FATAL_ERROR "da_link_3rdparty(${_target}): LIBS is required")
    endif()

    foreach(_lib ${_da_LIBS})
        # 每个库的实际链接作用域：SCOPE 参数覆盖全部；未指定时 pybind11/orderedmap
        # 默认 PUBLIC（header-only，下游编译需要），其余默认 PRIVATE
        set(_scope PRIVATE)
        if(_da_SCOPE)
            set(_scope ${_da_SCOPE})
        elseif(_lib STREQUAL "pybind11" OR _lib STREQUAL "orderedmap")
            set(_scope PUBLIC)
        endif()

        if(_lib STREQUAL "SARibbonBar")
            _da_3rdparty_find_in_libcmake(SARibbonBar "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} SARibbonBar::SARibbonBar)
            message(STATUS "  |-${_target} link SARibbonBar::SARibbonBar (${_scope})")
        elseif(_lib STREQUAL "DALiteCtk" OR _lib STREQUAL "ctk")
            _da_3rdparty_find_in_libcmake(DALiteCtk "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} DALiteCtk::DALiteCtk)
            message(STATUS "  |-${_target} link DALiteCtk::DALiteCtk (${_scope})")
        elseif(_lib STREQUAL "DAWidgets")
            _da_3rdparty_find_in_libcmake(DAWidgets "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} DA::DAWidgets)
            message(STATUS "  |-${_target} link DA::DAWidgets (${_scope})")
        elseif(_lib STREQUAL "ads" OR _lib STREQUAL "QtAdvancedDocking")
            # ADS 4.x 起上游将包名从 qt6advanceddocking 重命名为 qtadvanceddocking-qt6，
            # 先试新名，失败再回退旧名
            set(_ads_new qtadvanceddocking-qt${QT_VERSION_MAJOR})
            set(_ads_old qt${QT_VERSION_MAJOR}advanceddocking)
            find_package(${_ads_new} CONFIG QUIET)
            if(${_ads_new}_FOUND)
                message(STATUS "  |-finded ${_ads_new}")
                target_link_libraries(${_target} ${_scope} ads::${_ads_new})
                message(STATUS "  |-${_target} link ads::${_ads_new} (${_scope})")
            else()
                message(STATUS "  |-can not find ${_ads_new}, fallback to ${_ads_old}")
                _da_3rdparty_find_in_libcmake(${_ads_old} "${_da_INSTALL_DIR}")
                target_link_libraries(${_target} ${_scope} ads::${_ads_old})
                message(STATUS "  |-${_target} link ads::${_ads_old} (${_scope})")
            endif()
        elseif(_lib STREQUAL "qwt")
            _da_3rdparty_find_in_libcmake(qwt "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} qwt::plot qwt::plot3d)
            message(STATUS "  |-${_target} link qwt::plot qwt::plot3d (${_scope})")
        elseif(_lib STREQUAL "spdlog")
            _da_3rdparty_find_in_libcmake(spdlog "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} spdlog::spdlog)
            message(STATUS "  |-${_target} link spdlog::spdlog (${_scope})")
        elseif(_lib STREQUAL "quazip")
            set(_quazip_pkg QuaZip-Qt${QT_VERSION_MAJOR})
            if(_da_INSTALL_DIR)
                find_package(${_quazip_pkg} PATHS "${_da_INSTALL_DIR}" REQUIRED)
            else()
                find_package(${_quazip_pkg})
                if(NOT ${_quazip_pkg}_FOUND)
                    if(NOT DEFINED DA_INSTALL_LIB_CMAKE_PATH)
                        message(FATAL_ERROR "da_link_3rdparty: can not find ${_quazip_pkg}, and DA_INSTALL_LIB_CMAKE_PATH is not defined")
                    endif()
                    file(GLOB _da_quazip LIST_DIRECTORIES true "${DA_INSTALL_LIB_CMAKE_PATH}/${_quazip_pkg}*")
                    if(NOT _da_quazip)
                        message(FATAL_ERROR "da_link_3rdparty: no ${_quazip_pkg}* directories found in: ${DA_INSTALL_LIB_CMAKE_PATH}")
                    endif()
                    list(SORT _da_quazip)
                    list(REVERSE _da_quazip)
                    list(GET _da_quazip 0 _da_quazip_dir)
                    message(STATUS "  |-try to find ${_quazip_pkg} in ${_da_quazip_dir}")
                    find_package(${_quazip_pkg} PATHS ${_da_quazip_dir} NO_DEFAULT_PATH REQUIRED)
                endif()
            endif()
            target_link_libraries(${_target} ${_scope} QuaZip::QuaZip)
            message(STATUS "  |-${_target} link QuaZip::QuaZip (${_scope})")
        elseif(_lib STREQUAL "python" OR _lib STREQUAL "Python")
            # 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
            find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
            message(STATUS "  |-find python ${Python3_VERSION} at ${Python3_ROOT_DIR}")
            target_link_libraries(${_target} ${_scope} ${Python3_LIBRARIES})
            target_include_directories(${_target} ${_scope} ${Python3_INCLUDE_DIRS})
        elseif(_lib STREQUAL "pybind11")
            # pybind11 是 header only，安装位置在 share/cmake 而非 lib/cmake
            _da_3rdparty_find_in_sharecmake(pybind11 "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} pybind11::headers)
            message(STATUS "  |-${_target} link pybind11::headers (${_scope})")
        elseif(_lib STREQUAL "orderedmap" OR _lib STREQUAL "tsl-ordered-map")
            # tsl-ordered-map 的包名为 tsl-ordered-map，库命名为 ordered_map，
            # 安装位置在 share/cmake，无法走通用查找路径
            _da_3rdparty_find_in_sharecmake(tsl-ordered-map "${_da_INSTALL_DIR}")
            target_link_libraries(${_target} ${_scope} tsl::ordered_map)
            message(STATUS "  |-${_target} link tsl::ordered_map (${_scope})")
        else()
            message(FATAL_ERROR "da_link_3rdparty(${_target}): unknown 3rdparty lib '${_lib}', "
                "supported: SARibbonBar DALiteCtk DAWidgets ads qwt spdlog quazip python pybind11 orderedmap")
        endif()
    endforeach()
endfunction()

# 预查找全部第三方包（仅创建 IMPORTED 目标，不链接到任何 target）。
# 供插件 standalone 构建使用：DAWorkbenchTargets.cmake 的导出目标 INTERFACE 依赖
# pybind11::headers / qwt::plot / ads::... 等第三方目标，必须先存在才能 include。
function(da_link_3rdparty_find_all)
    # 常规 lib/cmake 包
    foreach(_pkg SARibbonBar DALiteCtk DAWidgets spdlog qwt)
        _da_3rdparty_find_in_libcmake(${_pkg} "")
    endforeach()
    # share/cmake 包（包名与逻辑名不同的在此映射）
    _da_3rdparty_find_in_sharecmake(pybind11 "")
    _da_3rdparty_find_in_sharecmake(tsl-ordered-map "")
    # ADS：新旧包名回退
    set(_ads_new qtadvanceddocking-qt${QT_VERSION_MAJOR})
    find_package(${_ads_new} CONFIG QUIET)
    if(NOT ${_ads_new}_FOUND)
        _da_3rdparty_find_in_libcmake(qt${QT_VERSION_MAJOR}advanceddocking "")
    endif()
    # quazip
    find_package(QuaZip-Qt${QT_VERSION_MAJOR} QUIET)
    if(NOT QuaZip-Qt${QT_VERSION_MAJOR}_FOUND)
        set(_quazip_pkg QuaZip-Qt${QT_VERSION_MAJOR})
        file(GLOB _da_quazip LIST_DIRECTORIES true "${DA_INSTALL_LIB_CMAKE_PATH}/${_quazip_pkg}*")
        if(_da_quazip)
            list(SORT _da_quazip)
            list(REVERSE _da_quazip)
            list(GET _da_quazip 0 _da_quazip_dir)
            find_package(${_quazip_pkg} PATHS ${_da_quazip_dir} NO_DEFAULT_PATH QUIET)
        endif()
    endif()
    # python（Interpreter + Development）
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
endfunction()

