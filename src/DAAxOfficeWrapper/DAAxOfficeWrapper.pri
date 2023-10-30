# QAxObject Wrapper 
include($$PWD/../common.pri)
include($$PWD/../function.pri)
LIB_NAME=$$saLibNameMake(DAAxOfficeWrapper)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

