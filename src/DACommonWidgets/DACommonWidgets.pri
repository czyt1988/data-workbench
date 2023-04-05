include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DACommonWidgets)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -L$${BIN_LIB_BUILD_DIR} -l$${LIB_NAME}

# 引用ctk
include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
