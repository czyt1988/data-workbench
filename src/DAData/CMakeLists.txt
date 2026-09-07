
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAData 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
set(DA_LIB_HEADER_FILES
    DAAbstractData.h
    DAData.h
    DADataAPI.h
    DADataManager.h
    DACommandsDataManager.h
    DADataEnumStringUtils.h
    # 表格数据源抽象（头文件不依赖Python）
    DATableDataBlock.h
    DATableDataSource.h
    DADataFactory.h
)
set(DA_LIB_SOURCE_FILES
    DAAbstractData.cpp
    DAData.cpp
    DADataManager.cpp
    DACommandsDataManager.cpp
    DADataEnumStringUtils.cpp
    DATableDataBlock.cpp
    DATableDataSource.cpp
    DADataFactory.cpp
)
list(APPEND DA_LIB_HEADER_FILES
    DADataPythonBinding.h
    DADataPyDataFrame.h
    DADataPyObject.h
    DADataPySeries.h
    DADataUndoCommand.h
)
list(APPEND DA_LIB_SOURCE_FILES
    DADataPythonBinding.cpp
    DADataPyDataFrame.cpp
    DADataPyObject.cpp
    DADataPySeries.cpp
    DADataUndoCommand.cpp
)

da_add_library(
    NAME DAData
    BUILD_DEFINE DADATA_BUILDLIB
    DESCRIPTION "Data Lib For DA | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets
    # DAData 的头文件包含了 DAPyBindQt/DAPyScripts 的内容，需公开链接
    LINK_PUBLIC DAPyBindQt DAPyScripts
    LINK_PRIVATE DAUtils DAMessageHandler
    THIRDPARTY python pybind11
    HEADERS ${DA_LIB_HEADER_FILES}
)
