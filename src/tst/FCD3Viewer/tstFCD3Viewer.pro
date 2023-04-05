QT          +=  core gui widgets
TEMPLATE	=   app
CONFIG		+=  c++11
CONFIG		+=  qt

include($$PWD/../../common.pri)
include($$PWD/../function.pri)

TARGET = tst_FCD3Viewer
#生成到bin下
$$commonProAppSet($${TARGET})

HEADERS += \
        $$PWD/MainWindow.h

SOURCES += \
	$$PWD/MainWindow.cpp \
        $$PWD/main.cpp

FORMS += \
        $$PWD/MainWindow.ui

include($${FC_SRC_DIR}/FCD3Viewer/FCD3Viewer.pri)








