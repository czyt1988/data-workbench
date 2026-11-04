/**
 * @file DAAppRibbonAreaWorkflow.cpp
 * @brief DAAppRibbonArea 的「工作流」上下文标签页实现
 *
 * 负责 workflow 上下文下的三张页面（objectName 见 DAUiObjectNames.h）：
 * - 工作流视图（da-ribbon-category-workflow.view）：锁定、网格、缩放适应、视图标记、导出
 * - 工作流编辑（da-ribbon-category-workflow.edit）：剪贴板、图元、文本、背景、分组
 * - 工作流运行（da-ribbon-category-workflow.run）：运行、停止
 *
 * 另外包含工作流相关的状态同步（标记线 action 的图标/勾选）与
 * WorkFlowEdit 画笔/画刷/字体的读写实现。
 *
 * 依赖注入：父容器与 DAPyWorkFlowOperateWidget* 全部入参，本文件不读 mDockArea。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
#include "AppMainWindow.h"
// Qt
#include <QActionGroup>
#include <QIcon>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonMenu.h"
// Widget
#include "DAFontEditPannelWidget.h"
#include "DAShapeEditPannelWidget.h"
// Workflow
#include "Workflow/DAPyWorkFlowOperateWidget.h"
#include "Workflow/DAPyWorkFlowEditWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsView.h"
// api
#include "DAAppActions.h"
#include "DAAppDockingArea.h"

using namespace DA;

/**
 * @brief WorkFlowEdit 画笔/画刷/字体的读写实现
 *
 * 把「工作流编辑页两个编辑面板」的读写收敛成一组 getter/setter，
 * 供 DAAppController 在选中图元变化时回灌界面（写入时阻塞信号，避免回环）。
 */
