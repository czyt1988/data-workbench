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
 * 数据/绘图操作窗口在没有任何内容时显示统一引导页；引导内容随数据管理器是否已有数据切换：
 * 没有数据时给「打开已有工程/导入数据」，已有数据时数据窗口提示如何打开数据表、
 * 绘图窗口给「添加绘图」。子类创建好主内容后调用 @ref setupNoDataGuide 安装
 * 「引导页 + 内容页」堆叠容器，再按内容数量切换显隐。
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
	// 设置数据管理器是否已有数据（引导页据此切换入口：导入数据 / 添加绘图 / 数据表提示）
	void setNoDataGuideDataAvailable(bool available);

Q_SIGNALS:
	// 引导页「打开已有工程」入口被点击
	void openProjectRequested();
	// 引导页「导入数据」入口被点击
	void importDataRequested();
	// 引导页「添加绘图」入口被点击（已有数据但还没有绘图时）
	void addFigureRequested();

protected:
	// 安装「无数据引导页 + 内容页」堆叠容器（子类创建主内容后调用一次）
	void setupNoDataGuide(QWidget* content, DANoDataGuideWidget* guide);

private:
	QStackedWidget* mOperateStack { nullptr };
	QWidget* mOperateContent { nullptr };
	DANoDataGuideWidget* mNoDataGuide { nullptr };
	bool mDataAvailable { false };
};

}
#endif  // DAABSTRACTOPERATEWIDGET_H
