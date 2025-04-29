
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAData 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAData"
    "Data Lib For DA | https://github.com/czyt1988"
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
    Gui
    Widgets
    REQUIRED
)

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
set(DA_LIB_HEADER_FILES
    DAAbstractData.h
    DAData.h
    DADataAPI.h
    DADataManager.h
    DADataPackage.h
    DACommandsDataManager.h
)
set(DA_LIB_SOURCE_FILES
    DAAbstractData.cpp
    DAData.cpp
    DADataManager.cpp
    DADataPackage.cpp
    DACommandsDataManager.cpp
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_LIB_HEADER_FILES
        DADataPyDataFrame.h
        DADataPyObject.h
        DADataPySeries.h
    )
    list(APPEND DA_LIB_SOURCE_FILES
        DADataPyDataFrame.cpp
        DADataPyObject.cpp
        DADataPySeries.cpp
    )
endif()

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DADATA_BUILDLIB)#!!!!** 注意变更 **!!!!
if(DA_ENABLE_PYTHON)
    #此宏一定要显示声明为public让调用者知道
    target_compile_definitions(${DA_LIB_NAME} PUBLIC -DDA_ENABLE_PYTHON=1)
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
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

if(DA_ENABLE_PYTHON)
    # -------------link Python--------------------------
    damacro_import_Python(${DA_LIB_NAME})

    # -------------link pybind11--------------------------
    damacro_import_pybind11(${DA_LIB_NAME})
    message(STATUS "  |-pybind11 include: ${pybind11_DIR}/include")
endif()
# -------------link DAWorkBench--------------------------
# DAUtils DAPyBindQt DAPyScripts
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到DAWorkbench
find_package(${DA_PROJECT_NAME} COMPONENTS DAUtils)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAUtils")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
)

if(DA_ENABLE_PYTHON)
    find_package(${DA_PROJECT_NAME} COMPONENTS
        DAPyBindQt
        DAPyScripts
    )
    if(${DA_PROJECT_NAME}_FOUND)
        message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAPyBindQt")
        message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAPyScripts")
    endif()
    # DAData的头文件包含了DAPyBindQt库的内容，因此DAPyBindQt需要公开链接
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        ${DA_PROJECT_NAME}::DAPyBindQt
        ${DA_PROJECT_NAME}::DAPyScripts
    )
    include_directories(${DAPyBindQt_INCLUDE_DIR})
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyBindQt")
    message(STATUS "  | |-include dir:${DAPyBindQt_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyScripts")
    message(STATUS "  | |-include dir:${DAPyScripts_INCLUDE_DIR}")
endif()

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

