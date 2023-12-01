
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyBindQt 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAPyBindQt"
    "qt wrapper python | https://github.com/czyt1988"
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
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

# 此处要依赖python
# https://zhuanlan.zhihu.com/p/666367728
# https://blog.csdn.net/weixin_40448140/article/details/112005184
# 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
if(${Python3_FOUND})
    message(STATUS "  |-find python")
    message(STATUS "  | |-include dir:${Python3_INCLUDE_DIRS}")
    message(STATUS "  | |-libs : ${Python3_LIBRARIES}")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC ${Python3_LIBRARIES})
target_include_directories(${DA_LIB_NAME} PUBLIC ${Python3_INCLUDE_DIRS})

# DAPyBindQt依赖pybind11
# pybind11是header only
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/pybind11>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/pybind11/pybind11/include>
)
message(STATUS "  |-pybind11 include: ${pybind11_DIR}/include")

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
#    ARCHIVE_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    LIBRARY_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    RUNTIME_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/bin"
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
#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)
