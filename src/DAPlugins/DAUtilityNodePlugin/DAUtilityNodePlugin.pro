QT       += core gui svg xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += DAUTILITYNODEPLUGIN_BUILDLIB
CONFIG += c++11
CONFIG += plugin # 插件
include($$PWD/../../common.pri)
include($$PWD/../../function.pri)
TARGET = $$saLibNameMake(DAUtilityNodePlugin)
# 通用的设置
$$commonProPluginSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin/plugins目录下
$$saCopyPluginLibToPlugin($${TARGET})

include($${DA_SRC_DIR}/DAPluginSupport/DAPluginSupport.pri)

HEADERS += \
    DAUtilityNodeAppExecute.h \
    DAUtilityNodeAppExecuteGraphicsItem.h \
    DAUtilityNodeFactory.h \
    DAUtilityNodePlugin.h \
    DAUtilityNodePluginAPI.h \
    settingWidget/DAUtilityNodeAppExecuteSettingWidget.h


SOURCES += \
    DAUtilityNodeAppExecute.cpp \
    DAUtilityNodeAppExecuteGraphicsItem.cpp \
    DAUtilityNodeFactory.cpp \
    DAUtilityNodePlugin.cpp \
    settingWidget/DAUtilityNodeAppExecuteSettingWidget.cpp

FORMS += \
    settingWidget/DAUtilityNodeAppExecuteSettingWidget.ui


RESOURCES += \
    resource.qrc



TRANSLATIONS += daUtilsPlugin_zh_CN.ts
