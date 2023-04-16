##############################################
# 此库依赖ctk
# 依赖此库的需要引入ctk头文件
# include($$DA_3RD_PARTY_DIR/use3rdparty_ctk.pri.pri)
# include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)
##############################################

QT          +=  core gui widgets xml
#QT += opengl
TEMPLATE = lib
DEFINES += DACOMMONWIDGETS_BUILD
CONFIG		+=  c++14
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
TARGET = $$saLibNameMake(DACommonWidgets)

# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})
# 引用DAUtils
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
# 引用ctk
include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
# 引用SARibbon
include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)

HEADERS += \
    DAAbstractSettingPage.h \
    DABrushEditWidget.h \
    DABrushStyleComboBox.h \
    DACollapsibleGroupBox.h \
    DAColorPickerButton.h \
    DACommonWidgetsAPI.h \
    DAFontEditPannelWidget.h \
    DAPathLineEdit.h \
    DAPenEditWidget.h \
    DAPenStyleComboBox.h \
    DASettingDialog.h \
    DASettingWidget.h \
    DAShapeEditPannelWidget.h \
    DAWaitCursorScoped.h

SOURCES += \
    DAAbstractSettingPage.cpp \
    DABrushEditWidget.cpp \
    DABrushStyleComboBox.cpp \
    DACollapsibleGroupBox.cpp \
    DAColorPickerButton.cpp \
    DAFontEditPannelWidget.cpp \
    DAPathLineEdit.cpp \
    DAPenEditWidget.cpp \
    DAPenStyleComboBox.cpp \
    DASettingDialog.cpp \
    DASettingWidget.cpp \
    DAShapeEditPannelWidget.cpp \
    DAWaitCursorScoped.cpp

FORMS += \
    DABrushEditWidget.ui \
    DAFontEditPannelWidget.ui \
    DASettingDialog.ui \
    DASettingWidget.ui \
    DAShapeEditPannelWidget.ui

RESOURCES += \
    icon.qrc




