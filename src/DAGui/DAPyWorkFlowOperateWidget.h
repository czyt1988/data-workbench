#ifndef DAPYWORKFLOWOPERATEWIDGET_H
#define DAPYWORKFLOWOPERATEWIDGET_H
#include <QWidget>
#include <QByteArray>
#include <functional>
#include "DAGuiAPI.h"
#include "DAAbstractOperateWidget.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAPyWorkFlowEditWidget.h"
class QUndoStack;
class QActionGroup;

namespace ads
{
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}

namespace DA
{
class DADataWorkFlow;
class DAPyWorkFlowManager;
class DAPyWorkFlowGraphicsView;
class DAPyNodeGraphicsItem;
class DAPyWorkFlowEditWidgetDockWidget;
/**
 * @brief 工作流绘图建模窗口
 *
 * 基于 Qt-Advanced-Docking-System (ADS) 的 dockindock 嵌套模式实现：内部持有一个嵌套的
 * ads::CDockManager，每个 DAPyWorkFlowEditWidget 包成 DAPyWorkFlowEditWidgetDockWidget
 * （纯 QWidget）后由 ads::CDockWidget 包装加入停靠区。工作流 dock 无法逃逸到 app 顶层
 * 管理器，满足"只能在本窗口内布局"。默认以标签形式加入当前聚焦 dock area，用户可自由
 * 拖拽分屏/并栏（禁浮动，保留 movable）。
 */
class DAGUI_API DAPyWorkFlowOperateWidget : public DAAbstractOperateWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowOperateWidget)
public:
    /**
     * @brief 场景操作的迭代函数指针，传入场景指针，返回false代表迭代中断，返回true代表迭代继续
     */
    using FpScenesOpt = std::function< bool(DAPyWorkFlowGraphicsScene*) >;

    /**
     * @brief DAPyWorkFlowEditWidget操作的迭代函数指针，传入DAPyWorkFlowEditWidget指针，返回false代表迭代中断，返回true代表迭代继续
     */
    using FpEditWidgetOpt = std::function< bool(DAPyWorkFlowEditWidget*) >;

