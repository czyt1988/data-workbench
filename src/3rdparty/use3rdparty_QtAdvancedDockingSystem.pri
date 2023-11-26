ThirdParty_QtAdvancedDockingSystem_Dir=$$PWD/Qt-Advanced-Docking-System
include($$PWD/common_3rdparty.pri)
# common_3rdparty 定义了第三方库的路径
QAD_LIB_NAME = qt$${QT_MAJOR_VERSION}advanceddocking
LIBS += -L$${BIN_LIB_BUILD_DIR} -l$$qtLibraryTarget($${QAD_LIB_NAME})
INCLUDEPATH += $${ThirdParty_QtAdvancedDockingSystem_Dir}/src

