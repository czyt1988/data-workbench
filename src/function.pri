################################################################
# 用于拷贝编译完成的库到对应目录
# czy.t@163.com
# 尘中远于2021年，添加快速部署脚本
################################################################
include( $${PWD}/common.pri )


#生成一个区别debug和release模式的lib名,输入一个lib名字
defineReplace(saLibNameMake) {
    LibName = $$1
    CONFIG(debug, debug|release){
        win32:LibName = $${LibName}d
        mac:LibName = $${LibName}_debug
    }else{
        LibName = $${LibName}
    }
    return ($${LibName})
}

# 用于修正路径，对于window会把路径的/转变为\,unix下不作为
defineReplace(saFixPath) {
    PathName = $$1
    win32 {
        FixPathName = $$replace(PathName, /, \\)
    }
    unix {
        FixPathName = $${PathName}
    }
    return ($${FixPathName})
}

# 把文件拷贝到bin/build_libs目录下
defineReplace(saCopy) {
    DIR_FROM = $$saFixPath($$1)
    DIR_TO = $$saFixPath($$2)
    CMD = $${QMAKE_COPY} $${DIR_FROM} $${DIR_TO}
    QMAKE_POST_LINK += $${CMD}
    export(QMAKE_POST_LINK)
    message($${CMD})
    return (true)
}

# 把文件拷贝到$$DA_BIN_DIR下
defineReplace(saCopyToBin) {
    $$saCopy($$1,$${DA_BIN_DIR})
    return (true)
}

# 把文件拷贝到bin/build_libs目录下
defineReplace(saCopyToBuildLibDir) {
    $$saCopy($$1,$${BIN_LIB_BUILD_DIR})
    return (true)
}
# 生成拷贝第三方库的cmd命令
defineReplace(saFindAndCopyPackageToBin) {
    LibFullPath = $$1
    LibName = $$2
    win32 {
        DIR1 = $${LibFullPath}/$${LibName}.dll
    }
    unix {
        DIR1 = $${LibFullPath}/$${LibName}.so
    }
    $$saCopyToBin($$DIR1)
    return (true)
}

# 生成拷贝第三方库的cmd命令 arg1: lib名称 此函数将从$$BIN_LIB_DIR的lib文件拷贝到$$DA_BIN_DIR下
defineReplace(saCopyLibToBin) {
    LibName = $$1
    win32 {
        DIR1 = $${BIN_LIB_BUILD_DIR}/$${LibName}.dll
    }
    unix {
        DIR1 = $${BIN_LIB_BUILD_DIR}/$${LibName}.so
    }
    $$saCopyToBin($$DIR1)
    return (true)
}


# 把文件拷贝到bin/build_libs目录下
defineReplace(saCopyToBuildLibDir) {
    DIR_TO = $$saFixPath($${BIN_LIB_BUILD_DIR})
    $$saCopy($$1,$${DIR_TO})
    return (true)
}

# 把plugin lib拷贝到plugin目录下,arg1为plugin的名称
defineReplace(saCopyPluginLibToPlugin) {
    LibName = $$1
    PLUGIN_FOLDER = $$saFixPath($${BIN_PLUGIN_DIR})
    win32 {
        DIR1 = $${BIN_PLUGIN_BUILD_DIR}/$${LibName}.dll
        DIR2 = $${PLUGIN_FOLDER}/$${LibName}.dll
    }
    unix {
        DIR1 = $${BIN_PLUGIN_BUILD_DIR}/$${LibName}.so
        DIR2 = $${PLUGIN_FOLDER}/$${LibName}.so
    }
    DIR_FROM = $$saFixPath($${DIR1})
    DIR_TO = $$saFixPath($${DIR2})
    CMD_CPY = $${QMAKE_COPY} $${DIR_FROM} $${DIR_TO}
    QMAKE_POST_LINK += $${CMD_CPY}
    export(QMAKE_POST_LINK)
    return (true)
}

