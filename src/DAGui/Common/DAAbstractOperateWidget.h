#ifndef DAABSTRACTOPERATEWIDGET_H
#define DAABSTRACTOPERATEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
class QUndoStack;
class QStackedWidget;
namespace DA {

class DANoDataGuideWidget;

/**
 * @brief 操作窗口的基类
 *
 * 数据/绘图操作窗口在没有任何内容时显示统一的「尚未载入数据」引导页
 * （打开已有工程 / 导入数据两个入口）；子类创建好主内容后调用
 * @ref setupNoDataGuide 安装「引导页 + 内容页」堆叠容器，再按内容数量切换显隐。
 */
class DAGUI_API DAAbstractOperateWidget : public QWidget
{
	Q_OBJECT
public:
	DAAbstractOperateWidget(QWidget* par = nullptr);
	~DAAbstractOperateWidget();
	// 获取QUndoStack
	virtual QUndoStack* getUndoStack();

	// 无数据引导页是否可见
	bool isNoDataGuideVisible() const;
	// 设置无数据引导页显隐（子类内容变化后调用）
	void setNoDataGuideVisible(bool visible);

Q_SIGNALS:
	// 引导页「打开已有工程」入口被点击
	void openProjectRequested();
	// 引导页「导入数据」入口被点击
	void importDataRequested();

protected:
	// 安装「无数据引导页 + 内容页」堆叠容器（子类创建主内容后调用一次）
	void setupNoDataGuide(QWidget* content, DANoDataGuideWidget* guide);

private:
	QStackedWidget* mOperateStack { nullptr };
	QWidget* mOperateContent { nullptr };
	DANoDataGuideWidget* mNoDataGuide { nullptr };
};

}
#endif  // DAABSTRACTOPERATEWIDGET_H
