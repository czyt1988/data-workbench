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
	// 撤销栈深度（005 T1.21 R19：DAGui 只暴露读写接口，配置读取/持久化在 APP 侧，
	// 严禁 DAGui→APP 反向依赖；0=无限制，QUndoStack 语义）
	int undoLimit() const;
	void setUndoLimit(int limit);
	// 激活此窗口的UndoStack
	void activeUndoStack();
	// 推入命令并激活本窗口的UndoStack
	void push(QUndoCommand* cmd);

protected:
	QUndoStack mUndoStack;
};
}  // end da
#endif  // DADATAOPERATEPAGEWIDGET_H
