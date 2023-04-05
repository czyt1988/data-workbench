#ifndef DACOMMANDINTERFACE_H
#define DACOMMANDINTERFACE_H
#include <QObject>
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
#include <QUndoGroup>
class QUndoStack;

namespace DA
{
class DACoreInterface;
class DAAppUIInterface;
DA_IMPL_FORWARD_DECL(DACommandInterface)
/**
 * @brief 命令接口
 *
 * 程序自己维护QUndoStack，并注册到QUndoGroup中
 */
class DAINTERFACE_API DACommandInterface : public DABaseInterface
{
    Q_OBJECT
    DA_IMPL(DACommandInterface)
public:
    DACommandInterface(DAAppUIInterface* u);
    ~DACommandInterface();
    //获取DAAppUIInterface
    DAAppUIInterface* ui() const;
    //获取undo/redoGroup
    QUndoGroup& undoGroup();
    //添加stack
    void addStack(QUndoStack* stack);
};
}  // namespace DA
#endif  // DACOMMANDINTERFACE_H
