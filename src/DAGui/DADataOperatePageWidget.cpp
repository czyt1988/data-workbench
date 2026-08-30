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
