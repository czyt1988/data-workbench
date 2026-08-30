
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAInterface 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
set(DA_LIB_HEADER_FILES
    DAActionsInterface.h
    DABaseInterface.h
    DACommandInterface.h
    DACoreInterface.h
    DADataManagerInterface.h
    DADockingAreaInterface.h
    DAStatusBarInterface.h
    DAInterfaceAPI.h
    DAProjectInterface.h
    DARibbonAreaInterface.h
    DAUIExtendInterface.h
    DAUIInterface.h
    DAInterfaceHelper.h
)
set(DA_LIB_SOURCE_FILES
    DAActionsInterface.cpp
    DABaseInterface.cpp
    DACommandInterface.cpp
    DACoreInterface.cpp
    DADataManagerInterface.cpp
    DADockingAreaInterface.cpp
    DAStatusBarInterface.cpp
    DAProjectInterface.cpp
    DARibbonAreaInterface.cpp
    DAUIExtendInterface.cpp
    DAUIInterface.cpp
    DAInterfaceHelper.cpp
)
list(APPEND DA_LIB_HEADER_FILES
    DAInterfacePythonBinding.h
    DAQwtPyPlotPythonBinding.h
)
list(APPEND DA_LIB_SOURCE_FILES
    DAInterfacePythonBinding.cpp
    DAQwtPyPlotPythonBinding.cpp
)

da_add_library(
    NAME DAInterface
    BUILD_DEFINE DAINTERFACE_BUILDLIB
    DESCRIPTION "Interface Of DAWorkbench | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PRIVATE Core Gui Widgets Xml
    LINK_PUBLIC DAGui
    LINK_PRIVATE DAMessageHandler
    THIRDPARTY SARibbonBar ads qwt ctk python pybind11
    HEADERS ${DA_LIB_HEADER_FILES}
)
