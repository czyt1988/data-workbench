#include "DAAbstractSettingPage.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "DADir.h"
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
    return DADir::getRootConfigPath();
}

}
