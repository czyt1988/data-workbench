﻿#ifndef DADATAOPERATEWIDGET_H
#define DADATAOPERATEWIDGET_H
#include <QWidget>
#include <QMap>
#include "DAGuiAPI.h"
#include "DAAbstractOperateWidget.h"
#include "DAData.h"

#include "DADataManager.h"
class QUndoStack;
namespace Ui
{
class DADataOperateWidget;
}
namespace DA
{
class DADataOperatePageWidget;
class DADataOperateOfDataFrameWidget;
class DADataManager;
/**
 * @brief 数据操作窗口，负责数据区域操作
 */
class DAGUI_API DADataOperateWidget : public DAAbstractOperateWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADataOperateWidget)
public:
	DADataOperateWidget(DADataManager* mgr, QWidget* parent = nullptr);
	DADataOperateWidget(QWidget* parent = nullptr);
	~DADataOperateWidget();

	//
	void setDataManager(DADataManager* mgr);
	// 当前显示的窗口
	QWidget* currentWidget() const;

	// 当前显示的DataFrame窗口，如果不是DataFrame窗口，返回nullptr
	DADataOperateOfDataFrameWidget* getCurrentDataFrameWidget() const;
	// 获取当前表格操作选中的数据，如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，如果用户选中了多列，那么每列作为一个DAData并组成list返回
	QList< DAData > getCurrentSelectDatas() const;

	// 获取QUndoStack
	QUndoStack* getUndoStack() override;
#if DA_ENABLE_PYTHON
	// 获取当前选中的Dataframe,如果用户在选中了列，返回选中的列索引
	std::pair< DAPyDataFrame, QList< int > > getCurrentSelectDataFrame() const;
#endif
public Q_SLOTS:
	// 显示一个数据
	void showData(const DA::DAData& d);
	// 删除tab窗口，同时删除tab标签和上次tab对应的widget
	bool removeTabWidget(QWidget* w);
Q_SIGNALS:
	/**
	 * @brief 数据操作页面添加信号
	 * @param page
	 */
	void dataTableCreated(DA::DADataOperatePageWidget* page);
	/**
	 * @brief 页面删除，此时可以操作此指针，此信号之后指针销毁
	 * @param page
	 */
	void dataTableRemoving(DA::DADataOperatePageWidget* page);
	/**
	 * @brief 当前的页面发生了改变
	 * @param w
	 * @param index
	 */
	void currentDataTableWidgetChanged(DA::DADataOperatePageWidget* page, int index);
private Q_SLOTS:
	// 数据删除对应的槽
	void onDataRemoved(const DA::DAData& d, int index);
	// 数据删除对应的槽
	void onDataChanged(const DA::DAData& d, DADataManager::ChangeType t);
	// tab标签切换
	void onTabWidgetCurrentChanged(int index);
	// tab关闭请求
	void onTabWidgetCloseRequested(int index);

private:
	void showDataframeData(const DA::DAData& d);
	void init();

private:
	Ui::DADataOperateWidget* ui;
};
}  // end of namespace DA
#endif  // DADATAOPERATEWIDGET_H
