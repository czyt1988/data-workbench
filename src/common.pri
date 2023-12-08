CONFIG(debug, debug|release){
    contains(QT_ARCH, i386) {
        DA_BIN_DIR = $$PWD/../bin_qt$$[QT_VERSION]_Debug_x86
    }else {
        DA_BIN_DIR = $$PWD/../bin_qt$$[QT_VERSION]_Debug_x64
    }
}else{
    contains(QT_ARCH, i386) {
        DA_BIN_DIR = $$PWD/../bin_qt$$[QT_VERSION]_Release_x86
    }else {
        DA_BIN_DIR = $$PWD/../bin_qt$$[QT_VERSION]_Release_x64
    }
    # 此句为release版本能带有调试信息
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
DA_SRC_DIR = $$PWD # 源代码路径
DA_3RD_PARTY_DIR = $${DA_SRC_DIR}/3rdparty # 第三方库路径
BIN_APP_BUILD_DIR = $${DA_BIN_DIR}/bin/build_apps # 生成的app路径
BIN_LIB_BUILD_DIR = $${DA_BIN_DIR}/bin/build_libs # 生成的lib路径
BIN_TEST_BUILD_DIR = $${DA_BIN_DIR}/bin/build_tst # 生成的测试程序路径
BIN_PLUGIN_BUILD_DIR = $${DA_BIN_DIR}/bin/build_plugins # 生成的plugin路径
BIN_PLUGIN_DIR = $${DA_BIN_DIR}/bin/plugins #插件的路径
BIN_TEST_DIR = $${DA_BIN_DIR}/bin/tst #测试程序路径
# 把当前目录作为include路径
INCLUDEPATH += $$PWD
LIBS += -L$${BIN_LIB_BUILD_DIR}