#define DAAPPRIBBONAREA_COMMON_SETTING_CPP(MiddleName, ShapeEditPannelWidget, FontEditWidget)                          \
    QPen DAAppRibbonArea::get##MiddleName##Pen() const                                                                 \
    {                                                                                                                  \
        return ShapeEditPannelWidget->getBorderPen();                                                                  \
    }                                                                                                                  \
    QBrush DAAppRibbonArea::get##MiddleName##Brush() const                                                             \
    {                                                                                                                  \
        return ShapeEditPannelWidget->getBackgroundBrush();                                                            \
    }                                                                                                                  \
    QFont DAAppRibbonArea::get##MiddleName##Font() const                                                               \
    {                                                                                                                  \
        return FontEditWidget->getCurrentFont();                                                                       \
    }                                                                                                                  \
    QColor DAAppRibbonArea::get##MiddleName##FontColor() const                                                         \
    {                                                                                                                  \
        return FontEditWidget->getCurrentFontColor();                                                                  \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Pen(const QPen& v)                                                          \
    {                                                                                                                  \
        QSignalBlocker b(ShapeEditPannelWidget);                                                                       \
        ShapeEditPannelWidget->setBorderPen(v);                                                                        \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Brush(const QBrush& v)                                                      \
    {                                                                                                                  \
        QSignalBlocker b(ShapeEditPannelWidget);                                                                       \
        ShapeEditPannelWidget->setBackgroundBrush(v);                                                                  \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Font(const QFont& v)                                                        \
    {                                                                                                                  \
        QSignalBlocker b(FontEditWidget);                                                                              \
        FontEditWidget->setCurrentFont(v);                                                                             \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##FontColor(const QColor& v)                                                  \
    {                                                                                                                  \
        QSignalBlocker b(FontEditWidget);                                                                              \
        FontEditWidget->setCurrentFontColor(v);                                                                        \
    }

DAAPPRIBBONAREA_COMMON_SETTING_CPP(WorkFlowEdit, mWorkflowShapeEditPannelWidget, mWorkflowFontEditPannel)

/**
 * @brief 构建工作流上下文标签
 * objectname=da-ribbon-contextcategory-workflow
 *
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 * @note 三页必须按 视图 -> 编辑 -> 运行 的顺序构建，顺序决定标签页的排列。
 */
void DAAppRibbonArea::buildContextCategoryWorkflow(SARibbonBar* ribbon)
{
    mContextWorkflow = ribbon->addContextCategory(tr("Workflow"));  // cn:工作流
    mContextWorkflow->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowContextCategory));

    // 工作流操作窗口是这三页动作的来源（缩放、标记线、剪贴板等内建 action）；
    // 未就绪时整个工作流上下文不构建（此时也没有工作流可编辑）
    DAPyWorkFlowOperateWidget* wfo = mDockArea ? mDockArea->getWorkFlowOperateWidget() : nullptr;
    if (!wfo) {
        qWarning() << "DAAppRibbonArea::buildContextCategoryWorkflow: workflow operate widget is not ready";
        return;
    }
    buildContextCategoryWorkflowView(mContextWorkflow, wfo);
    buildContextCategoryWorkflowEdit(mContextWorkflow, wfo);
    buildContextCategoryWorkflowRun(mContextWorkflow);

    ribbon->showContextCategory(mContextWorkflow);
}

/**
 * @brief 构建「工作流视图」页
 * objectname=da-ribbon-category-workflow.view
 *
 * 面板：视图（锁定视图、显示网格、缩放适应/放大/缩小、视图标记下拉）、导出。
 * @param contextWorkflow 工作流上下文（由 buildContextCategoryWorkflow 创建）
 * @param wfo 工作流操作窗口，提供缩放与标记线内建 action（由 buildContextCategoryWorkflow 保证非空）
 */
void DAAppRibbonArea::buildContextCategoryWorkflowView(SARibbonContextCategory* contextWorkflow,
                                                       DAPyWorkFlowOperateWidget* wfo)
{
    mCategoryWorkflowGraphicsView = contextWorkflow->addCategoryPage(tr("Workflow View"));  // cn:工作流视图
    mCategoryWorkflowGraphicsView->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowViewCategory));
    // 视图面板：锁定视图、显示网格、缩放适应、放大、缩小、视图标记（下拉）
    mPannelWorkflowView = mCategoryWorkflowGraphicsView->addPanel(tr("View"));  // cn:视图
    mPannelWorkflowView->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowViewPanel));
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowViewReadOnly);
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowShowGrid);
    mPannelWorkflowView->addSeparator();
    mPannelWorkflowView->addLargeAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomFit));
    mPannelWorkflowView->addMediumAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomIn));
    mPannelWorkflowView->addMediumAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomOut));

    mMenus.viewLineMarkers = new SARibbonMenu(mApp);
    mMenus.viewLineMarkers->setObjectName(QString::fromUtf8(UiNames::Menu::ViewLineMarkers));
    mMenus.viewLineMarkers->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
    mMenus.viewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCrossLineMarker));
    mMenus.viewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionHLineMarker));
    mMenus.viewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionVLineMarker));
    mMenus.viewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker));

    // 标记线菜单被选中时，把对应图标与勾选状态同步到「视图标记」大按钮上
    if (QActionGroup* ag = wfo->getLineMarkerActionGroup()) {
        connect(ag, &QActionGroup::triggered, this, [ this, wfo ](QAction* act) {
            QAction* actMarker = mActions->actionWorkflowViewMarker;
            QAction* noneAct   = wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker);
            if (act != noneAct && act->isChecked()) {
                actMarker->setIcon(act->icon());
                actMarker->setChecked(true);
            } else {
                actMarker->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
                actMarker->setChecked(false);
            }
        });
    }
    mActions->actionWorkflowViewMarker->setMenu(mMenus.viewLineMarkers);
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowViewMarker, QToolButton::MenuButtonPopup);

    // 导出面板：导出为图片（下拉选择 PNG 等格式）
    mPannelWorkflowExport = mCategoryWorkflowGraphicsView->addPanel(tr("Export"));  // cn:导出
    mPannelWorkflowExport->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowExportPanel));
    mPannelWorkflowExport->addMenu(
        mMenus.exportWorkflowSceneToImage, SARibbonPanelItem::Large, QToolButton::MenuButtonPopup);
}

/**
 * @brief 构建「工作流编辑」页
 * objectname=da-ribbon-category-workflow.edit
 *
 * 面板：剪贴板、图元（连线/轮廓/字体）、文本、背景、分组。
 * 图元编辑器与本类的 selectedWorkflowItem* 信号相连，由 Controller 转发给工作流视图。
 * @param contextWorkflow 工作流上下文（由 buildContextCategoryWorkflow 创建）
 * @param wfo 工作流操作窗口，提供粘贴/剪切/复制内建 action（由 buildContextCategoryWorkflow 保证非空）
 */
