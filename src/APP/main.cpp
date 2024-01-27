#include "AppMainWindow.h"
#include <QProcess>
#include <QObject>
#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QFileInfo>
#include <QDir>
#include "DAAppCore.h"
#include "DAMessageHandler.h"
#include "DATranslatorManeger.h"
#include "DADumpCapture.h"
#include "SARibbonBar.h"
#ifdef DA_ENABLE_PYTHON
#include "DAPybind11InQt.h"
#include "DAPyScripts.h"
#include "DAPyInterpreter.h"
#include "DAPybind11QtTypeCast.h"
#endif
void setAppFont();

void enableHDPIScaling()
{
    SARibbonBar::initHighDpi();
}

int main(int argc, char* argv[])
{
    // 程序路径捕获
    QFileInfo fi(argv[ 0 ]);
    // 进行dump捕获
    DA::DADumpCapture::initDump(fi.absolutePath());
    //
    QString logfilePath = QDir::toNativeSeparators(fi.absolutePath() + "/log/da_log.log");
    // 注册旋转文件消息捕获
    DA::daRegisterRotatingMessageHandler(logfilePath);
    // DA::daRegisterConsolMessageHandler();
    for (int i = 0; i < argc; ++i) {
        qDebug() << "argv[" << i << "]" << argv[ i ];
    }
    // 高清屏的适配
    enableHDPIScaling();

    // 启动app
    QApplication app(argc, argv);
    DA::DATranslatorManeger datr;
    datr.installAllTranslator();
#ifdef DA_ENABLE_PYTHON
    // 注册元对象
    DA::PY::registerMetaType();
#endif
    setAppFont();
    DA::DAAppCore& core = DA::DAAppCore::getInstance();
    // 初始化python环境
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
