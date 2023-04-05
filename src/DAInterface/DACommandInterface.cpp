#include "DACommandInterface.h"
#include "DAAppUIInterface.h"
namespace DA
{
class DACommandInterfacePrivate
{
    DA_IMPL_PUBLIC(DACommandInterface)
public:
    DACommandInterfacePrivate(DACommandInterface* p);
    QUndoGroup _undoGroup;
};
}  // namespace DA
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DACommandInterfacePrivate
//===================================================
DACommandInterfacePrivate::DACommandInterfacePrivate(DACommandInterface* p) : q_ptr(p)
{
}
//===================================================
// DACommandInterface
//===================================================
DACommandInterface::DACommandInterface(DAAppUIInterface* u)
    : DABaseInterface(u->core(), u), d_ptr(new DACommandInterfacePrivate(this))
{
}

DACommandInterface::~DACommandInterface()
{
}

/**
 * @brief 获取undo/redoGroup
 * @return
 */
QUndoGroup& DACommandInterface::undoGroup()
{
    return d_ptr->_undoGroup;
}

/**
 * @brief 添加stack
 * @param stack
 */
void DACommandInterface::addStack(QUndoStack* stack)
{
    d_ptr->_undoGroup.addStack(stack);
}
