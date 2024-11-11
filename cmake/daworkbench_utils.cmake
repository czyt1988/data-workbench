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
    # 安装路径设置 设置变量值，并传递到父作用域
    ########################################################
    set(${_var} "bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${_CXX_COMPILER_ID}_${_platform_name}" PARENT_SCOPE)
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


# 通用的安装
macro(damacro_lib_install_no_rc)
    target_compile_definitions(${DA_LIB_NAME} PRIVATE QT_MESSAGELOGCONTEXT)
    ########################################################
    # 目标依赖目录
    ########################################################
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    )
    # 这个主要是DAGlobal.h
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>
    )
    # 这个主要是DAShared
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/DAShared>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../DAShared>
    )
    # DALibConfig.cmake.in中，会让此变量和“${PACKAGE_PREFIX_DIR}/”进行拼接，也就是${PACKAGE_PREFIX_DIR}/@DA_LIB_INCLUDE_INSTALL_DIR@
    # PACKAGE_PREFIX_DIR = ${CMAKE_CURRENT_LIST_DIR}/../..
    # 最终变为${CMAKE_CURRENT_LIST_DIR}/../../include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    set(DA_LIB_INCLUDE_INSTALL_DIR include/${DA_PROJECT_NAME}/${DA_LIB_NAME})

    include(CMakePackageConfigHelpers)
    # Generate library version info which will generate xxxConfigVersion.cmake,
    # the ${PACKAGE_VERSION} is the version defined in project()
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake
        VERSION ${DA_LIB_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    configure_package_config_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/../DALibConfig.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
      INSTALL_DESTINATION lib/cmake/${DA_PROJECT_NAME}
      PATH_VARS DA_LIB_INCLUDE_INSTALL_DIR
    )
    # 声明导出target的名称
    install(TARGETS ${DA_LIB_NAME}
        EXPORT ${DA_LIB_NAME}Targets
        INCLUDES DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(EXPORT "${DA_LIB_NAME}Targets"
        FILE ${DA_LIB_NAME}Targets.cmake # 导出的文件基准名。
        NAMESPACE ${DA_PROJECT_NAME}::
        DESTINATION lib/cmake/${DA_PROJECT_NAME}
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake"
        DESTINATION lib/cmake/${DA_PROJECT_NAME}
    )

    export(EXPORT ${DA_LIB_NAME}Targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Targets.cmake
       NAMESPACE ${DA_PROJECT_NAME}::
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
    set(DA_APP_FULL_DESCRIPTION "${DA_APP_NAME} ${DA_APP_VERSION} | ${DA_APP_DESCRIPTION}")

    project(${DA_APP_NAME}
        VERSION ${DA_APP_VERSION}
        LANGUAGES CXX
        DESCRIPTION ${DA_APP_FULL_DESCRIPTION}
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
macro(damacro_app_install _icon_path)
    ########################################################
    # dll资源信息添加到 target_sources中
    ########################################################
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/create_win32_resource_version.cmake)
    if(WIN32)
        set(__rc_path "${CMAKE_CURRENT_BINARY_DIR}/${DA_APP_NAME}.rc")
        if(NOT EXISTS "${__rc_path}")
            generate_win32_rc_file(
                PATH "${__rc_path}"
                VERSION "${DA_APP_VERSION}"
                COMPANY "czy"
                DESCRIPTION ${DA_APP_FULL_DESCRIPTION}
                COPYRIGHT "LGPL License"
                PRODUCT ${DA_APP_NAME}
                ICONS ${_icon_path}
            )
        endif()
        if(EXISTS "${__rc_path}")
            target_sources(${DA_APP_NAME} PRIVATE "${__rc_path}")
        endif()
    endif()
    ########################################################
    # 目标依赖目录
    ########################################################
    # 声明导出target的名称
    install(TARGETS ${DA_APP_NAME} RUNTIME DESTINATION bin)
    message(STATUS "${DA_APP_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_app_install)

