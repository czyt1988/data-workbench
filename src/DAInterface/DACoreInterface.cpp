#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>
#include <QApplication>
#include "DAProjectInterface.h"
namespace DA
{

DACoreInterface::DACoreInterface(QObject* parent) : QObject(parent)
{
}

DACoreInterface::~DACoreInterface()
{
}

bool DACoreInterface::isProjectDirty() const
{
    DAProjectInterface* pi = getProjectInterface();
    if (pi) {
        return pi->isDirty();
    }
    return false;
}

void DACoreInterface::setProjectDirty(bool on)
{
    DAProjectInterface* pi = getProjectInterface();
    if (pi) {
        pi->setModified(on);
    }
}

/**
 * @brief 获取本程序的临时路径
 * @return
 */
QDir DACoreInterface::getTempDir() const
{
    return QDir(mTempDir.path());
}

/**
 * @brief 获取python脚本的位置
 *
 * 默认情况下，在初始化python环境后，脚本位置会被加入到python的系统目录中
 * @return
 */
QString DACoreInterface::getPythonScriptsPath()
{
    QString appabsPath = QApplication::applicationDirPath();
    return QDir::toNativeSeparators(appabsPath + "/PyScripts");
}
}  // namespace DA