void DAAppRibbonArea::buildContextCategoryWorkflowEdit(SARibbonContextCategory* contextWorkflow,
                                                       DAPyWorkFlowOperateWidget* wfo)
{
    mCategoryWorkflowGraphicsEdit = contextWorkflow->addCategoryPage(tr("Workflow Edit"));  // cn:工作流编辑
    mCategoryWorkflowGraphicsEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowEditCategory));

    // 剪贴板面板：粘贴、剪切、复制
    mPannelClipBoard = mCategoryWorkflowGraphicsEdit->addPanel(tr("Clipboard"));  // cn:剪贴板
    mPannelClipBoard->addLargeAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionPaste));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCut));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCopy));
    // 图元面板：连线、轮廓与背景、字体
    mPannelWorkflowItem = mCategoryWorkflowGraphicsEdit->addPanel(tr("Item"));  // cn:图元
    mPannelWorkflowItem->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowItemPanel));
    mPannelWorkflowItem->addLargeAction(mActions->actionWorkflowLinkEnable);  // 连线
    mWorkflowShapeEditPannelWidget = new DAShapeEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowShapeEditPannelWidget, SARibbonPanelItem::Large);
    mPannelWorkflowItem->addSeparator();
    mWorkflowFontEditPannel = new DAFontEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowFontEditPannel, SARibbonPanelItem::Large);
    // 文本面板：绘制矩形、绘制文本
    mPannelWorkflowText = mCategoryWorkflowGraphicsEdit->addPanel(tr("Text"));  // cn:文本
    mPannelWorkflowText->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowTextPanel));
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawRect);
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawText);
    // 背景面板：添加背景图、锁定背景图、元件随背景移动
    mPannelWorkflowBackground = mCategoryWorkflowGraphicsEdit->addPanel(tr("Background"));  // cn:背景
    mPannelWorkflowBackground->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowBackgroundPanel));
    mPannelWorkflowBackground->addLargeAction(mActions->actionWorkflowAddBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowLockBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowEnableItemMoveWithBackground);

    // 分组面板：分组、取消分组、联动移动
    mPannelWorkflowGroup = mCategoryWorkflowGraphicsEdit->addPanel(tr("Group"));  // cn:分组
    mPannelWorkflowGroup->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowGroupPanel));
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemGrouping);
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemUngroup);
    mPannelWorkflowGroup->addLargeAction(mActions->actionWorkflowEnableItemLinkageMove);

    // 信号连接：图元编辑面板 -> 本类信号 -> DAAppController -> 工作流视图
    connect(mWorkflowShapeEditPannelWidget,
            &DAShapeEditPannelWidget::borderPenChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemPen);
    connect(mWorkflowShapeEditPannelWidget,
            &DAShapeEditPannelWidget::backgroundBrushChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemBrush);
    connect(mWorkflowFontEditPannel,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemFont);
    connect(mWorkflowFontEditPannel,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemFontColor);
}

/**
 * @brief 构建「工作流运行」页
 * objectname=da-ribbon-category-workflow.run
 *
 * 面板：运行、停止。
 * @param contextWorkflow 工作流上下文（由 buildContextCategoryWorkflow 创建）
 */
void DAAppRibbonArea::buildContextCategoryWorkflowRun(SARibbonContextCategory* contextWorkflow)
{
    mCategoryWorkflowRun = contextWorkflow->addCategoryPage(tr("Workflow Run"));  // cn:工作流运行
    mCategoryWorkflowRun->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowRunCategory));
    // 运行面板：运行工作流、停止工作流
    mPannelWorkflowRun = mCategoryWorkflowRun->addPanel(tr("Run"));  // cn:运行
    mPannelWorkflowRun->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowRunPanel));
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowRun);
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowTerminate);
}

/**
 * @brief 把当前工作流视图的标记线状态回灌到「视图标记」按钮
 * @param wfo 工作流操作窗口；内部会取当前编辑视图
 */
void DAAppRibbonArea::updateWorkflowAboutRibbon(DAPyWorkFlowOperateWidget* wfo)
{
    if (!wfo) {
        return;
    }
    DAPyWorkFlowEditWidget* wf     = wfo->getCurrentWorkFlowWidget();
    DAPyWorkFlowGraphicsView* view = wf ? wf->getWorkFlowGraphicsView() : nullptr;
    // 要判断DAPyWorkFlowGraphicsView的MarkerStyle
    if (view) {
        QAction* actMarker = mActions->actionWorkflowViewMarker;
        QAction* act       = wfo->getLineMarkerActionGroup()->checkedAction();
        if (!act || act == wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker)) {
            actMarker->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
            actMarker->setChecked(false);
        } else {
            actMarker->setIcon(act->icon());
            actMarker->setChecked(true);
        }
    }
}
