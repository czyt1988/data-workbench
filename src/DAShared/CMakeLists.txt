
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAShared 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAShared"
    "DA DAShared Lib | https://github.com/czyt1988"
    0
    0
    1
)

########################################################
# Qt
########################################################
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# Build the headers-only target
add_library(${DA_LIB_NAME} INTERFACE)
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
# 构建库定义的宏,必须为Private
target_compile_definitions(${DA_LIB_NAME} INTERFACE DASHARED_BUILD)


# 设置HEADER_ONLY属性
set_target_properties(${DA_LIB_NAME} PROPERTIES HEADER_ONLY ON)
########################################################
# header only lib 只需定义include路径即可
########################################################
target_include_directories(${DA_LIB_NAME} INTERFACE
        $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    )

include(CMakePackageConfigHelpers)
# Generate library version info which will generate xxxConfigVersion.cmake,
# the ${PACKAGE_VERSION} is the version defined in project()
# DALibConfig.cmake.in中，会让此变量和“${PACKAGE_PREFIX_DIR}/”进行拼接，也就是${PACKAGE_PREFIX_DIR}/@DA_LIB_INCLUDE_INSTALL_DIR@
# PACKAGE_PREFIX_DIR = ${CMAKE_CURRENT_LIST_DIR}/../..
# 最终变为${CMAKE_CURRENT_LIST_DIR}/../../include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
set(DA_LIB_INCLUDE_INSTALL_DIR include/${DA_PROJECT_NAME}/${DA_LIB_NAME})

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
install(FILES
    ${DA_LIB_HEADER_FILES}
    ${DA_LIB_HPP_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
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
