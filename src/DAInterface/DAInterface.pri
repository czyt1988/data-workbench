include(./../common.pri)
include(./../function.pri)
LIB_NAME=$$saLibNameMake(DAInterface)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS += -l$${LIB_NAME}

#DAGui
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_SARibbon.pri)
# |——include($${DA_3RD_PARTY_DIR}/use3rdparty_QtAdvancedDockingSystem.pri)
# |——include($${DA_SRC_DIR}/DAMessageHandler/DAMessageHandler.pri)
# |  |——include($${DA_SRC_DIR}/DAUtils/DAUtils.pri)
# |  |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_spdlog.pri)
# |——include($${DA_SRC_DIR}/DAData/DAData.pri)
# |  |——include($${DA_SRC_DIR}/DAPyScripts/DAPyScripts.pri)
# |  |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
# |     |——include($${DA_SRC_DIR}/python_lib.pri)
# |     |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
# |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
# |  |——include($${DA_3RD_PARTY_DIR}/use3rdparty_ctk.pri)
# |——include($${DA_SRC_DIR}/DAPyCommonWidgets/DAPyCommonWidgets.pri)
# |  |——include($${DA_SRC_DIR}/DAPyBindQt/DAPyBindQt.pri)
# |     |——include($${DA_SRC_DIR}/python_lib.pri)
# |     |——include($${DA_SRC_DIR}/3rdparty/use3rdparty_pybind11.pri)
# |——include($${DA_SRC_DIR}/DAWorkFlow/DAWorkFlow.pri)
# |  |——include($${DA_SRC_DIR}/DAGraphicsView/DAGraphicsView.pri)
# |  |——include($${DA_SRC_DIR}/DACommonWidgets/DACommonWidgets.pri)
include($${DA_SRC_DIR}/DAGui/DAGui.pri)
