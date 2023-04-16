#include "DABaseInterface.h"
#include "DACoreInterface.h"

namespace DA
{
//===================================================
// DABaseInterfacePrivate
//===================================================
class DABaseInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DABaseInterface)
public:
    PrivateData(DABaseInterface* p, DACoreInterface* c);

    DACoreInterface* mCore;
};

DABaseInterface::PrivateData::PrivateData(DABaseInterface* p, DACoreInterface* c) : q_ptr(p), mCore(c)
{
}
//===================================================
// DABaseInterface
//===================================================
DABaseInterface::DABaseInterface(DACoreInterface* c, QObject* par)
    : QObject(par), d_ptr(std::make_unique< DABaseInterface::PrivateData >(this, c))
{
}

DABaseInterface::~DABaseInterface()
{
}

DACoreInterface* DABaseInterface::core() const
{
    return d_ptr->mCore;
}
}  // namespace DA
