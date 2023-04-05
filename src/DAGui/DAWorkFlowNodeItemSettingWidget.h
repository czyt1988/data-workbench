#ifndef DAWORKFLOWNODEITEMSETTINGWIDGET_H
#define DAWORKFLOWNODEITEMSETTINGWIDGET_H
#include <QWidget>
#include <QTabWidget>
#include <QPointer>
#include "DAGuiAPI.h"
#include "DANodeSettingWidget.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DANodeItemSettingWidget.h"
#include "DANodeLinkItemSettingWidget.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
namespace Ui
{
class DAWorkFlowNodeItemSettingWidget;
}
namespace DA
{
class DAWorkFlowEditWidget;
class DAWorkFlowOperateWidget;
/**
 * @brief 节点设置窗口
 */
class DAGUI_API DAWorkFlowNodeItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAWorkFlowNodeItemSettingWidget(QWidget* parent = nullptr);
    ~DAWorkFlowNodeItemSettingWidget();

    //添加新窗体
    void addWidget(QWidget* w, const QIcon& icon, const QString& title);
    void removeWidget(QWidget* w);
    // tab窗体
    QTabWidget* tabWidget();
    //设置场景
    void setWorkFlowOperateWidget(DAWorkFlowOperateWidget* wf);
    //设置节点设置可用
    void setNodeSettingEnable(bool on = true);
    //设置item设置可用
    void setItemSettingEnable(bool on = true);
    //设置link设置可用
    void setLinkSettingEnable(bool on = true);
    //获取当前的scene
    DAWorkFlowGraphicsScene* getCurrentScene() const;
private slots:
    //选择改变
    void onSceneSelectionChanged();
    //条目的位置改变触发的槽
    void onSceneItemsPositionChanged(const QList< QGraphicsItem* >& items, const QList< QPointF >& oldPos, const QList< QPointF >& newPos);
    //条目bodysize改变触发的信号
    void onSceneItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize);
    //条目item旋转发出的信号
    void onSceneItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation);
    //记录tab最后一个index
    void onTabBarCurrentIndexChanged(int index);
    //编辑窗口发生变更
    void onWorkFlowEditWidgetChanged(DAWorkFlowEditWidget* w);

private:
    void init();
    void bindWorkFlowEditWidget(DAWorkFlowEditWidget* w);

private:
    Ui::DAWorkFlowNodeItemSettingWidget* ui;
    QPointer< DAWorkFlowOperateWidget > _workflowOptWidget;
    QPointer< DAWorkFlowEditWidget > _workflowEditWidget;
    int _lastTabIndex;
};
}  // namespace DA
#endif  // DAWORKFLOWNODEITEMSETTINGWIDGET_H
