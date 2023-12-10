
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAUtils 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAUtils"
    "DA Utils Lib | https://github.com/czyt1988"
    0
    0
    1
)

########################################################
# Qt
########################################################
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Xml
    Widgets
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


add_library(${DA_LIB_NAME} SHARED
        ${DA_LIB_HEADER_FILES}
        ${DA_LIB_SOURCE_FILES}
        ${DA_GLOBAL_HEADER}
)	
# 定义别名让DAWorkbench::${DA_LIB_NAME}也能获取到
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
# 构建库定义的宏,必须为Private
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAUTILS_BUILD)
########################################################
# 依赖链接
########################################################
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Qt6::Core5Compat)
endif()
########################################################
# 设置target属性，以及Qt的moc
########################################################
# 在同一个 CMake build tree 中，每个项目的 targets 对外都是可见的
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
    ${DA_LIB_HPP_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)

