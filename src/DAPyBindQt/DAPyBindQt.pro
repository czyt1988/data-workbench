# 这是对python和Qt的封装，DA的python相关类都需要依赖此类

QT = core
include($$PWD/../common.pri)
include($$PWD/../function.pri)
include($$PWD/../python_lib.pri) #需要使用pybind11
include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)


TEMPLATE = lib
CONFIG +=  c++17
DEFINES += DAPYBINDQT_BUILDLIB
TARGET = $$saLibNameMake(DAPyBindQt)
# 通用的设置
$$commonProLibSet($${TARGET})

# 在lib文件夹下编译完后，把dll文件拷贝到bin目录下
$$saCopyLibToBin($${TARGET})

HEADERS += \
    DAPyInterpreter.h \
    DAPyBindQtGlobal.h \
    DAPyModule.h \
    DAPyObjectWrapper.h \
    DAPybind11InQt.h \
    DAPybind11QtTypeCast.h \
    DAPyModuleDatetime.h \
    $$PWD/numpy/DAPyModuleNumpy.h \
    $$PWD/numpy/DAPyDType.h \
    $$PWD/pandas/DAPyIndex.h \
    $$PWD/pandas/DAPyModulePandas.h \
    $$PWD/pandas/DAPyDataFrame.h \
    $$PWD/pandas/DAPySeries.h

SOURCES += \
    DAPyInterpreter.cpp \
    DAPyModule.cpp \
    DAPyObjectWrapper.cpp \
    DAPybind11InQt.cpp \
    DAPybind11QtTypeCast.cpp \
    DAPyModuleDatetime.cpp \
    $$PWD/numpy/DAPyModuleNumpy.cpp \
    $$PWD/numpy/DAPyDType.cpp \
    $$PWD/pandas/DAPyIndex.cpp \
    $$PWD/pandas/DAPyModulePandas.cpp \
    $$PWD/pandas/DAPyDataFrame.cpp \
    $$PWD/pandas/DAPySeries.cpp






