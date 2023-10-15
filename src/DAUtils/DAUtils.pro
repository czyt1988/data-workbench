##############################################
# DA 工具类库，封装共性算法和类
##############################################

QT          +=  core xml gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = lib
DEFINES += DAUTILS_BUILD
CONFIG		+=  c++17
CONFIG		+=  qt
include($$PWD/../common.pri)
include($$PWD/../function.pri)
TARGET = $$saLibNameMake(DAUtils)

# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAAbstractProtocol.h \
    DAAlgorithm.h \
    DAAutoincrementSeries.h \
    DAColorTheme.h \
    DAIndexedVector.h \
    DAProperties.h \
    DAQtContainerUtil.h \
    DAStringUtil.h \
    DAUtilsAPI.h \
    DATable.h \
    DATree.h \
    DATreeItem.h \
    DATranslatorManeger.h \
    DAXMLConfig.h \
    DAXMLFileInterface.h \
    DACsvStream.h \
    DAXMLProtocol.h \
    DAMimeData.h \


SOURCES += \
    DAAbstractProtocol.cpp \
    DAColorTheme.cpp \
    DAProperties.cpp \
    DAStringUtil.cpp \
    DATree.cpp \
    DATreeItem.cpp \
    DATranslatorManeger.cpp \
    DAXMLConfig.cpp \
    DAXMLFileInterface.cpp \
    DACsvStream.cpp \
    DAXMLProtocol.cpp \
    DAMimeData.cpp





