#ifndef DADATAOPERATEPAGEWIDGET_H
#define DADATAOPERATEPAGEWIDGET_H
#include <QWidget>
#include <QUndoStack>
#include "DAGuiAPI.h"
namespace DA
{
/**
 * @brief 数据操作的基类窗口，所有数据操作页面继承此窗口方便管理
 */
class DAGUI_API DADataOperatePageWidget : public QWidget
{
	Q_OBJECT
public:
	enum DataOperatePageType
	{
		DataOperateOfDataFrame  = 1,
		DataOperateOfUserDefine = 1000
	};

public:
	DADataOperatePageWidget(QWidget* par = nullptr);
	~DADataOperatePageWidget();
	virtual int getDataOperatePageType() const = 0;
	QUndoStack* getUndoStack();
	// 激活此窗口的UndoStack
	void activeUndoStack();

protected:
	QUndoStack mUndoStack;
};
}  // end da
#endif  // DADATAOPERATEPAGEWIDGET_H
