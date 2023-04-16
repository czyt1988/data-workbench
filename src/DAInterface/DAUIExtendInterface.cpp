#include "DAUIExtendInterface.h"
#include "DAUIInterface.h"
namespace DA
{

//===================================================
// DAAppUIExtendInterface
//===================================================
DAUIExtendInterface::DAUIExtendInterface(DAUIInterface* u) : DABaseInterface(u->core(), u)
{
}

DAUIExtendInterface::~DAUIExtendInterface()
{
}

DAUIInterface* DAUIExtendInterface::ui() const
{
    return qobject_cast< DAUIInterface* >(parent());
}

}
