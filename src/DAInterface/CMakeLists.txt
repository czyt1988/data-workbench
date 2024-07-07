
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAInterface"
    "Interface Of DAWorkbench | https://github.com/czyt1988"
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
    REQUIRED
)


########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAINTERFACE_BUILDLIB)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
)




# -------------link SARibbon--------------------------
damacro_import_SARibbonBar(${DA_LIB_NAME})

# -------------link QtAdvancedDockingSystem--------------------------
damacro_import_QtAdvancedDocking(${DA_LIB_NAME})

# -------------link qwt--------------------------
damacro_import_qwt(${DA_LIB_NAME})

# -------------link ctk--------------------------
damacro_import_DALiteCtk(${DA_LIB_NAME})

if(DA_ENABLE_PYTHON)
    # -------------link python--------------------------
    damacro_import_Python(${DA_LIB_NAME})
endif()
# -------------link DAWorkBench--------------------------
# DAGui
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
# find_package(${DA_PROJECT_NAME} COMPONENTS
#     DAUtils
#     DAMessageHandler
#     DAData
#     DACommonWidgets
#     DAGraphicsView
#     DAWorkFlow
#     DAFigure
#     DAGui
# )
# target_link_libraries(${DA_LIB_NAME} PUBLIC
#     ${DA_PROJECT_NAME}::DAUtils
#     ${DA_PROJECT_NAME}::DAMessageHandler
#     ${DA_PROJECT_NAME}::DAData
#     ${DA_PROJECT_NAME}::DACommonWidgets
#     ${DA_PROJECT_NAME}::DAGraphicsView
#     ${DA_PROJECT_NAME}::DAWorkFlow
#     ${DA_PROJECT_NAME}::DAFigure
#     ${DA_PROJECT_NAME}::DAGui
# )
# python相关依赖
# if(DA_ENABLE_PYTHON)
#     find_package(${DA_PROJECT_NAME} COMPONENTS
#         DAPyBindQt
#         DAPyScripts
#         DAPyCommonWidgets
#     )
#     target_link_libraries(${DA_LIB_NAME} PRIVATE
#         ${DA_PROJECT_NAME}::DAPyBindQt
#         ${DA_PROJECT_NAME}::DAPyScripts
#         ${DA_PROJECT_NAME}::DAPyCommonWidgets
#     )
#     # -------------link python--------------------------
#     damacro_import_Python(${DA_LIB_NAME})
# endif()

find_package(${DA_PROJECT_NAME} COMPONENTS
    DAGui
)
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAGui
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
