include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAFigure)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

# DAFigure 自动引入
include($${DA_SRC_DIR}/3rdparty/use3rdparty_qwt.pri) # 引入qwt

