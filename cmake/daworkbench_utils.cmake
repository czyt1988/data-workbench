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
    set(DA_LIB_DESCRIPTION "DA Utils Lib | https://github.com/czyt1988")
    set(DA_LIB_VERSION_MAJOR 0)
    set(DA_LIB_VERSION_MINOR 0)
    set(DA_LIB_VERSION_PATCH 1)
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
    message("${DA_LIB_FULL_DESCRIPTION}")
    message(STATUS " | => DA_LIB_NAME=${DA_LIB_NAME}")
    message(STATUS " | => DA_GLOBAL_HEADER=${DA_GLOBAL_HEADER}")
    message(STATUS " | => CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    message(STATUS " | => CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS " | => CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endmacro(damacro_lib_setting)

# 通用的安装
macro(damacro_lib_install)
    ########################################################
    # 目标依赖目录
    ########################################################
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
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
    ########################################################
    # dll资源信息
    ########################################################
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/create_win32_resource_version.cmake)
    if(WIN32)
            create_win32_resource_version(
                    TARGET ${DA_LIB_NAME}
                    FILENAME ${DA_LIB_NAME}
                    VERSION ${DA_LIB_VERSION}
                    EXT "dll"
                    COMPANYNAME "DA"
                    COPYRIGHT "czy"
                    DESCRIPTION ${DA_LIB_DESCRIPTION}
            )
    endif()

    message(STATUS "${DA_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")
endmacro(damacro_lib_install)