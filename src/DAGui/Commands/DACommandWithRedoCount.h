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
	// 计数加
	void addRedoCnt();
	// 计数减
	void subRedoCnt();
	// 计数是否等于2
	bool isEqualTwo() const;
	// 获取redocnt
	size_t getRedoCnt() const;
	// 设置成功
	bool isSuccess() const;
	// 设置成功
	void setSuccess(bool on = true);
	// 是否首次运行redo
	bool isFirstRunRedo();

protected:
	size_t m_redocnt;
	bool m_isSuccess;
};
}  // end of namespace DA
#endif  // DACOMMANDWITHREDOCOUNT_H
