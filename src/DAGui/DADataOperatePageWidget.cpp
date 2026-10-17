#include "DADataOperatePageWidget.h"
namespace DA
{
DADataOperatePageWidget::DADataOperatePageWidget(QWidget* par) : QWidget(par)
{
	mUndoStack.setUndoLimit(20);
}

DADataOperatePageWidget::~DADataOperatePageWidget()
{
}

QUndoStack* DADataOperatePageWidget::getUndoStack()
{
	return &mUndoStack;
}

/**
 * @brief 撤销栈深度上限（005 T1.21）
 * @return 当前上限（0=无限制）
 */
int DADataOperatePageWidget::undoLimit() const
{
	return mUndoStack.undoLimit();
}

/**
 * @brief 设置撤销栈深度上限（005 T1.21 R19：默认 20，历史面板可调大；
 * 配置读写与持久化在 APP 侧，本接口只是施加口径）
 * @param limit 上限（0=无限制；调大风险主要来自 Swap 类命令的旧对象驻留内存）
 */
void DADataOperatePageWidget::setUndoLimit(int limit)
{
	mUndoStack.setUndoLimit(qMax(0, limit));
}

void DADataOperatePageWidget::activeUndoStack()
{
	if (!getUndoStack()->isActive()) {
		getUndoStack()->setActive();
	}
}

/**
 * @brief 推入一个命令并激活本页的undo栈
 *
 * push即激活，保证命令进入的栈就是QUndoGroup的active栈，
 * 全局undo/redo action状态立即反映本页的操作
 * @param cmd 待推入的命令
 */
void DADataOperatePageWidget::push(QUndoCommand* cmd)
{
	if (!cmd) {
		return;
	}
	mUndoStack.push(cmd);
	activeUndoStack();
}
}  // end DA
