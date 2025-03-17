#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>

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
}  // namespace DA
