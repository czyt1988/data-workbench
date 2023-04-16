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
class DAUIInterface;
/**
 * @brief 命令接口
 *
 * 程序自己维护QUndoStack，并注册到QUndoGroup中
 */
class DAINTERFACE_API DACommandInterface : public DABaseInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DACommandInterface)
public:
    DACommandInterface(DAUIInterface* u);
    ~DACommandInterface();
    //获取DAAppUIInterface
    DAUIInterface* ui() const;
    //获取undo/redoGroup
    QUndoGroup& undoGroup();
    //添加stack
    void addStack(QUndoStack* stack);
};
}  // namespace DA
#endif  // DACOMMANDINTERFACE_H
