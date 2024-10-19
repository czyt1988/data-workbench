macro(damacro_set_bin_name _var)
    set(DA_MIN_QT_VERSION 5.14)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
    ########################################################
    # 平台判断
    ########################################################
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(_platform_name "x86")
    else()
        set(_platform_name "x64")
    endif()
    ########################################################
    # 安装路径设置
    ########################################################
    set(_var bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${_platform_name})
endmacro(damacro_set_bin_name)

# 定义DA_PLUGIN*的宏
# _plugin_name lib的名字，决定变量DA_PLUGIN_NAME
# _plugin_description lib的描述，决定变量DA_PLUGIN_DESCRIPTION
# _plugin_ver_major lib的主版本号，决定变量DA_PLUGIN_VERSION_MAJOR
# _plugin_ver_minor lib的次版本号，决定变量DA_PLUGIN_VERSION_MINOR
# _plugin_ver_path lib的末版本号，决定变量DA_PLUGIN_VERSION_PATCH
# 生成：DA_PLUGIN_VERSION，完整的版本名
# 生成：DA_PLUGIN_FULL_DESCRIPTION，完整的描述
# 生成：DA_MIN_QT_VERSION 最低qt版本要求
macro(damacro_plugin_setting _plugin_name _plugin_description _plugin_ver_major _plugin_ver_minor _plugin_ver_path _daworkbench_intall_dir)
    set(DA_MIN_QT_VERSION 5.14)
	set(DA_PLUGIN_NAME ${_plugin_name})
    set(DA_PLUGIN_DESCRIPTION ${_plugin_description})
    set(DA_PLUGIN_VERSION_MAJOR ${_plugin_ver_major})
    set(DA_PLUGIN_VERSION_MINOR ${_plugin_ver_minor})
    set(DA_PLUGIN_VERSION_PATCH ${_plugin_ver_path})
    set(DA_PLUGIN_VERSION "${DA_PLUGIN_VERSION_MAJOR}.${DA_PLUGIN_VERSION_MINOR}.${DA_PLUGIN_VERSION_PATCH}")
    set(DA_PLUGIN_FULL_DESCRIPTION "${DA_PLUGIN_NAME} ${DA_PLUGIN_VERSION} | ${DA_PLUGIN_DESCRIPTION}")

    project(${DA_PLUGIN_NAME} 
        VERSION ${DA_PLUGIN_VERSION} 
        LANGUAGES CXX
        DESCRIPTION ${DA_PLUGIN_FULL_DESCRIPTION}
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
    set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")
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
    # 插件的安装路径和DAWorkbench一致，这样才能正确把插件安装
    set(CMAKE_INSTALL_PREFIX "${_daworkbench_intall_dir}")
    ########################################################
    # 打印信息
    ########################################################
    message("")
    message("${DA_PLUGIN_FULL_DESCRIPTION}")
    message(STATUS "  | => DAWorkBench Install Dir=${_daworkbench_intall_dir}")
    message(STATUS "  | => DA_PLUGIN_NAME=${DA_PLUGIN_NAME}")
    message(STATUS "  | => PLUGIN CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  | => PLUGIN CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  | => PLUGIN CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(damacro_plugin_setting)


# 定义DA_PLUGIN*的宏
# _plugin_name lib的名字，决定变量DA_PLUGIN_NAME
# _plugin_description lib的描述，决定变量DA_PLUGIN_DESCRIPTION
# _plugin_ver_major lib的主版本号，决定变量DA_PLUGIN_VERSION_MAJOR
# _plugin_ver_minor lib的次版本号，决定变量DA_PLUGIN_VERSION_MINOR
# _plugin_ver_path lib的末版本号，决定变量DA_PLUGIN_VERSION_PATCH
# 生成：DA_PLUGIN_VERSION，完整的版本名
# 生成：DA_PLUGIN_FULL_DESCRIPTION，完整的描述
# 生成：DA_MIN_QT_VERSION 最低qt版本要求
macro(_damacro_plugin_setting _plugin_name _plugin_description _plugin_ver_major _plugin_ver_minor _plugin_ver_path)
    set(DA_MIN_QT_VERSION 5.14)
    set(DA_PLUGIN_NAME ${_plugin_name})
    set(DA_PLUGIN_DESCRIPTION ${_plugin_description})
    set(DA_PLUGIN_VERSION_MAJOR ${_plugin_ver_major})
    set(DA_PLUGIN_VERSION_MINOR ${_plugin_ver_minor})
    set(DA_PLUGIN_VERSION_PATCH ${_plugin_ver_path})
    set(DA_PLUGIN_VERSION "${DA_PLUGIN_VERSION_MAJOR}.${DA_PLUGIN_VERSION_MINOR}.${DA_PLUGIN_VERSION_PATCH}")
    set(DA_PLUGIN_FULL_DESCRIPTION "${DA_PLUGIN_NAME} ${DA_PLUGIN_VERSION} | ${DA_PLUGIN_DESCRIPTION}")

    project(${DA_PLUGIN_NAME}
        VERSION ${DA_PLUGIN_VERSION}
        LANGUAGES CXX
        DESCRIPTION ${DA_PLUGIN_FULL_DESCRIPTION}
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
    set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
    set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")
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
    # 插件的安装路径和DAWorkbench一致，这样才能正确把插件安装
    ########################################################
    # 打印信息
    ########################################################
    message("")
    message("${DA_PLUGIN_FULL_DESCRIPTION}")
    message(STATUS "  | => DA_PLUGIN_NAME=${DA_PLUGIN_NAME}")
    message(STATUS "  | => PLUGIN CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  | => PLUGIN CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  | => PLUGIN CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(_damacro_plugin_setting)

# 获取插件的runtime所在目录，也就是dll放置的位置
macro(damacro_get_plugin_runtime_install_dir _var_plugin_install_dir _daworkbench_install_root_dir)
    ########################################################
    # 获取插件的安装目录
    ########################################################
    # 声明导出target的名称
    set(_var_plugin_install_dir ${_daworkbench_install_root_dir}/bin/plugins)
    message(STATUS "${DA_PLUGIN_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_get_plugin_runtime_install_dir)

# 通用的安装
macro(damacro_plugin_install)
    ########################################################
    # dll资源信息添加到 target_sources中
    ########################################################
    # 声明导出target的名称
    install(TARGETS ${DA_PLUGIN_NAME}
        RUNTIME DESTINATION bin/plugins
    )
    message(STATUS "${DA_PLUGIN_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_plugin_install)


# damacro_import_SARibbonBar(${DA_LIB_NAME})
macro(damacro_import_SARibbonBar __target_name __install_dir)
    find_package(SARibbonBar PATHS ${__install_dir})
    if(SARibbonBar_FOUND)
        message(STATUS "  |-link SARibbonBar")
        message(STATUS "  | |-include dir:${SARibbonBar_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC
        SARibbonBar::SARibbonBar
    )
endmacro(damacro_import_SARibbonBar)


macro(damacro_import_DALiteCtk __target_name __install_dir)
    find_package(DALiteCtk PATHS ${__install_dir})
    if(DALiteCtk_FOUND)
        message(STATUS "  |-link DALiteCtk")
        message(STATUS "  | |-include dir:${DALiteCtk_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC
        DALiteCtk
    )
endmacro(damacro_import_DALiteCtk)


macro(damacro_import_QtAdvancedDocking __target_name __install_dir)
    find_package(qt${QT_VERSION_MAJOR}advanceddocking PATHS ${__install_dir})
    if(qt${QT_VERSION_MAJOR}advanceddocking_FOUND)
        message(STATUS "  |-link qt${QT_VERSION_MAJOR}advanceddocking")
        message(STATUS "  | |-include dir:${qt${QT_VERSION_MAJOR}advanceddocking_INCLUDE_DIR}")
    endif()
    target_link_libraries(${__target_name} PUBLIC
        ads::qt${QT_VERSION_MAJOR}advanceddocking
    )
endmacro(damacro_import_QtAdvancedDocking)


macro(damacro_import_qwt __target_name __install_dir)
    # 3rdparty - qwt
    find_package(qwt PATHS ${__install_dir})
    if(qwt_FOUND)
        message(STATUS "  |-link qwt")
        message(STATUS "  | |-include dir:${qwt_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC
        qwt
    )
endmacro(damacro_import_qwt)

macro(damacro_import_QtPropertyBrowser __target_name __install_dir)
    # 3rdparty - QtPropertyBrowser
    find_package(QtPropertyBrowser PATHS ${__install_dir})
    if(QtPropertyBrowser_FOUND)
        message(STATUS "  |-link QtPropertyBrowser")
        message(STATUS "  | |-include dir:${QtPropertyBrowser_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC
        QtPropertyBrowser
    )
endmacro(damacro_import_QtPropertyBrowser)

macro(damacro_import_spdlog __target_name __install_dir)
    # 3rdparty - spdlog
    find_package(spdlog)
    if(spdlog_FOUND)
        message(STATUS "  |-link spdlog")
        message(STATUS "  | |-include dir:${spdlog_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC spdlog::spdlog)
endmacro(damacro_import_spdlog)

macro(damacro_import_Python __target_name)
    # Python
    # https://zhuanlan.zhihu.com/p/666367728
    # https://blog.csdn.net/weixin_40448140/article/details/112005184
    # 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    if(${Python3_FOUND})
        message(STATUS "  |-find python")
        message(STATUS "  | |-include dir:${Python3_INCLUDE_DIRS}")
        message(STATUS "  | |-libs : ${Python3_LIBRARIES}")
    endif()
    target_link_libraries(${__target_name} PUBLIC ${Python3_LIBRARIES})
    target_include_directories(${__target_name} PUBLIC ${Python3_INCLUDE_DIRS})
endmacro(damacro_import_Python)

macro(damacro_import_orderedmap __target_name __install_dir)
    # 3rdparty - orderedmap
    find_package(tsl-ordered-map)
    if(tsl-ordered-map_FOUND)
        message(STATUS "  |-link tsl-ordered-map")
        message(STATUS "  | |-include dir:${tsl-ordered-map_INCLUDE_DIR}")
    endif()
    # 链接的第三方库
    target_link_libraries(${__target_name} PUBLIC tsl::ordered_map)
endmacro(damacro_import_orderedmap)

