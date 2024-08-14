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
    if(pi){
        return pi->isDirty();
    }
    return false;
}

void DACoreInterface::setProjectDirty(bool on)
{
    DAProjectInterface* pi = getProjectInterface();
    if(pi){
        pi->setDirty(on);
    }
}
}  // namespace DA
