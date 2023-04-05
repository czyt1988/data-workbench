include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAMessageHandler)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}


# 自身依赖
include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
