ThirdParty_QWT_Dir=$$PWD/qwt/qwt
# common_3rdparty 定义了第三方库的路径
include($${ThirdParty_QWT_Dir}/qwtconfig.pri)
include( $${ThirdParty_QWT_Dir}/qwtfunctions.pri)
CONFIG += qwt
QWT_CONFIG  += QwtDll
DEFINES    += QWT_DLL
INCLUDEPATH += $${ThirdParty_QWT_Dir}/src
INCLUDEPATH += $${ThirdParty_QWT_Dir}/classincludes
DEPENDPATH  += $${ThirdParty_QWT_Dir}/classincludes
LIBS += -l$$saLibNameMake(qwt)


