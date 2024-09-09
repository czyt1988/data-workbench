#ifndef DAWORKFLOWOPERATEWIDGET_H
#define DAWORKFLOWOPERATEWIDGET_H
#include <QWidget>
#include <functional>
#include "DAGuiAPI.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAWorkFlowEditWidget.h"
namespace Ui
{
class DAWorkFlowOperateWidget;
}
class QUndoStack;

namespace DA
{
class DADataWorkFlow;
class DAWorkFlowGraphicsView;
class DAAbstractNodeGraphicsItem;
/**
 * @brief 工作流绘图建模窗口
 */
class DAGUI_API DAWorkFlowOperateWidget : public QWidget
{
	Q_OBJECT
public:
	/**
	 * @brief 场景操作的迭代函数指针，传入场景指针，返回false代表迭代中断，返回true代表迭代继续
	 */
	using FpScenesOpt = std::function< bool(DAWorkFlowGraphicsScene*) >;

	/**
	 * @brief DAWorkFlowEditWidget操作的迭代函数指针，传入DAWorkFlowEditWidget指针，返回false代表迭代中断，返回true代表迭代继续
	 */
	using FpEditWidgetOpt = std::function< bool(DAWorkFlowEditWidget*) >;

public:
	/**
	 * @brief DAWorkFlowOperateWidget窗口内部的action
	 */
	enum InnerActions
	{
		ActionCopy = 0,
		ActionCut,
		ActionPaste,
		ActionDelete,
		ActionCancel,
		ActionSelectAll,
		ActionZoomIn,
		ActionZoomOut,
		ActionZoomFit
	};

public:
	DAWorkFlowOperateWidget(QWidget* parent = nullptr);
	~DAWorkFlowOperateWidget();
	// 创建工作流，创建完后通过getWorkflow获取
	// 如果对DAWorkFlow如果有继承，那么重载此函数创建自己的workflow就行
	virtual DAWorkFlow* createWorkflow();
	// 创建工作流页面，会触发workflowCreated信号
	DAWorkFlowEditWidget* appendWorkflow(const QString& name);
	// 创建一个新的工作流窗口，此函数带有交互
	DAWorkFlowEditWidget* appendWorkflowWithDialog();
	// 获取当前工作流的索引
	int getCurrentWorkflowIndex() const;
	void setCurrentWorkflow(int index);
	DAWorkFlow* getCurrentWorkflow() const;
	// 设置当前的工作流
	void setCurrentWorkflowWidget(DAWorkFlowEditWidget* wf);
	DAWorkFlowEditWidget* getCurrentWorkFlowWidget() const;
	void setCurrentWorkflowName(const QString& name);
	//
	// 获取所有的工作流编辑窗口
	QList< DAWorkFlowEditWidget* > getAllWorkFlowWidgets() const;
	// 获取当前场景
	DAWorkFlowGraphicsScene* getCurrentWorkFlowScene() const;
	QList< DAWorkFlowGraphicsScene* > getAllWorkFlowScene() const;
	// 获取当前视图
	DAWorkFlowGraphicsView* getCurrentWorkFlowView() const;
	// 获取工作流窗口
	DAWorkFlowEditWidget* getWorkFlowWidget(int index) const;

	// 获取工作流窗口的名称
	QString getWorkFlowWidgetName(int index) const;
	// 给工作流重命名
	void renameWorkFlowWidget(int index, const QString& name);
	// 获取编辑窗口数量
	int count() const;
	// 移除工作流，会进行销毁操作
	void removeWorkflow(int index);

	// 激活UndoStack
	void setUndoStackActive();
	// 设置显示grid
	bool isCurrentWorkflowShowGrid() const;
	// 获取QUndoStack
	QUndoStack* getUndoStack();
	// 设置鼠标动作
	bool setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous);
	// 清空所有工程
	void clear();
	// 获取所有工作流的名字
	QList< QString > getAllWorkflowNames() const;
	// 设置文本字体 -- 此参数设置决定创建文本框时的字体和颜色
	QFont getDefaultTextFont() const;
	void setDefaultTextFont(const QFont& f);
	// 设置文本颜色 -- 此参数设置决定创建文本框时的字体和颜色
	QColor getDefaultTextColor() const;
	void setDefaultTextColor(const QColor& c);
	// 设置只允许一个工作流
	bool isOnlyOneWorkflow() const;
	void setOnlyOneWorkflow(bool v);
	// 获取窗口内置的action，一般这个函数用来把action设置到工具栏或者菜单中
	QAction* getInnerAction(InnerActions act);
	// 迭代场景操作
	void iteratorScene(FpScenesOpt fp);
public slots:
	// 添加一个背景图
	void addBackgroundPixmap(const QString& pixmapPath);
	// 锁定背景图
	void setBackgroundPixmapLock(bool on);
	// 设置字体
	void setSelectTextFont(const QFont& f);
	// 设置文字颜色
	void setSelectTextColor(const QColor& color);
	// 设置当前选中的图元的背景
	void setSelectShapeBackgroundBrush(const QBrush& b);
	// 设置当前选中的图元的背景
	void setSelectShapeBorderPen(const QPen& v);
	// 设置当前工作流的网格显示与否
	void setCurrentWorkflowShowGrid(bool on);
	// 设置当前工作流锁定
	void setCurrentWorkflowReadOnly(bool on);
	// 设置当前工作流全部显示
	void setCurrentWorkflowWholeView();
	// 设置当前工作流全部显示
	void setCurrentWorkflowZoomIn();
	// 设置当前工作流全部显示
	void setCurrentWorkflowZoomOut();
	// 全选
	void setCurrentWorkflowSelectAll();
	// 运行工作流
	void runCurrentWorkFlow();
	// 终止工作流
	void terminateCurrentWorkFlow();
	// 复制当前选中的items
	void copyCurrentSelectItems();
	// 剪切当前选中的items
	void cutCurrentSelectItems();
	// 重剪切板粘贴
	void pasteFromClipBoard();
	// 删除当前的item
	void removeCurrentSelectItems();
	// 当前的wf执行取消动作
	void cancelCurrent();
	// 设置是否允许连接
	void setEnableWorkflowLink(bool on);
	bool isEnableWorkflowLink() const;
signals:

