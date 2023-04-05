#include "DACoreInterface.h"
#include "DAAppUIInterface.h"
#include <QPointer>
namespace DA
{
class DACoreInterfacePrivate
{
    DA_IMPL_PUBLIC(DACoreInterface)
public:
    DACoreInterfacePrivate(DACoreInterface* p);

public:
    QPointer< DAAppUIInterface > _uiInterface;
};
}  // namespace DA
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DACoreInterfacePrivate
//===================================================
DACoreInterfacePrivate::DACoreInterfacePrivate(DACoreInterface* p) : q_ptr(p)
{
}

DACoreInterface::DACoreInterface(QObject* parent) : QObject(parent), d_ptr(new DACoreInterfacePrivate(this))
{
}

DACoreInterface::~DACoreInterface()
{
}
