# 这是对python 和数据分析相关的功能的Qt封装

QT = core
include($$PWD/../common.pri)
include($$PWD/../function.pri)
#DAPyBindQt自动引入
#include($$PWD/../python_lib.pri) #需要使用pybind11
#include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)


TEMPLATE = lib
CONFIG +=  c++17
DEFINES += DAPYSCRIPTS_BUILDLIB
TARGET = $$saLibNameMake(DAPyScripts)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAPyScripts.h \
    DAPyScriptsDataFrame.h \
    DAPyScriptsGlobal.h \
    DAPyScriptsIO.h




SOURCES += \
    DAPyScripts.cpp \
    DAPyScriptsDataFrame.cpp \
    DAPyScriptsIO.cpp