public:
    /**
     * @brief DAPyWorkFlowOperateWidget窗口内部的action
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
        ActionZoomFit,
        ActionCrossLineMarker,  ///< 十字线
        ActionHLineMarker,      ///< 水平线
        ActionVLineMarker,      ///< 垂直线
        ActionNoneMarker,       ///< 无标记线
    };

public:
    DAPyWorkFlowOperateWidget(QWidget* parent = nullptr);
    ~DAPyWorkFlowOperateWidget();
    // 创建工作流管理器，子类可覆写以注入自定义 workflow 类型
    virtual DAPyWorkFlowManager* createManager();
    // 创建工作流页面，会触发workflowCreated信号
    // id 非空时用作工作流持久 id 与 dock objectName（供工程反序列化恢复布局）；为空时使用工作流自生成 uuid
    DAPyWorkFlowEditWidget* appendWorkflow(const QString& name, const QString& id = QString());
    // 创建一个新的工作流窗口，此函数带有交互
    DAPyWorkFlowEditWidget* appendWorkflowWithDialog();
    // 获取当前工作流的索引
    int getCurrentWorkflowIndex() const;
    void setCurrentWorkflow(int index);
    // 获取当前工作流管理器
    DAPyWorkFlowManager* getCurrentManager() const;
    // 兼容方法：返回当前 Manager 的 workflow
    DAPyWorkFlow getCurrentWorkflow() const;
    // 设置当前的工作流
    void setCurrentWorkflowWidget(DAPyWorkFlowEditWidget* wf);
    DAPyWorkFlowEditWidget* getCurrentWorkFlowWidget() const;
    void setCurrentWorkflowName(const QString& name);
    //
    // 获取所有的工作流编辑窗口
    QList< DAPyWorkFlowEditWidget* > getAllWorkFlowWidgets() const;
    // 获取当前场景
    DAPyWorkFlowGraphicsScene* getCurrentWorkFlowScene() const;
    QList< DAPyWorkFlowGraphicsScene* > getAllWorkFlowScene() const;
    // 获取当前视图
    DAPyWorkFlowGraphicsView* getCurrentWorkFlowView() const;
    // 获取工作流窗口
    DAPyWorkFlowEditWidget* getWorkFlowWidget(int index) const;
    // 按 id 查找工作流窗口
    DAPyWorkFlowEditWidget* findWorkFlowWidget(const QString& id) const;

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
    QUndoStack* getUndoStack() override;
    // 设置鼠标动作
    bool setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::SceneActionFlag mf);
    // 清空所有工程
    void clear();
    // 保存嵌套停靠区布局（供工程序列化），无停靠区时返回空
    QByteArray saveWorkFlowLayout() const;
    // 恢复嵌套停靠区布局（供工程反序列化），state 为空或失败返回 false
    bool restoreWorkFlowLayout(const QByteArray& state);
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
    // 设置当前视图的标记线
    void setCurrentViewLineMarker(DAGraphicsViewOverlayMouseMarker::MarkerStyle s);
    // 获取LineMarker的ActionGroup
    QActionGroup* getLineMarkerActionGroup() const;
public Q_SLOTS:
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
Q_SIGNALS:

    /**
     * @brief 选中了某个节点的设置窗口
     * @param w
     */
    void selectNodeItemChanged(DA::DAPyNodeGraphicsItem* i);

    /**
     * @brief 有新的工作流创建
     * @param wfw
     */
    void workflowCreated(DA::DAPyWorkFlowEditWidget* wfw);

    /**
     * @brief 工作流窗口正在移除，此时DA::DAPyWorkFlowEditWidget*指针还可正常操作
     *
     * 此时DAPyWorkFlowOperateWidget还管理着DA::DAPyWorkFlowEditWidget*，此信号之后将移除
     * @param wfw
     */
    void workflowRemoving(DA::DAPyWorkFlowEditWidget* wfw);

    /**
     * @brief 工作流在清空，不会发射workflowRemoving信号，而是发射workflowClearing信号
     */
    void workflowClearing();
    /**
     * @brief 当前的工作流窗口发生了改变
     * @param w
     */
    void currentWorkFlowWidgetChanged(DA::DAPyWorkFlowEditWidget* w);

    /**
     * @brief 场景动作激活
     * @param scAction 场景动作
     */
    void sceneActionActived(DA::DAAbstractGraphicsSceneAction* scAction);

    /**
     * @brief 场景动作取消
     * @param scAction 场景动作
     */
    void sceneActionDeactived(DA::DAAbstractGraphicsSceneAction* scAction);

    /**
     * @brief 选中的item改变发送的信号
     * @param lastSelectItem
     */
    void selectionItemChanged(QGraphicsItem* lastSelectItem);
    /**
     * @brief 开始执行，exec函数调用后会触发此信号
     */
    void workflowStartExecute(DA::DAPyWorkFlowEditWidget* wfw);
    /**
     * @brief 执行到某个节点发射的信号
     * @param wfw 工作流编辑窗口
     * @param n 节点代理（const引用）
     * @param state 执行状态
     */
    void nodeExecuteFinished(DA::DAPyWorkFlowEditWidget* wfw, const DA::DAPyNode& n, bool state);

    /**
     * @brief 工作流执行完毕信号
     * @param success 成功全部执行完成为true
     */
    void workflowFinished(DA::DAPyWorkFlowEditWidget* wfw, bool success);

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
private Q_SLOTS:
    // 嵌套停靠区聚焦 dock 改变（过滤掉非本管理器的顶层 dock）
    void onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock);
    // 选中改变
    void onSelectionChanged();
    //
    void onSceneItemsAdded(const QList< QGraphicsItem* >& its);
    void onSceneItemsRemoved(const QList< QGraphicsItem* >& its);
    //
    void onActionGroupViewLineMarkersTriggered(QAction* act);

private:
    // dock 关闭请求处理（经 closeRequested 信号触发，传入对应工作流编辑窗口）
    void onWorkflowCloseRequested(DAPyWorkFlowEditWidget* wfe);
    // 工作流标题改变槽函数（同步 dock 标签）
    void onWorkflowTitleChanged(const QString& t);
    // 实际移除工作流（不发确认框，不发 workflowRemoving），供 clear/关闭/移除共用
    void removeWorkflowNoConfirm(DAPyWorkFlowEditWidget* wfe, bool deleteWidget = true);
    QList< DAGraphicsStandardTextItem* > getSelectTextItems();
    // 初始化actions
    void initActions();
    // 把当前视图的标记线样式同步到 line-marker action group 的选中状态
    void syncLineMarkerActionForView(DAPyWorkFlowGraphicsView* view);
    //
    void retranslateUi();
};
}  // namespace DA
#endif  // DAPYWORKFLOWOPERATEWIDGET_H
