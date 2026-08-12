#ifndef DACOMMANDINTERFACE_H
#define DACOMMANDINTERFACE_H
#include <QObject>
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
#include <QUndoGroup>
#include <QUndoCommand>
#include "DAData.h"
class QUndoStack;

namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DADataAbstractUndoCommand;
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
    virtual ~DACommandInterface() override;
    // 获取DAAppUIInterface
    DAUIInterface* ui() const;
    // 获取undo/redoGroup
    QUndoGroup& undoGroup();
    // 添加stack
    void addStack(QUndoStack* stack);
    // 移除stack
    void removeStack(QUndoStack* stack);

    // 开始一个数据操作命令，此命令会推入到当前激活的数据操作窗口的回退栈中
    virtual DADataAbstractUndoCommand* beginDataOperateCommand(const DAData& data,
                                                               const QString& text,
                                                               bool isObjectPersist = false,
                                                               bool isSkipFirstRedo = true) = 0;
    // 结束一个数据操作命令
    virtual bool endDataOperateCommand(const DAData& data) = 0;
};
}  // namespace DA
#endif  // DACOMMANDINTERFACE_H
