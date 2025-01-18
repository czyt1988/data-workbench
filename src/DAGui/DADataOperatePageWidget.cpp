#include "DADataOperatePageWidget.h"
namespace DA
{
DADataOperatePageWidget::DADataOperatePageWidget(QWidget* par) : QWidget(par)
{
	mUndoStack.setUndoLimit(20);
	mUndoStack.setActive();
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
}  // end DA
