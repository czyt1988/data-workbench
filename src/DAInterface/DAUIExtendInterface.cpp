#include "DAUIExtendInterface.h"
#include "DAUIInterface.h"
namespace DA
{

//===================================================
// DAAppUIExtendInterface
//===================================================
DAUIExtendInterface::DAUIExtendInterface(DAUIInterface* u) : DABaseInterface(u->core(), u), mUI(u)
{
}

DAUIExtendInterface::~DAUIExtendInterface()
{
}

DAUIInterface* DAUIExtendInterface::ui() const
{
    return mUI;
    // return qobject_cast< DAUIInterface* >(parent());
}

}
