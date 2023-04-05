include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAPyCommonWidgets)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $${CTK_INCLUDE_PATHS}
DEPENDPATH += $${CTK_INCLUDE_PATHS}
LIBS += -L$${BIN_LIB_BUILD_DIR} -l$${LIB_NAME}

#DAPyBindQt自动引入
#include($${DA_SRC_DIR}/python_lib.pri)
#include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
