##############################################
# 此库依赖ctk
# 依赖此库的需要引入ctk头文件
# include($$DA_3RD_PARTY_DIR/use3rdparty_ctk.pri.pri)
##############################################

QT          +=  core gui widgets
#QT += opengl
TEMPLATE = lib
DEFINES += DACOMMONWIDGETS_BUILD
CONFIG		+=  c++11
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
TARGET = $$saLibNameMake(DACommonWidgets)

# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

# 引用ctk
include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)

HEADERS += \
    DAAbstractSettingPage.h \
    DABrushEditWidget.h \
    DABrushStyleComboBox.h \
    DAColorPickerButton.h \
    DACommonWidgetsAPI.h \
    DAFontEditPannelWidget.h \
    DAPathLineEdit.h \
    DAPenEditWidget.h \
    DAPenStyleComboBox.h \
    DASettingWidget.h \
    DAShapeEditPannelWidget.h

SOURCES += \
    DAAbstractSettingPage.cpp \
    DABrushEditWidget.cpp \
    DABrushStyleComboBox.cpp \
    DAColorPickerButton.cpp \
    DAFontEditPannelWidget.cpp \
    DAPathLineEdit.cpp \
    DAPenEditWidget.cpp \
    DAPenStyleComboBox.cpp \
    DASettingWidget.cpp \
    DAShapeEditPannelWidget.cpp

FORMS += \
    DABrushEditWidget.ui \
    DAFontEditPannelWidget.ui \
    DASettingWidget.ui \
    DAShapeEditPannelWidget.ui

RESOURCES += \
    icon.qrc




