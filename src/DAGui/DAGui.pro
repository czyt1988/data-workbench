###########################
# DAGui库，包括da-work-flow下的大部分ui类的封装，此库的下层级就是接口类，接口类的ui部分大部分都是基于此库暴露的接口
# DAGui库是一个较为重度的依赖库
#
###########################

QT       += core gui xml svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += DAGUI_BUILDLIB
CONFIG		+=  c++11
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)


TARGET = $$saLibNameMake(DAGui)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

# 两个第三方库
include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)
include($${DA_3RD_PARTY_DIR}/use3rdparty_QtAdvancedDockingSystem.pri)
# DAMessageHandler包含
# DAMessageHandler
# |——include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
# |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
include($${DA_SRC_DIR}/DAMessageHandler/DAMessageHandler.pri)
# DAData
# |——include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
#    |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
#       |——include($${DA_SRC_DIR}/python_lib.pri)
#       |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAData/DAData.pri)
# 依赖 设置界面 TODO 节点设置可以单独作为一个包
# DACommonWidgets
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
# DAPyCommonWidgets
# |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
#       |——include($${DA_SRC_DIR}/python_lib.pri)
#       |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAPyCommonWidgets/DAPyCommonWidgets.pri)
# DAWorkFlow
# |——include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
#    |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
include($${DA_SRC_DIR}/DAWorkFlow/DAWorkFlow.pri)
# DAFigure
include($${DA_SRC_DIR}/DAFigure/DAFigure.pri)


# 子模块
include($$PWD/Models/Models.pri)
include($$PWD/Dialog/Dialog.pri)
include($$PWD/MimeData/MimeData.pri)
include($$PWD/Commands/Commands.pri)
include($$PWD/ChartSetting/ChartSetting.pri)

HEADERS += \
    DAAbstractChartAddItemWidget.h \
    DAChartAddXYSeriesWidget.h \
    DADataManagerComboBox.h \
    DADataOperatePageWidget.h \
    DADataframeToVectorPointWidget.h \
    DAFigureFactory.h \
    DAGuiAPI.h \
    DAChartListView.h \
    DAChartManageWidget.h \
    DAChartOperateWidget.h \
    DADataListView.h \
    DADataManageTableView.h \
    DADataManageTreeView.h \
    DADataManageWidget.h \
    DADataOperateOfDataFrameWidget.h \
    DADataOperateWidget.h \
    DAMessageLogViewWidget.h \
    DANodeItemSettingWidget.h \
    DANodeLinkItemSettingWidget.h \
    DANodeListWidget.h \
    DANodeMimeData.h \
    DANodeSettingWidget.h \
    DANodeTreeWidget.h \
    DAToolBox.h \
    DASettingContainerWidget.h \
    DAWorkFlowEditWidget.h \
    DAWorkFlowGraphicsScene.h \
    DAWorkFlowGraphicsView.h \
    DAWorkFlowNodeListWidget.h \
    DAWorkFlowNodeItemSettingWidget.h \
    DAWorkFlowOperateWidget.h \
    DAXmlHelper.h

SOURCES += \
    DAAbstractChartAddItemWidget.cpp \
    DAChartAddXYSeriesWidget.cpp \
    DAChartListView.cpp \
    DAChartManageWidget.cpp \
    DAChartOperateWidget.cpp \
    DADataListView.cpp \
    DADataManageTableView.cpp \
    DADataManageTreeView.cpp \
    DADataManageWidget.cpp \
    DADataManagerComboBox.cpp \
    DADataOperatePageWidget.cpp \
    DADataOperateWidget.cpp \
    DADataOperateOfDataFrameWidget.cpp \
    DADataframeToVectorPointWidget.cpp \
    DAFigureFactory.cpp \
    DAMessageLogViewWidget.cpp \
    DANodeItemSettingWidget.cpp \
    DANodeLinkItemSettingWidget.cpp \
    DANodeListWidget.cpp \
    DANodeMimeData.cpp \
    DANodeSettingWidget.cpp \
    DANodeTreeWidget.cpp \
    DAToolBox.cpp \
    DASettingContainerWidget.cpp \
    DAWorkFlowEditWidget.cpp \
    DAWorkFlowGraphicsScene.cpp \
    DAWorkFlowGraphicsView.cpp \
    DAWorkFlowNodeListWidget.cpp \
    DAWorkFlowNodeItemSettingWidget.cpp \
    DAWorkFlowOperateWidget.cpp \
    DAXmlHelper.cpp

FORMS += \
    DAChartAddXYSeriesWidget.ui \
    DAChartManageWidget.ui \
    DAChartOperateWidget.ui \
    DADataManageWidget.ui \
    DADataOperateOfDataFrameWidget.ui \
    DADataOperateWidget.ui \
    DADataframeToVectorPointWidget.ui \
    DAMessageLogViewWidget.ui \
    DANodeItemSettingWidget.ui \
    DANodeLinkItemSettingWidget.ui \
    DANodeSettingWidget.ui \
    DASettingContainerWidget.ui \
    DAWorkFlowOperateWidget.ui \
    DAWorkFlowEditWidget.ui \
    DAWorkFlowNodeListWidget.ui \
    DAWorkFlowNodeItemSettingWidget.ui

RESOURCES += \
    icon.qrc \



