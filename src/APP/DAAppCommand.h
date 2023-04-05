#ifndef DAAPPCOMMAND_H
#define DAAPPCOMMAND_H
#include "DACommandInterface.h"
#include <QUndoStack>
#include <QPointer>

namespace DA
{
/**
 * @brief App的命令管理
 *
 * 所有的固定stack都从这里能获取到，一些非固定stack不进行管理
 *
 * QUndoGroup管理所有的栈
 */
class DAAppCommand : public DACommandInterface
{
    Q_OBJECT
public:
    DAAppCommand(DAAppUIInterface* u);
    ~DAAppCommand();

public:
    //数据管理的redo/undo栈
    QUndoStack* getDataManagerStack() const;
    // TODO:工作流的redo/undo栈
    // TODO:绘图的redo/undo栈（每个图片一个栈）
public:
    void setDataManagerStack(QUndoStack* s);

private:
    QPointer< QUndoStack > m_dataManagerStack;
};
}  // namespace DA

#ifndef DA_APP_COMMAND
/**
 * @def 获取@sa DAAppCommand 实例
 *
 * @note 需要一下include：
 * @code
 * #include "DAAppCore.h"
 * #include "DAAppUI.h"
 * @endcode
 *
 */
#define DA_APP_COMMAND DA::DAAppCore::getInstance().getUi()->getAppCmd()
#endif

#endif  // DAAPPCOMMAND_H
