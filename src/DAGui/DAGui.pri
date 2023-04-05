include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAGui)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

# 两个第三方库
include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)
include($${DA_3RD_PARTY_DIR}/use3rdparty_QtAdvancedDockingSystem.pri)
# DAMessageHandler包含
# DAMessageHandler
# |——include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
# |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
include($${DA_SRC_DIR}/DAMessageHandler/DAMessageHandler.pri)
# DAData
# |——include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
#    |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
#       |——include($${DA_SRC_DIR}/python_lib.pri)
#       |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAData/DAData.pri)
# 依赖 设置界面 TODO 节点设置可以单独作为一个包
# DACommonWidgets
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
# DAPyCommonWidgets
# |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
#       |——include($${DA_SRC_DIR}/python_lib.pri)
#       |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
include($${DA_SRC_DIR}/DAPyCommonWidgets/DAPyCommonWidgets.pri)
# DAWorkFlow
# |——include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
#    |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
include($${DA_SRC_DIR}/DAWorkFlow/DAWorkFlow.pri)
# DAFigure
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_qwt.pri)
include($${DA_SRC_DIR}/DAFigure/DAFigure.pri)
