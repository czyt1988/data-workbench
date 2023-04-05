include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAData)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

# DAPyScripts 自动引入
#include($${DA_SRC_DIR}/python_lib.pri) # 引入python环境
#include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri) # 引入pybind11
#include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
# da utility
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
