﻿#include "AppMainWindow.h"
#include <QProcess>
#include <QObject>
#include <QApplication>
#include <QDebug>
#include <QLocale>
#include "DAAppCore.h"
#include "DAMessageHandler.h"
#include "DAPybind11InQt.h"
#include "DAPyScripts.h"
#include "DAPyInterpreter.h"
#include "DAPybind11QtTypeCast.h"
#include "DATranslatorManeger.h"
#include "DADumpCapture.h"
void setAppFont();

int main(int argc, char* argv[])
{
    //进行dump捕获
    DA::DADumpCapture::initDump();
    //注册旋转文件消息捕获
    DA::daRegisterRotatingMessageHandler("./log/da_log.log");
    // DA::daRegisterConsolMessageHandler();
    for (int i = 0; i < argc; ++i) {
        qDebug() << argv[ i ];
    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    DA::DATranslatorManeger datr;
    datr.installAllTranslator();

    //注册元对象
    DA::PY::registerMetaType();
    setAppFont();
    DA::DAAppCore& core = DA::DAAppCore::getInstance();
    //初始化python环境
    if (!core.initialized()) {
        qCritical() << QObject::tr("Kernel initialization failed");  // cn:内核初始化失败
        return -1;
    }

    // TODO 此处进行一些核心的初始化操作
    DA::AppMainWindow w;

    w.show();
    int r = app.exec();
    DA::daUnregisterMessageHandler();
    return r;
}

void setAppFont()
{
    QFont font = QApplication::font();
    font.setFamily(QStringLiteral(u"微软雅黑"));
    QApplication::setFont(font);
}
