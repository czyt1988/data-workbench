ThirdParty_QtAdvancedDockingSystem_Dir=$$PWD/Qt-Advanced-Docking-System
include($$PWD/common_3rdparty.pri)
# common_3rdparty 定义了第三方库的路径
LIBS += -L$${BIN_LIB_BUILD_DIR} -l$$qtLibraryTarget(qtadvanceddocking)
INCLUDEPATH += $${ThirdParty_QtAdvancedDockingSystem_Dir}/src

