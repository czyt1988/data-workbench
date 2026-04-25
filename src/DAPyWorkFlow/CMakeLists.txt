
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyWorkFlow 库

cmake_minimum_required(VERSION 3.10)
damacro_lib_setting(
    "DAPyWorkFlow"
    "DA WorkFlow Python Binding Lib | https://github.com/czyt1988"
    0
    0
    1
)


########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_PYTHONBINDING_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/PythonBinding/*.h")
file(GLOB DA_LIB_PYTHONBINDING_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/PythonBinding/*.cpp")
add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_PYTHONBINDING_HEADER_FILES}
            ${DA_LIB_PYTHONBINDING_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAPYWORKFLOW_BUILDLIB)#!!!!** 注意变更 **!!!!
########################################################
# 添加预定义宏NOMINMAX,windows系统会定义max和min宏，这个宏和std::max冲突
########################################################
if(WIN32)
    target_compile_definitions(${DA_LIB_NAME} PRIVATE -DNOMINMAX)
endif()

add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
# -------------link DAWorkbench--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAGraphicsView
    ${DA_PROJECT_NAME}::DAPyBindQt
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
    ${DA_LIB_PYTHONBINDING_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/PythonBinding
    COMPONENT headers
)