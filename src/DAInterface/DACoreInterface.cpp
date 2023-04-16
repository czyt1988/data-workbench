#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>
namespace DA
{

DACoreInterface::DACoreInterface(QObject* parent) : QObject(parent)
{
}

DACoreInterface::~DACoreInterface()
{
}
}  // namespace DA
