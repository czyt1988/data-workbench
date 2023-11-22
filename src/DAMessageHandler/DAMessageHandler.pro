# 这是用于qdebug/qinfo/qwarning等qt日志系统的处理库，此库提供了日志窗口
QT          +=  core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = lib
DEFINES += DAMESSAGEHANDLER_BUILDLIB
CONFIG		+=  c++17
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
TARGET = $$saLibNameMake(DAMessageHandler)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DALog.h \
    DAMessageHandlerGlobal.h \
    DAMessageHandler.h \
    DAMessageLogItem.h \
    DAMessageQueueProxy.h


SOURCES += \
    DAMessageHandler.cpp \
    DAMessageLogItem.cpp \
    DAMessageQueueProxy.cpp




