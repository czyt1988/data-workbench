#include "DACommandInterface.h"
#include "DAUIInterface.h"
namespace DA
{
class DACommandInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DACommandInterface)
public:
    PrivateData(DACommandInterface* p);
    QUndoGroup mUndoGroup;
    DAUIInterface* ui { nullptr };
};

//===================================================
// DACommandInterfacePrivate
//===================================================
DACommandInterface::PrivateData::PrivateData(DACommandInterface* p) : q_ptr(p)
{
}
//===================================================
// DACommandInterface
//===================================================
DACommandInterface::DACommandInterface(DAUIInterface* u) : DABaseInterface(u->core(), u), DA_PIMPL_CONSTRUCT
{
    d_ptr->ui = u;
}

DACommandInterface::~DACommandInterface()
{
}

DAUIInterface* DACommandInterface::ui() const
{
    return d_ptr->ui;
}

/**
 * @brief 获取undo/redoGroup
 * @return
 */
QUndoGroup& DACommandInterface::undoGroup()
{
    return d_ptr->mUndoGroup;
}

/**
 * @brief 添加stack
 * @param stack
 */
void DACommandInterface::addStack(QUndoStack* stack)
{
    d_ptr->mUndoGroup.addStack(stack);
}

void DACommandInterface::removeStack(QUndoStack* stack)
{
    d_ptr->mUndoGroup.removeStack(stack);
}
}  // namespace DA
