# 这是和python依赖的窗口库，封装了python操作相关的窗口

QT          +=  core gui widgets
#QT += opengl
TEMPLATE = lib
DEFINES += DAPYCOMMONWIDGETS_BUILD
CONFIG		+=  c++17
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
include($$PWD/../python_lib.pri) #需要使用pybind11
include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
TARGET = $$saLibNameMake(DAPyCommonWidgets)

# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAPyCommonWidgetsAPI.h \
    DAPyDTypeComboBox.h \ \
    DAPyDataframeColumnsListWidget.h


SOURCES += \
    DAPyDTypeComboBox.cpp \
    DAPyDataframeColumnsListWidget.cpp

RESOURCES += \
    resource.qrc




