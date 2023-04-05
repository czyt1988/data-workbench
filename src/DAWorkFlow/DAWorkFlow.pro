QT       += core gui xml svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += DAWORKFLOW_BUILDLIB
CONFIG		+=  c++11
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)


TARGET = $$saLibNameMake(DAWorkFlow)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})
# 依赖 可缩放的graphicsView


include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# 依赖 设置界面 TODO 节点设置可以单独作为一个包
#include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)

HEADERS +=\
    DACommandsForWorkFlowNodeGraphics.h \
    DANodeLinkPointDrawDelegate.h \
    DAStandardNodePixmapGraphicsItem.h \
    DAStandardNodeRectGraphicsItem.h \
    DAStandardNodeSvgGraphicsItem.h \
    DAStandardNodeWidgetGraphicsItem.h \
    DAWorkFlowExecuter.h \
    DAWorkFlowGlobal.h \
    DAAbstractNode.h \
    DAAbstractNodeFactory.h \
    DAAbstractNodeGraphicsItem.h \
    DAAbstractNodeLinkGraphicsItem.h \
    DAAbstractNodeWidget.h \
    DANodeGraphicsScene.h \
    DANodeLinkPoint.h \
    DANodeMetaData.h \
    DANodePalette.h \  
    DAStandardNodeLinkGraphicsItem.h \
    DAWorkFlow.h



SOURCES += \
    DAAbstractNode.cpp \
    DAAbstractNodeFactory.cpp \
    DAAbstractNodeGraphicsItem.cpp \
    DAAbstractNodeLinkGraphicsItem.cpp \
    DAAbstractNodeWidget.cpp \
    DACommandsForWorkFlowNodeGraphics.cpp \
    DANodeGraphicsScene.cpp \
    DANodeLinkPoint.cpp \
    DANodeLinkPointDrawDelegate.cpp \
    DANodeMetaData.cpp \
    DANodePalette.cpp \
    DAStandardNodeLinkGraphicsItem.cpp \
    DAStandardNodePixmapGraphicsItem.cpp \
    DAStandardNodeRectGraphicsItem.cpp \
    DAStandardNodeSvgGraphicsItem.cpp \
    DAStandardNodeWidgetGraphicsItem.cpp \
    DAWorkFlow.cpp \
    DAWorkFlowExecuter.cpp