#通用的设置 传入生成的lib名
defineReplace(commonProLibSet) {
    #lib构建在lib目录下
    TARGET = $$1
    DESTDIR = $${BIN_LIB_BUILD_DIR}
    MOC_DIR = $${DESTDIR}/$${TARGET}/moc
    RCC_DIR = $${DESTDIR}/$${TARGET}/rcc
    UI_DIR = $${DESTDIR}/$${TARGET}/qui
    OBJECTS_DIR = $${DESTDIR}/$${TARGET}/obj
    export(TARGET)
    export(DESTDIR)
    export(MOC_DIR)
    export(RCC_DIR)
    export(UI_DIR)
    export(OBJECTS_DIR)
    return (true)
}

# 通用的App设置 传入生成的App名
# 例如：
# $$commonProAppSet($${TARGET})
defineReplace(commonProAppSet) {
    #lib构建在lib目录下
    TARGET = $$1
    DESTDIR = $${DA_BIN_DIR}
    MOC_DIR = $${BIN_APP_BUILD_DIR}/$${TARGET}/moc
    RCC_DIR = $${BIN_APP_BUILD_DIR}/$${TARGET}/rcc
    UI_DIR = $${BIN_APP_BUILD_DIR}/$${TARGET}/qui
    OBJECTS_DIR = $${BIN_APP_BUILD_DIR}/$${TARGET}/obj
    export(TARGET)
    export(DESTDIR)
    export(MOC_DIR)
    export(RCC_DIR)
    export(UI_DIR)
    export(OBJECTS_DIR)
    return (true)
}

#通用的测试程序设置 传入生成的测试名
defineReplace(commonProTstAppSet) {
    #lib构建在lib目录下
    TARGET = $$1
    DESTDIR = $${BIN_TEST_DIR}
    MOC_DIR = $${BIN_TEST_BUILD_DIR}/$${TARGET}/moc
    RCC_DIR = $${BIN_TEST_BUILD_DIR}/$${TARGET}/rcc
    UI_DIR = $${BIN_TEST_BUILD_DIR}/$${TARGET}/qui
    OBJECTS_DIR = $${BIN_TEST_BUILD_DIR}/$${TARGET}/obj
    export(TARGET)
    export(DESTDIR)
    export(MOC_DIR)
    export(RCC_DIR)
    export(UI_DIR)
    export(OBJECTS_DIR)
    return (true)
}

#通用的设置 传入生成的plugin lib名
defineReplace(commonProPluginSet) {
    #plugin lib构建在lib/plugin目录下
    TARGET = $$1
    DESTDIR = $${BIN_PLUGIN_BUILD_DIR}
    MOC_DIR = $${DESTDIR}/$${TARGET}/moc
    RCC_DIR = $${DESTDIR}/$${TARGET}/rcc
    UI_DIR = $${DESTDIR}/$${TARGET}/qui
    OBJECTS_DIR = $${DESTDIR}/$${TARGET}/obj
    export(TARGET)
    export(DESTDIR)
    export(MOC_DIR)
    export(RCC_DIR)
    export(UI_DIR)
    export(OBJECTS_DIR)
    return (true)
}

# 此函数针对插件，把
defineReplace(copyDesignerPluginToQtDesignerPath) {
    LIBNAME = $$1
    win32 {
        LIBFILENAME = $${LIBNAME}.dll
        DIR_FROM = $${BIN_LIB_BUILD_DIR}/$${LIBFILENAME}
    }
    unix {
        LIBFILENAME = $${LIBNAME}.so
        DIR_FROM = $${BIN_LIB_BUILD_DIR}/$${LIBFILENAME}
    }
    DIR_FROM = $$saFixPath($${DIR_FROM})
    PLUGINDLL_DESIGNER_PATH = $$[QT_INSTALL_PLUGINS]/designer/$${LIBFILENAME}
    PLUGINDLL_DESIGNER_PATH = $$saFixPath($${PLUGINDLL_DESIGNER_PATH})
    CMD_CPY = $${QMAKE_COPY} $${DIR_FROM} $${PLUGINDLL_DESIGNER_PATH}
    message(cmd:$${CMD_CPY})
    QMAKE_POST_LINK += $${CMD_CPY}
    export(QMAKE_POST_LINK)
    return (true)
}
