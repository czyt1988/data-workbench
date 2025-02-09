#ifndef DACOMMANDWITHREDOCOUNT_H
#define DACOMMANDWITHREDOCOUNT_H
#include "DAGuiAPI.h"
#include <QUndoCommand>
namespace DA
{
/**
 * @brief 加上计数的cmd
 * @code
 * auto cmd = new DACommandDataFrame_iat(...);
 * cmd->redo();
 *
 * void DACommandDataFrame_iat::redo()
 * {
 *     addRedoCnt();
 *     if (isEqualTwo()) {
 *         //第二次执行跳过，推入栈
 *         return;
 *     }
 *     。。。
 * }
 * @endcode
 *
 * 或者
 *
 * @code
 * auto cmd = new DACommandDataFrame_other(...);
 * cmd->isvalid();
 *
 * void DACommandDataFrame_other::redo()
 * {
 *     addRedoCnt();
 *     run();
 * }
 * @endcode
 */
class DAGUI_API DACommandWithRedoCount : public QUndoCommand
{
public:
	DACommandWithRedoCount(QUndoCommand* par = nullptr);
	~DACommandWithRedoCount();
	virtual void redo() override;
	/**
	 * @brief 执行函数，此函数返回false，说明执行失败，不应该被放入command stack中，在DACommandWithRedoCount中，exec函数应该替代redo函数
	 * @return
	 */
	virtual bool exec();

protected:
	bool mIsFirstRedo { true };
};
}  // end of namespace DA
#endif  // DACOMMANDWITHREDOCOUNT_H
