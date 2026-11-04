#ifndef DAAPPWORKFLOWACTIONHANDLER_H
#define DAAPPWORKFLOWACTIONHANDLER_H
#include <QObject>
#include <QList>
#include <memory>
#include "DAAppActionHandlerDeps.h"
#include "Workflow/DAPyWorkFlowGraphicsScene.h"

class QAction;
class QMenu;

namespace DA
{
class DAPyWorkFlowEditWidget;
class DAGraphicsScene;
class DAAbstractGraphicsSceneAction;
class DAPyWorkFlowOperateWidget;

/**
 * @brief 工作流域动作分发器：新建工作流、图元编辑、场景视图开关、运行与终止
 *
 * 从 DAAppController 抽出的分发器，只负责把动作转成对当前活动对象的调用；
 * 翻译上下文沿用 DA::DAAppController（.ts 译文已存在，拆类不改上下文）。
 */
class DAAppWorkflowActionHandler : public QObject
{
    Q_OBJECT
public:
    explicit DAAppWorkflowActionHandler(const DAAppActionHandlerDeps& deps, QObject* par = nullptr);
    // 建立本域的动作与信号绑定
    void initConnections();

public Q_SLOTS:
    // 鼠标动作结束
    void onWorkFlowGraphicsSceneActionDeactive(DA::DAAbstractGraphicsSceneAction* scAction);
    // 工作流页面创建槽
    void onWorkflowCreated(DA::DAPyWorkFlowEditWidget* wfw);
    // 当前工作流的字体变更
    void onCurrentWorkflowFontChanged(const QFont& f);
    void onCurrentWorkflowFontColorChanged(const QColor& c);
    // 图元的背景和框线变更
    void onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b);
    void onCurrentWorkflowShapeBorderPenChanged(const QPen& p);
    // 显示标记线 - 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在DAAppRibbonArea::buildContextCategoryWorkflowView函数中
    void onActionWorkflowViewMarkerTriggered(bool on);

private Q_SLOTS:
    // 添加背景图
    void onActionAddBackgroundPixmapTriggered();
    // 锁定背景图
    void onActionLockBackgroundPixmapTriggered(bool on);
    // 跟随背景图
    void onActionEnableItemMoveWithBackgroundTriggered(bool on);
    // 允许移动图元时，其它和此图元链接起来的图元跟随移动
    void onActionWorkflowEnableItemLinkageMoveTriggered(bool on);
    // 分组
    void onActionItemGroupingTriggered();
    // 取消分组
    void onActionItemUngroupTriggered();
    /////////---视图------
    // 导出png
    void onActionExportWorkflowScenePNGTriggered();
    // 锁定视图
    void onActionWorkflowViewReadOnlyTriggered(bool on);
    /////////---运行------
    // 运行
    void onActionRunCurrentWorkflowTriggered();
    // 终止
    void onActionTerminateCurrentWorkflowTriggered();
    void onWorkflowSceneSelectionItemChanged(QGraphicsItem* lastSelectItem);
    void onWorkflowStartExecute(DA::DAPyWorkFlowEditWidget* wfw);
    void onWorkflowFinished(DA::DAPyWorkFlowEditWidget* wfw, bool success);
    void onWorkflowSceneitemsAdded(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);
    void onWorkflowSceneitemsRemoved(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);
    void onCurrentWorkflowWidgetChanged(DA::DAPyWorkFlowEditWidget* wfw);
    void onActionNewWorkflowTriggered();
    // 绘制矩形
    void onActionStartDrawRectTriggered(bool on);
    // 绘制文本框
    void onActionStartDrawTextTriggered(bool on);
    // 允许连线
    void onActionWorkflowLinkEnableTriggered(bool on);

private:
    DAAppActionHandlerDeps mDeps;
};
}  // namespace DA

#endif  // DAAPPWORKFLOWACTIONHANDLER_H
