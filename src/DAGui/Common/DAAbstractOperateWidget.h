#ifndef DAABSTRACTOPERATEWIDGET_H
#define DAABSTRACTOPERATEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
class QUndoStack;
namespace DA {

/**
 * @brief 操作窗口的基类
 */
class DAGUI_API DAAbstractOperateWidget : public QWidget
{
	Q_OBJECT
public:
	DAAbstractOperateWidget(QWidget* par = nullptr);
	~DAAbstractOperateWidget();
	// 获取QUndoStack
	virtual QUndoStack* getUndoStack();
};

}
#endif  // DAABSTRACTOPERATEWIDGET_H
