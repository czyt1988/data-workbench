include($$PWD/common_3rdparty.pri)
include($$PWD/../function.pri)
system(mkdir $$saFixPath($${BIN_LIB_BUILD_DIR}))#创建库路径
TEMPLATE = subdirs
SUBDIRS += \
          $${PWD}/ctk \# lite ctk
          $${PWD}/SARibbon/src/SARibbonBar \ #只编译SARibbonBar
          $${PWD}/Qt-Advanced-Docking-System/src \#  Qt-Advanced-Docking-System 库
          $${PWD}/qwt/src 

include($${PWD}/SARibbon/common.pri)
$$saFindAndCopyPackageToBin($${SARIBBON_BIN_DIR},$${SARIBBON_LIB_NAME})
$$saCopyToBuildLibDir($${SARIBBON_BIN_DIR}/*$${SARIBBON_LIB_NAME}*)

CONFIG(debug, debug|release){
    win32 {
        versionAtLeast(QT_VERSION, 5.15.0) {
                QtAdvancedDockingSystemLibName = qtadvanceddocking
        }
        else {
                QtAdvancedDockingSystemLibName = qtadvanceddockingd
        }
    }
    else:mac {
        QtAdvancedDockingSystemLibName = qtadvanceddocking_debug
    }
    else {
        QtAdvancedDockingSystemLibName = qtadvanceddocking
    }
}
else{
    QtAdvancedDockingSystemLibName = qtadvanceddocking
}
$$saFindAndCopyPackageToBin($${OUT_PWD}/Qt-Advanced-Docking-System/lib,$${QtAdvancedDockingSystemLibName})
$$saCopyToBuildLibDir($${OUT_PWD}/Qt-Advanced-Docking-System/lib/*qtadvanceddocking*)

CONFIG   += ordered
