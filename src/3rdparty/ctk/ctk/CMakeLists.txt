
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# lite ctk 的 Cmake文件

cmake_minimum_required(VERSION 3.5)

########################################################
# 版本定义
########################################################
set(DACTK_VERSION_MAJOR 0)
set(DACTK_VERSION_MINOR 0)
set(DACTK_VERSION_PATCH 2)
set(DACTK_VERSION "${DACTK_VERSION_MAJOR}.${DACTK_VERSION_MINOR}.${DACTK_VERSION_PATCH}")
message(STATUS "DA Version is ${DACTK_VERSION}")
########################################################
# 定义工程
########################################################
set(DACTK_LIB_NAME "DALiteCtk")

project(${DACTK_LIB_NAME} 
        VERSION ${DACTK_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : A Workflow Integration Workbench"
        )


########################################################
# 一些常规设置
########################################################


set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# c++标准，要求为17
set(CMAKE_CXX_STANDARD 17)
# 强制要求
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # make sure __cplusplus is defined when using msvc and enable parallel build
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()

########################################################
# Qt
########################################################
set(DACTK_MIN_QT_VERSION 5.6)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DACTK_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED
)


########################################################
# 目录包含
########################################################
# 包含自身目录
set(CMAKE_INCLUDE_CURRENT_DIR ON)

########################################################
# 文件加载 
########################################################
file(GLOB DACTK_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DACTK_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${DACTK_LIB_NAME} SHARED
            ${DACTK_LIB_HEADER_FILES}
            ${DACTK_LIB_SOURCE_FILES}
)

# 构建库定义的宏
target_compile_definitions(${DACTK_LIB_NAME} PRIVATE MAKE_CTK_LIB)#!!!!** 注意变更 **!!!!
target_compile_definitions(${DACTK_LIB_NAME} PRIVATE CTK_SHARED)#!!!!** 注意变更 **!!!!

target_link_libraries(${DACTK_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
if(${QT_VERSION_MAJOR} EQUAL 6)
   find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
   target_link_libraries(${DACTK_LIB_NAME} PRIVATE Qt6::Core5Compat)
endif()
########################################################
# Qt的moc
########################################################
set_target_properties(${DACTK_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DACTK_VERSION}
    EXPORT_NAME ${DACTK_LIB_NAME}
)


##################################
# install
##################################
# xxxConfig.cmake.in中，会让此变量和“${PACKAGE_PREFIX_DIR}/”进行拼接，也就是${PACKAGE_PREFIX_DIR}/@DA_LIB_INCLUDE_INSTALL_DIR@
# PACKAGE_PREFIX_DIR等于${CMAKE_CURRENT_LIST_DIR}/../..
# 最终变为${CMAKE_CURRENT_LIST_DIR}/../../include/${DACTK_LIB_NAME}
# 注意此处是替换，不要出现引号

set(DA_LIB_INCLUDE_INSTALL_DIR include/${DACTK_LIB_NAME})
# Generate library version info which will generate xxxConfigVersion.cmake,
# the ${PACKAGE_VERSION} is the version defined in project()
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${DACTK_LIB_NAME}ConfigVersion.cmake
    VERSION ${DACTK_LIB_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${DACTK_LIB_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${DACTK_LIB_NAME}Config.cmake"
  INSTALL_DESTINATION lib/cmake/${DACTK_LIB_NAME}
  PATH_VARS DA_LIB_INCLUDE_INSTALL_DIR
)

# 声明导出target的名称
install(TARGETS ${DACTK_LIB_NAME}
    EXPORT "${DACTK_LIB_NAME}Targets"
    INCLUDES DESTINATION include/${DACTK_LIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(EXPORT "${DACTK_LIB_NAME}Targets"
    FILE ${DACTK_LIB_NAME}Targets.cmake # 导出的文件基准名。
    DESTINATION lib/cmake/${DACTK_LIB_NAME}
)


install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DACTK_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${DACTK_LIB_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${DACTK_LIB_NAME}
)
install(FILES
    ${DACTK_LIB_HEADER_FILES}
    DESTINATION include/${DACTK_LIB_NAME}
    COMPONENT headers
)

target_include_directories(${DACTK_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DACTK_LIB_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

export(EXPORT ${DACTK_LIB_NAME}Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${DACTK_LIB_NAME}Targets.cmake
)
