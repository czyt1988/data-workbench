QT += core gui xml
QT += axcontainer
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += DAAXOFFICEWRAPPER_BUILDLIB
CONFIG		+=  c++17
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)

TARGET = $$saLibNameMake(DAAxOfficeWrapper)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
HEADERS +=\
    DAAxObjectExcelWrapper.h \
    DAAxObjectWordTableWrapper.h \
    DAAxObjectWordWrapper.h \
    DAAxOfficeWrapperGlobal.h 


SOURCES += \
    DAAxObjectExcelWrapper.cpp \
    DAAxObjectWordTableWrapper.cpp \
    DAAxObjectWordWrapper.cpp




