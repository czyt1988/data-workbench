#ifndef DACOMMANDWITHREDOCOUNT_H
#define DACOMMANDWITHREDOCOUNT_H
#include "DAGuiAPI.h"
#include <QUndoCommand>
namespace DA
{
/**
 * @brief 首次redo跳过的命令基类
 * TODO:这个类的名字需要修改为DACommandInitialSkipRedo
 *
 * 子类应重写 exec() 实现实际的 redo 逻辑，而不是重写 redo()。
 * 第一次调用 redo() 时会跳过 exec()（mIsFirstRedo 为 true），
 * 之后的 redo() 调用会执行 exec()。
 *
 * 典型用法：
 * @code
 * auto cmd = new DACommandDataFrame_iat(...);
 * // 构造时已执行实际操作，cmd->exec() 在第一次 redo() 时被跳过
 * undoStack->push(cmd);  // push 会调用 redo()，但第一次跳过 exec()
 * @endcode
 */
class DAGUI_API DACommandWithRedoCount : public QUndoCommand
{
public:
	DACommandWithRedoCount(QUndoCommand* par = nullptr);
	~DACommandWithRedoCount();
	virtual void redo() override;
	// 执行函数，此函数返回false，说明执行失败，不应该被放入command stack中，在DACommandWithRedoCount中，exec函数应该替代redo函数
	virtual bool exec();

protected:
	bool mIsFirstRedo { true };
};
}  // end of namespace DA
#endif  // DACOMMANDWITHREDOCOUNT_H
