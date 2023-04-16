QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
include($$PWD/../common.pri)
include($$PWD/../function.pri)
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = daWork
#生成到bin下
$$commonProAppSet($${TARGET})


include($${DA_SRC_DIR}/DAPluginSupport/DAPluginSupport.pri)
include($$PWD/SettingPages/SettingPages.pri)
SOURCES += \
    AppMainWindow.cpp \
    DAAppActions.cpp \
    DAAppChartOperateWidget.cpp \
    DAAppCommand.cpp \
    DAAppController.cpp \
    DAAppCore.cpp \
    DAAppDataManager.cpp \
    DAAppDockingArea.cpp \
    DAAppFigureFactory.cpp \
    DAAppFigureWidget.cpp \
    DAAppPluginManager.cpp \
    DAAppProject.cpp \
    DAAppRibbonArea.cpp \
    DAAppUI.cpp \
    DAAppWorkFlowOperateWidget.cpp \
    DAGuideTabWidget.cpp \
    DAWordWrapItemDelegate.cpp \
    DADataWorkFlow.cpp \
    DAPluginManagerDialog.cpp \
    DAAppSettingDialog.cpp \
    main.cpp

HEADERS += \
    AppMainWindow.h \
    DAAppActions.h \
    DAAppChartOperateWidget.h \
    DAAppCommand.h \
    DAAppController.h \
    DAAppCore.h \
    DAAppDataManager.h \
    DAAppDockingArea.h \
    DAAppFigureFactory.h \
    DAAppFigureWidget.h \
    DAAppPluginManager.h \
    DAAppProject.h \
    DAAppRibbonArea.h \
    DAAppUI.h \
    DAAppWorkFlowOperateWidget.h \
    DADumpCapture.h \
    DAGuideTabWidget.h \
    DAWordWrapItemDelegate.h \
    DADataWorkFlow.h \
    DAPluginManagerDialog.h \
    DAAppSettingDialog.h

FORMS += \
    AppMainWindow.ui \
    DAPluginManagerDialog.ui

# windows下的msvc特殊配置
win32:msvc{
    #msvc，加入dump支持
LIBS += \
    -lDbghelp

# 加入管理员权限
#QMAKE_LFLAGS +=/MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
}



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target





RESOURCES += \
    icon.qrc

