
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAUtils 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAUtils"
    "DA Utils Lib | https://github.com/czyt1988"
    0
    0
    3
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
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Qt6::Core5Compat)
endif()

########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

########################################################
# 附加编译选项
########################################################
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    # 这里是为 MSVC 编译器设置的选项
    target_link_options(${DA_LIB_NAME} PRIVATE /VERBOSE)
    # 或者设置其他 MSVC 特定的编译或链接选项
else()
    # 这里是为非 MSVC 编译器设置的选项（如果有的话）
endif()
########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

