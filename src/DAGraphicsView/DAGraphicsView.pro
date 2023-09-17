QT       += core gui xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += DAGRAPHICSVIEW_BUILDLIB
CONFIG		+=  c++11
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)

TARGET = $$saLibNameMake(DAGraphicsView)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
HEADERS +=\
    DAGraphicsItem.h \
    DAGraphicsItemFactory.h \
    DAGraphicsLinkItem.h \
    DAGraphicsResizeableRectItem.h \
    DAGraphicsViewGlobal.h \
    DAGraphicsTextItem.h \
    DAGraphicsResizeableItem.h \
    DAGraphicsResizeablePixmapItem.h \
    DAGraphicsResizeableTextItem.h \
    DAGraphicsRelativeSimpleTextItem.h \
    DAStandardGraphicsTextItem.h \
    DAGraphicsSceneWithUndoStack.h \
    DACommandsForGraphics.h \
    DAGraphicsView.h 


SOURCES += \
    DAGraphicsItem.cpp \
    DAGraphicsItemFactory.cpp \
    DAGraphicsLinkItem.cpp \
    DAGraphicsRelativeSimpleTextItem.cpp \
    DAGraphicsResizeableItem.cpp \
    DAGraphicsResizeablePixmapItem.cpp \
    DAGraphicsResizeableRectItem.cpp \
    DAGraphicsResizeableTextItem.cpp \
    DAStandardGraphicsTextItem.cpp \
    DAGraphicsSceneWithUndoStack.cpp \
    DACommandsForGraphics.cpp \
    DAGraphicsView.cpp 




