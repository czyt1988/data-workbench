include(./../common.pri)
include(./../function.pri)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIB_NAME=$$saLibNameMake(DAUtils)
LIBS += -l$${LIB_NAME}
