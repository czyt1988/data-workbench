include($${PWD}/common_3rdparty.pri)
ThirdParty_ctk_Dir=$$PWD/ctk/ctk
include($${ThirdParty_ctk_Dir}/ctk.pri)
#-L$${BIN_LIB_BUILD_DIR}
LIBS += -l$${CTK_LIB_NAME}

