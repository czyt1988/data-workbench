# 这是对python和Qt的封装，DA的python相关类都需要依赖此类

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include($$PWD/../common.pri)
include($$PWD/../function.pri)
# DAPyScripts 自动引入
#include($${DA_SRC_DIR}/python_lib.pri) # 引入python环境
#include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri) # 引入pybind11
#include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
# da utility
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
TEMPLATE = lib
CONFIG +=  c++14
DEFINES += DADATA_BUILDLIB
TARGET = $$saLibNameMake(DAData)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAAbstractData.h \
    DAData.h \
    DADataAPI.h \
    DADataManager.h \
    DADataPackage.h \
    DADataPyDataFrame.h \
    DADataPyObject.h  \
    DACommandsDataManager.h \
    DADataPySeries.h

SOURCES += \
    DAAbstractData.cpp \
    DAData.cpp \
    DADataManager.cpp \
    DADataPackage.cpp \
    DADataPyDataFrame.cpp \
    DADataPyObject.cpp  \
    DACommandsDataManager.cpp \
    DADataPySeries.cpp






