##############################################
# 此库依赖QWT
# 依赖此库的需要引入QWT头文件
# include($$DA_3RD_PARTY_DIR/use3rdparty_qwt.pri)
##############################################

QT          +=  core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
TEMPLATE = lib
DEFINES += DAFIGURE_BUILD
CONFIG		+=  c++11
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
TARGET = $$saLibNameMake(DAFigure)

# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

# 引用QWT
include($${DA_3RD_PARTY_DIR}/use3rdparty_qwt.pri)

HEADERS += \
    DAAbstractChartMarker.h \
    DAChartCanvas.h \
    DAChartCrossTracker.h \
    DAChartEllipseRegionSelectEditor.h \
    DAChartFactory.h \
    DAChartItemTableModel.h \
    DAChartPointMarker.h \
    DAChartPolygonRegionSelectEditor.h \
    DAChartRectRegionSelectEditor.h \
    DAChartScrollBar.h \
    DAChartScrollZoomer.h \
    DAChartSelectRegionShapeItem.h \
    DAChartSerialize.h \
    DAChartWidget.h \
    DAChartXYDataPicker.h \
    DAChartYDataPicker.h \
    DAChartYValueMarker.h \
    DAFigureAPI.h \
    DAFigureContainer.h \
    DAFigureTreeModel.h \
    DAFigureTreeView.h \
    DAFigureWidget.h \
    DAFigureWidgetOverlay.h \
    DAChartUtil.h \
    DAAbstractPlotEditor.h \
    DAAbstractRegionSelectEditor.h \
    DAFigureWidgetOverlayChartEditor.h

HEADERS += \
    MarkSymbol/DAAbstractMarkSymbol.h \
    MarkSymbol/DATriangleMarkSymbol.h \


SOURCES += \
    DAAbstractChartMarker.cpp \
    DAChartCanvas.cpp \
    DAChartCrossTracker.cpp \
    DAChartEllipseRegionSelectEditor.cpp \
    DAChartFactory.cpp \
    DAChartItemTableModel.cpp \
    DAChartPointMarker.cpp \
    DAChartPolygonRegionSelectEditor.cpp \
    DAChartRectRegionSelectEditor.cpp \
    DAChartScrollBar.cpp \
    DAChartScrollZoomer.cpp \
    DAChartSelectRegionShapeItem.cpp \
    DAChartSerialize.cpp \
    DAChartWidget.cpp \
    DAChartXYDataPicker.cpp \
    DAChartYDataPicker.cpp \
    DAChartYValueMarker.cpp \
    DAFigureContainer.cpp \
    DAFigureTreeModel.cpp \
    DAFigureTreeView.cpp \
    DAFigureWidget.cpp \
    DAFigureWidgetOverlay.cpp \
    DAChartUtil.cpp \
    DAAbstractPlotEditor.cpp \
    DAAbstractRegionSelectEditor.cpp \
    DAFigureWidgetOverlayChartEditor.cpp

SOURCES += \
    MarkSymbol/DAAbstractMarkSymbol.cpp \
    MarkSymbol/DATriangleMarkSymbol.cpp \


RESOURCES += \
    icon.qrc


