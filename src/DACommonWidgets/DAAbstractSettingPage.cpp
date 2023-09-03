#include "DAAbstractSettingPage.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
namespace DA
{

//===================================================
// DAAbstractSettingPage
//===================================================

DAAbstractSettingPage::DAAbstractSettingPage(QWidget* parent, Qt::WindowFlags f) : QWidget(parent, f)
{
}

DAAbstractSettingPage::~DAAbstractSettingPage()
{
}

QString DAAbstractSettingPage::getConfigFileSavePath()
{
    QString appConfigLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir;
    // 如果无法创建路径，将qcritual
    // mkpath回保证能有此路径，否则会返回false
    if (!dir.mkpath(appConfigLocation)) {
        qCritical() << tr("Unable to create application configuration path:%1");  // cn:无法创建音乐配置路径：%1
    }
    return appConfigLocation;
}

}
