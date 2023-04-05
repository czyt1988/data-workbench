#include "DAAppUIExtendInterface.h"
#include "DAAppUIInterface.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppUIExtendInterface
//===================================================
DAAppUIExtendInterface::DAAppUIExtendInterface(DAAppUIInterface* u) : DABaseInterface(u->core(), u)
{
}

DAAppUIExtendInterface::~DAAppUIExtendInterface()
{
}

DAAppUIInterface* DAAppUIExtendInterface::ui() const
{
    return qobject_cast< DAAppUIInterface* >(parent());
}
