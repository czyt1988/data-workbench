#include "DABaseInterface.h"
#include "DACoreInterface.h"

namespace DA
{

class DABaseInterfacePrivate
{
    DA_IMPL_PUBLIC(DABaseInterface)
public:
    DABaseInterfacePrivate(DABaseInterface* p, DACoreInterface* c);

    DACoreInterface* _core;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DABaseInterfacePrivate
//===================================================

DABaseInterfacePrivate::DABaseInterfacePrivate(DABaseInterface* p, DACoreInterface* c) : q_ptr(p), _core(c)
{
}
//===================================================
// DABaseInterface
//===================================================
DABaseInterface::DABaseInterface(DACoreInterface* c, QObject* par)
    : QObject(par), d_ptr(new DABaseInterfacePrivate(this, c))
{
}

DABaseInterface::~DABaseInterface()
{
}

DACoreInterface* DABaseInterface::core() const
{
    return d_ptr->_core;
}
