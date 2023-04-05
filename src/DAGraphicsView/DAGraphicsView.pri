include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAGraphicsView)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
