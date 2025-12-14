
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyBindQt 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAPyBindQt"
    "qt wrapper python | https://github.com/czyt1988"
    0
    0
    2
)

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    REQUIRED
)


########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_DAPYBINDQT_PANDAS_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/pandas/*.h")
file(GLOB DA_DAPYBINDQT_PANDAS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/pandas/*.cpp")
file(GLOB DA_DAPYBINDQT_NUMPY_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/numpy/*.h")
file(GLOB DA_DAPYBINDQT_NUMPY_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/numpy/*.cpp")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_DAPYBINDQT_PANDAS_HEADER_FILES}
            ${DA_DAPYBINDQT_PANDAS_SOURCE_FILES}
            ${DA_DAPYBINDQT_NUMPY_HEADER_FILES}
            ${DA_DAPYBINDQT_NUMPY_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏,必须为Private
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAPYBINDQT_BUILDLIB)#!!!!** 注意变更 **!!!!

# 定义别名让DAWorkbench::${DA_LIB_NAME}也能获取到
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})

########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

# DA相关
find_package(${DA_PROJECT_NAME} COMPONENTS
    DAUtils
)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAUtils")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
)


# -------------link 3rdparty python--------------------------
damacro_import_Python(${DA_LIB_NAME})

# -------------link 3rdparty pybind11--------------------------
damacro_import_pybind11(${DA_LIB_NAME})

########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
install(FILES
    ${DA_DAPYBINDQT_PANDAS_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/pandas
    COMPONENT headers
)
install(FILES
    ${DA_DAPYBINDQT_NUMPY_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/numpy
    COMPONENT headers
)

