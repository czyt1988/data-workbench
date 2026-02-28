
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
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Qt6::Core5Compat)
endif()

# MinGW 7 需要手动链接 filesystem 库
if(MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    target_link_libraries(DAUtils PRIVATE stdc++fs)
endif()

########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

########################################################
# 安装
########################################################
damacro_lib_install()
include(GNUInstallDirs)


install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

