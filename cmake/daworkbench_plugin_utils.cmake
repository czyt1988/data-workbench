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
macro(damacro_plugin_setting _plugin_name _plugin_description _plugin_ver_major _plugin_ver_minor _plugin_ver_path _daworkbench_intall_cmake_dir)
    set(DA_MIN_QT_VERSION 5.14)
	set(DA_PLUGIN_NAME ${_plugin_name})
    set(DA_PLUGIN_DESCRIPTION ${_plugin_description})
    set(DA_PLUGIN_VERSION_MAJOR ${_plugin_ver_major})
    set(DA_PLUGIN_VERSION_MINOR ${_plugin_ver_minor})
    set(DA_PLUGIN_VERSION_PATCH ${_plugin_ver_path})
    set(DA_PLUGIN_VERSION "${DA_PLUGIN_VERSION_MAJOR}.${DA_PLUGIN_VERSION_MINOR}.${DA_PLUGIN_VERSION_PATCH}")
    set(DA_PLUGIN_FULL_DESCRIPTION "${DA_PLUGIN_NAME} ${DA_PLUGIN_VERSION} | ${DA_PLUGIN_DESCRIPTION}")
    set(DAWorkbench_DIR ${_daworkbench_intall_cmake_dir})
    get_filename_component(DAWORKBENCH_INSTALL_DIR "${_daworkbench_intall_cmake_dir}/../../../" ABSOLUTE)

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
    damacro_set_bin_name(DA_BIN_DIR_NAME)
    set(CMAKE_INSTALL_PREFIX "${DAWORKBENCH_INSTALL_DIR}/${DA_BIN_DIR_NAME}/plugins")
    ########################################################
    # 打印信息
    ########################################################
    message("")
    message("${DA_PLUGIN_FULL_DESCRIPTION}")
    message(STATUS "  | => DA_PLUGIN_NAME=${DA_PLUGIN_NAME}")
    message(STATUS "  | => DA_GLOBAL_HEADER=${DA_GLOBAL_HEADER}")
    message(STATUS "  | => CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  | => CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  | => CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(damacro_plugin_setting)

# 通用的安装
macro(damacro_plugin_install)
    ########################################################
    # dll资源信息添加到 target_sources中
    ########################################################


    # 声明导出target的名称
    install(TARGETS ${DA_PLUGIN_NAME}
        RUNTIME DESTINATION .
    )

    message(STATUS "${DA_PLUGIN_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_plugin_install)

