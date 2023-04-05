#include <QApplication>
#include <QDebug>
#include <QLocale>
#include "DAMessageHandler.h"

int main(int argc, char* argv[])
{
    //注册旋转文件消息捕获
    daRegisterRotatingMessageHandler("./log/da_log.log");
    //注册标准输出消息捕获
    // daRegisterStdOutMessageHandler();
    qDebug() << QStringLiteral(u"数据工作流-版本=v0.0.1");
    qInfo() << QStringLiteral(u"qInfo test");
    qWarning() << QStringLiteral(u"qWarning test");
    qCritical() << QStringLiteral(u"qCritical test");
    daUnregisterMessageHandler();
    return 0;
}
