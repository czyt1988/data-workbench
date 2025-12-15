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
    ~DACommandInterface();
    // 获取DAAppUIInterface
    DAUIInterface* ui() const;
    // 获取undo/redoGroup
    QUndoGroup& undoGroup();
    // 添加stack
    void addStack(QUndoStack* stack);
    // 移除stack
    void removeStack(QUndoStack* stack);

    /**
     * @brief 开始一个数据操作命令，此命令会推入到当前激活的数据操作窗口的回退栈中
     * @param data
     * @param text 命令名字
     * @param isObjectPersist 是否把对象持久化到硬盘
     * @param isSkipFirstRedo 是否跳过第一次入栈的redo
     * @return
     */
    virtual DADataAbstractUndoCommand* beginDataOperateCommand(const DAData& data,
                                                               const QString& text,
                                                               bool isObjectPersist = false,
                                                               bool isSkipFirstRedo = true) = 0;
    /**
     * @brief 结束一个数据操作命令
     * @param data
     */
    virtual bool endDataOperateCommand(const DAData& data) = 0;
};
}  // namespace DA
#endif  // DACOMMANDINTERFACE_H
