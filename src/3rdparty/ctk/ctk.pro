QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

DEFINES += MAKE_CTK_LIB #定义此宏将构建库
DEFINES += CTK_SHARED
include($$PWD/../common_3rdparty.pri)
include($$PWD/ctk.pri)
include($$PWD/ctk_source.pri)
TARGET = $${CTK_LIB_NAME}
TEMPLATE = lib
DESTDIR = $${BIN_LIB_BUILD_DIR}
