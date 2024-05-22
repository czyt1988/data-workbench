
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAAxOfficeWrapper 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAAxOfficeWrapper"
    "office ax container qt wrapper | https://github.com/czyt1988"
    0
    0
    1
)



########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    AxContainer
    REQUIRED
)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
add_library(DAWorkbench::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAAXOFFICEWRAPPER_BUILDLIB)
########################################################
# 依赖链接
########################################################
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::AxContainer
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

# DAUtils
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到DAWorkbench
set(${DA_PROJECT_NAME}_DIR "${CMAKE_BINARY_DIR}")
find_package(${DA_PROJECT_NAME} COMPONENTS DAUtils)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link DAUtils")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAUtils
)

########################################################
# Qt的moc
########################################################
set_target_properties(${DA_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_LIB_VERSION}
    EXPORT_NAME ${DA_LIB_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

