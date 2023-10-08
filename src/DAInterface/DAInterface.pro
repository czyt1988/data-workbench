QT          +=  core gui xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = lib
DEFINES += DAINTERFACE_BUILDLIB
CONFIG		+=  c++17
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
TARGET = $$saLibNameMake(DAInterface)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAActionsInterface.h \
    DABaseInterface.h \
    DACommandInterface.h \
    DACoreInterface.h \
    DADataManagerInterface.h \
    DADockingAreaInterface.h \
    DAInterfaceAPI.h \
    DAProjectInterface.h \
    DARibbonAreaInterface.h \
    DAUIExtendInterface.h \
    DAUIInterface.h

SOURCES += \
    DAActionsInterface.cpp \
    DABaseInterface.cpp \
    DACommandInterface.cpp \
    DACoreInterface.cpp \
    DADataManagerInterface.cpp \
    DADockingAreaInterface.cpp \
    DAProjectInterface.cpp \
    DARibbonAreaInterface.cpp \
    DAUIExtendInterface.cpp \
    DAUIInterface.cpp

# gui  相关
#DAGui
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_QtAdvancedDockingSystem.pri)
# |——include($${DA_SRC_DIR}/DAMessageHandler/DAMessageHandler.pri)
# |  |——include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
# |  |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
# |——include($${DA_SRC_DIR}/DAData/DAData.pri)
# |  |——include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
# |  |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
# |     |——include($${DA_SRC_DIR}/python_lib.pri)
# |     |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
# |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
# |  |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
# |——include($${DA_SRC_DIR}/DAPyCommonWidgets/DAPyCommonWidgets.pri)
# |  |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
# |     |——include($${DA_SRC_DIR}/python_lib.pri)
# |     |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
# |——include($${DA_SRC_DIR}/DAWorkFlow/DAWorkFlow.pri)
# |  |——include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# |  |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
include($${DA_SRC_DIR}/DAGui/DAGui.pri)
