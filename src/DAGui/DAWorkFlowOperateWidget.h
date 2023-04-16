#ifndef DAWORKFLOWOPERATEWIDGET_H
#define DAWORKFLOWOPERATEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAStandardGraphicsTextItem.h"
#include "DAWorkFlowGraphicsScene.h"
namespace Ui
{
class DAWorkFlowOperateWidget;
}
class QUndoStack;

namespace DA
{
class DAWorkFlowEditWidget;
class DADataWorkFlow;
class DAWorkFlowGraphicsView;
class DAAbstractNodeGraphicsItem;
/**
 * @brief 工作流绘图窗口
 */
class DAGUI_API DAWorkFlowOperateWidget : public QWidget
{
    Q_OBJECT

public:
    DAWorkFlowOperateWidget(QWidget* parent = nullptr);
    ~DAWorkFlowOperateWidget();
    // 创建工作流，创建完后通过getWorkflow获取
    // 如果对DAWorkFlow如果有继承，那么重载此函数创建自己的workflow就行
    virtual DAWorkFlow* createWorkflow();
    //创建工作流页面，会触发workflowCreated信号
    DAWorkFlowEditWidget* appendWorkflow(const QString& name);
    //获取当前工作流的索引
    int getCurrentWorkflowIndex() const;
    void setCurrentWorkflow(int index);
    //设置当前的工作流
    void setCurrentWorkflowWidget(DAWorkFlowEditWidget* wf);
    DAWorkFlowEditWidget* getCurrentWorkFlowWidget() const;
    void setCurrentWorkflowName(const QString& name);
    //获取当前场景
    DAWorkFlowGraphicsScene* getCurrentWorkFlowScene() const;
    QList< DAWorkFlowGraphicsScene* > getAllWorkFlowScene() const;
    //获取当前视图
    DAWorkFlowGraphicsView* getCurrentWorkFlowView() const;
    //获取工作流窗口
    DAWorkFlowEditWidget* getWorkFlowWidget(int index) const;
    //获取工作流窗口的名称
    QString getWorkFlowWidgetName(int index) const;
    //给工作流重命名
    void renameWorkFlowWidget(int index, const QString& name);
    //获取编辑窗口数量
    int count() const;
    //移除工作流，会进行销毁操作
    void removeWorkflow(int index);

    //激活UndoStack
    void setUndoStackActive();
    //设置显示grid
    void setEnableShowGrid(bool on);
    bool isEnableShowGrid() const;
    //获取QUndoStack
    QUndoStack* getUndoStack();
    //设置鼠标动作
    bool setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous);
    //清空所有工程
    void clear();
    //获取所有工作流的名字
    QList< QString > getAllWorkflowNames() const;
    //设置文本字体 -- 此参数设置决定创建文本框时的字体和颜色
    QFont getDefaultTextFont() const;
    void setDefaultTextFont(const QFont& f);
    //设置文本颜色 -- 此参数设置决定创建文本框时的字体和颜色
    QColor getDefaultTextColor() const;
    void setDefaultTextColor(const QColor& c);
    //创建一个新的工作流窗口，此函数带有交互
    DAWorkFlowEditWidget* appendWorkflowWithDialog();
public slots:
    //添加一个背景图
    void addBackgroundPixmap(const QString& pixmapPath);
    //锁定背景图
    void setBackgroundPixmapLock(bool on);
    //设置字体
    void setSelectTextFont(const QFont& f);
    //设置文字颜色
    void setSelectTextColor(const QColor& color);
    //设置当前选中的图元的背景
    void setSelectShapeBackgroundBrush(const QBrush& b);
    //设置当前选中的图元的背景
    void setSelectShapeBorderPen(const QPen& v);
    //设置当前工作流的网格显示与否
    void setCurrentWorkflowShowGrid(bool on);
    //设置当前工作流全部显示
    void setCurrentWorkflowWholeView();
    //设置当前工作流全部显示
    void setCurrentWorkflowZoomIn();
    //设置当前工作流全部显示
    void setCurrentWorkflowZoomOut();
    //运行工作流
    void runCurrentWorkFlow();
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
    void workflowCreated(DAWorkFlowEditWidget* wfw);
    /**
     * @brief 当前的工作流窗口发生了改变
     * @param w
     */
    void currentWorkFlowWidgetChanged(DAWorkFlowEditWidget* w);
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
    void workflowStartExecute(DAWorkFlowEditWidget* wfw);
    /**
     * @brief 执行到某个节点发射的信号
     * @param n
     */
    void nodeExecuteFinished(DAWorkFlowEditWidget* wfw, DAAbstractNode::SharedPointer n, bool state);

    /**
     * @brief 工作流执行完毕信号
     * @param success 成功全部执行完成为true
     */
    void workflowFinished(DAWorkFlowEditWidget* wfw, bool success);
private slots:
    //当前tab发生了改变
    void onTabWidgetCurrentChanged(int index);
    //请求关闭
    void onTabWidgetTabCloseRequested(int index);
    //选中改变
    void onSelectionChanged();

private:
    QList< DAStandardGraphicsTextItem* > getSelectTextItems();

private:
    Ui::DAWorkFlowOperateWidget* ui;
    bool _isShowGrid;
    QColor _defaultTextColor;
    QFont _defaultFont;
    bool _isDestorying;
};
}  // namespace DA
#endif  // DAWORKFLOWOPERATEWIDGET_H