	/**
	 * @brief 选中了某个节点的设置窗口
	 * @param w
	 */
	void selectNodeItemChanged(DA::DAAbstractNodeGraphicsItem* i);

	/**
	 * @brief 有新的工作流创建
	 * @param wfw
	 */
	void workflowCreated(DA::DAWorkFlowEditWidget* wfw);

	/**
	 * @brief 工作流窗口正在移除，此时DA::DAWorkFlowEditWidget*指针还可正常操作
	 *
	 * 此时DAWorkFlowOperateWidget还管理着DA::DAWorkFlowEditWidget*，此信号之后将移除
	 * @param wfw
	 */
	void workflowRemoving(DA::DAWorkFlowEditWidget* wfw);

	/**
	 * @brief 工作流在清空，不会发射workflowRemoving信号，而是发射workflowClearing信号
	 */
	void workflowClearing();
	/**
	 * @brief 当前的工作流窗口发生了改变
	 * @param w
	 */
	void currentWorkFlowWidgetChanged(DA::DAWorkFlowEditWidget* w);
	/**
	 * @brief 鼠标动作已经执行完毕
	 * @param mf 已经执行完的鼠标动作
	 */
	void mouseActionFinished(DA::DAWorkFlowGraphicsScene::MouseActionFlag mf);

	/**
	 * @brief 选中的item改变发送的信号
	 * @param lastSelectItem
	 */
	void selectionItemChanged(QGraphicsItem* lastSelectItem);
	/**
	 * @brief 开始执行，exec函数调用后会触发此信号
	 */
	void workflowStartExecute(DA::DAWorkFlowEditWidget* wfw);
	/**
	 * @brief 执行到某个节点发射的信号
	 * @param n
	 */
	void nodeExecuteFinished(DA::DAWorkFlowEditWidget* wfw, DA::DAAbstractNode::SharedPointer n, bool state);

	/**
	 * @brief 工作流执行完毕信号
	 * @param success 成功全部执行完成为true
	 */
	void workflowFinished(DA::DAWorkFlowEditWidget* wfw, bool success);

	/**
	 * @brief item添加的信号
	 *
	 * @note 此信号是通过@ref DAGraphicsScene::addItem_ 或是@ref DAGraphicsScene::addItems_ 函数才会触发，
	 * 直接调用@ref QGraphicsScene::addItem 函数不会触发此函数
	 * @param sc
	 * @param item
	 */
	void itemsAdded(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);

	/**
	 * @brief item移除的信号
	 *
	 * @note 此信号是通过@ref DAGraphicsScene::removeItem_ 或是@ref DAGraphicsScene::removeItems_ 函数才会触发，
	 * 直接调用@ref QGraphicsScene::removeItem 函数不会触发此函数
	 * @param sc
	 * @param item
	 */
	void itemsRemoved(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);
private slots:
	// 当前tab发生了改变
	void onTabWidgetCurrentChanged(int index);
	// 请求关闭
	void onTabWidgetTabCloseRequested(int index);
	// 选中改变
	void onSelectionChanged();
	//
	void onSceneItemsAdded(const QList< QGraphicsItem* >& its);
	void onSceneItemsRemoved(const QList< QGraphicsItem* >& its);

private:
	QList< DAGraphicsStandardTextItem* > getSelectTextItems();
	// 初始化actions
	void initActions();
	//
	void retranslateUi();

private:
	Ui::DAWorkFlowOperateWidget* ui;
	bool mIsShowGrid;
	QColor mDefaultTextColor;
	QFont mDefaultFont;
	bool mIsDestorying;
	bool mOnlyOneWorkflow { false };    ///< 设置只允许一个工作流
	bool mEnableWorkflowLink { true };  ///< 是否允许工作流连接
	QAction* mActionCopy { nullptr };
	QAction* mActionCut { nullptr };
	QAction* mActionPaste { nullptr };
	QAction* mActionDelete { nullptr };     ///< 删除选中
	QAction* mActionCancel { nullptr };     ///< 取消动作
	QAction* mActionSelectAll { nullptr };  ///< 全选
	QAction* mActionZoomIn { nullptr };     ///< 放大
	QAction* mActionZoomOut { nullptr };    ///< 缩小
	QAction* mActionZoomFit { nullptr };    ///< 全部显示
};
}  // namespace DA
#endif  // DAWORKFLOWOPERATEWIDGET_H
