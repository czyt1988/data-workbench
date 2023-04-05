include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAWorkFlow)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

#DAGraphicsView自动引入
#include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# 依赖 设置界面 TODO 节点设置可以单独作为一个包
include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
