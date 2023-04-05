include(./../common.pri)
include(./../function.pri)
# 使用此库务必保证引入python环境
# include($${DA_SRC_DIR}/python_lib.pri)
# include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
LIB_NAME=$$saLibNameMake(DAPyScripts)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

#DAPyBindQt自动引入
#include($$PWD/../python_lib.pri) #需要使用pybind11
#include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
