/**
 * @file DAAppRibbonAreaChart.cpp
 * @brief DAAppRibbonArea 的「绘图」上下文标签页实现
 *
 * 负责 Chart 上下文下的两张页面（objectName 见 DAUiObjectNames.h）：
 * - 绘图样式（da-ribbon-category-chart.style）：图表设置、绘图主题
 * - 图表编辑（da-ribbon-category-chart.edit）：绘图窗口设置、选区工具、辅助工具
 *
 * 另外包含图表状态回灌（网格/缩放/拾取/图例）的实现：这些函数只操作本页的 action，
 * 因此跟随本页放在同一个文件，便于「改界面 + 改同步」一次看全。
 *
 * 依赖注入：父容器与菜单集合全部入参，本文件不读 mContextChart 以外的上下文成员。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
// Qt
#include <QActionGroup>
#include <QDebug>
#include <QIcon>
#include <QSize>
#include <QToolButton>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonGallery.h"
#include "SARibbonGalleryGroup.h"
// Widget
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
// api
#include "DAAppActions.h"

using namespace DA;

/**
 * @brief 构建绘图上下文标签
 * objectname=da-ribbon-contextcategory-chart
 *
 * 两页结构：绘图样式、图表编辑（顺序即标签顺序）。
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildContextCategoryChart(SARibbonBar* ribbon)
{
    mContextChart = ribbon->addContextCategory(tr("Chart Operate"));  // cn:绘图操作
    mContextChart->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartContextCategory));
    buildContextCategoryChartStyle(mContextChart, mMenus);
    buildContextCategoryChartEdit(mContextChart);
}

/**
 * @brief 构建「绘图样式」页
 * objectname=da-ribbon-category-chart.style
 *
 * 面板：图表设置（网格/拖动/缩放/拾取/图例）、绘图主题（主题画廊 + 复制绘图到剪贴板）。
 * @param contextChart 绘图上下文（由 buildContextCategoryChart 创建）
 * @param menus 下拉菜单集合（图例位置、拾取设置，由 buildMenu 创建）
 * @note action->setMenu() 必须早于 addLargeAction()，否则按钮上的下拉箭头不会出现。
 */
void DAAppRibbonArea::buildContextCategoryChartStyle(SARibbonContextCategory* contextChart, const Menus& menus)
{
    mCategoryChartStyle = contextChart->addCategoryPage(tr("Chart Style"));  // cn:绘图样式
    mCategoryChartStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartStyleCategory));
    // 图表设置面板：应用到所有图表、网格显示（总开关、X/Y 方向、X/Y 最小刻度）、
    // 拖动、禁止水平缩放、禁止垂直缩放、缩放开关、放大、缩小、显示全部、
    // 十字拾取、XY 拾取、Y 拾取、联动拾取、显示图例（下拉选择位置）
    mPannelChartSetting = new SARibbonPanel(mCategoryChartStyle);
    mPannelChartSetting->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditChartSettingPanel));
    mPannelChartSetting->addLargeAction(mActions->actionFigureSettingApplyAllChart);
    mPannelChartSetting->addSeparator();
    // 网格
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableGrid);
    mChartGridDirActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridX);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridY);
    mPannelChartSetting->addSmallWidget(mChartGridDirActionsButtonGroup);
    mChartGridMinActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridXMin);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridYMin);
    mPannelChartSetting->addSmallWidget(mChartGridMinActionsButtonGroup);
    // 拖动
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePan);
    mPannelChartSetting->addMediumAction(mActions->actionChartDisableZoomX);
    mPannelChartSetting->addMediumAction(mActions->actionChartDisableZoomY);
    // 缩放
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableZoom);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomIn);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomOut);
    mPannelChartSetting->addLargeAction(mActions->actionChartZoomAll);
    // 数据拾取
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerCross);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerXY);
    mActions->actionChartEnablePickerY->setMenu(menus.chartPickSetting);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerY, QToolButton::MenuButtonPopup);
    mPannelChartSetting->addLargeAction(mActions->actionChartLinkAllPickerEnabled);
    // legend（下拉菜单为图例位置：上/下/左/右互斥）
    mActions->actionChartEnableLegend->setMenu(menus.chartLegendPosition);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableLegend, QToolButton::MenuButtonPopup);

    mCategoryChartStyle->addPanel(mPannelChartSetting);

    // 绘图主题面板：主题画廊、复制绘图到剪贴板
    mPanelFigureTheme   = new SARibbonPanel(mCategoryChartStyle);
    mFigureThemeGallery = mPanelFigureTheme->addGallery(true);
    mFigureThemeGallery->setMinimumWidth(200);
    SARibbonGalleryGroup* group1 =
        mFigureThemeGallery->addCategoryActions(tr("Theme"), mActions->actionListOfColorTheme);  // cn:主题
    group1->setGalleryGroupStyle(SARibbonGalleryGroup::IconWithText);
    group1->setGridMinimumWidth(80);
    group1->setIconSize(QSize(120, 50));
    group1->setGridSize(QSize(120, 60));
    mPanelFigureTheme->addAction(mActions->actionCopyFigureInClipboard);
    // 手动设置才能刷新当前界面的配置
    mFigureThemeGallery->setCurrentViewGroup(group1);
    mCategoryChartStyle->addPanel(mPanelFigureTheme);
}

/**
 * @brief 构建「图表编辑」页
 * objectname=da-ribbon-category-chart.edit
 *
 * 面板：绘图窗口设置（添加绘图/调整子图大小/新建坐标系）、选区工具、辅助工具。
 * @param contextChart 绘图上下文（由 buildContextCategoryChart 创建）
 */
void DAAppRibbonArea::buildContextCategoryChartEdit(SARibbonContextCategory* contextChart)
{
    mCategoryChartEdit = contextChart->addCategoryPage(tr("Chart Edit"));  // cn:图表编辑
    mCategoryChartEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditCategory));
    // 绘图窗口设置面板：添加绘图、调整子图大小、新建坐标系
    mPannelFigureSettingForContext = new SARibbonPanel(mCategoryChartEdit);
    mPannelFigureSettingForContext->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditFigureSettingPanel));
    mPannelFigureSettingForContext->addLargeAction(mActions->actionAddFigure);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionChartEditorResizeSubChart);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionFigureNewXYAxis);  // 新建坐标系
    mCategoryChartEdit->addPanel(mPannelFigureSettingForContext);
    // 选区工具面板：指针选择、矩形选框、椭圆选框、多边形选框
    mPannelChartSelectTool = mCategoryChartEdit->addPanel(tr("Select Tool"));  // cn:选区工具
    mPannelChartSelectTool->addLargeAction(mActions->actionChartEditorPointerSelector);
    mPannelChartSelectTool->addLargeAction(mActions->actionChartEditorRectSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorEllipseSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorPolygonSelector);
    // 辅助工具面板：十字标记、水平标记、垂直标记、箭头标记、文本标注、数据探针
    mPannelChartAssistTool = mCategoryChartEdit->addPanel(tr("Assist Tools"));  // cn:辅助工具
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddCrossMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddHLineMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddVLineMarker);
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddArrowMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddTextMarker);
    mPannelChartAssistTool->addSeparator();
    // 数据探针（多子图联动拾取+点击建探针+数据联动表）
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorDataProbe);
}

/**
 * @brief 更新绘图相关的ribbon
 * @param fig 当前绘图窗口；为空时直接返回
 */
void DAAppRibbonArea::updateFigureAboutRibbon(DAFigureWidget* fig)
{
    if (nullptr == fig) {
        qDebug() << "updateFigureAboutRibbon(fig:nullptr)";
        return;
    }
    DAChartWidget* chart = fig->getCurrentChart();
    updateChartAboutRibbon(chart);
    mActions->actionChartLinkAllPickerEnabled->setChecked(fig->isDataPickerGroupEnabled());
}

/**
 * @brief 更新图表相关的ribbon（网格、缩放拖动、拾取、图例）
 * @param chart 当前图表；为空时直接返回
 */
void DAAppRibbonArea::updateChartAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        qDebug() << "updateChartAboutRibbon(chart:nullptr)";
        return;
    }
    updateChartGridAboutRibbon(chart);
    updateChartZoomPanAboutRibbon(chart);
    updateChartPickerAboutRibbon(chart);
    updateChartLegendAboutRibbon(chart);
}

/**
 * @brief 更新Ribbon图表网格相关的界面
 * @param chart 当前图表；为空时直接返回
 */
void DAAppRibbonArea::updateChartGridAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    mActions->actionChartEnableGrid->setChecked(chart->isGridEnabled());
    mActions->actionChartEnableGridX->setChecked(chart->isGridXEnabled());
    mActions->actionChartEnableGridY->setChecked(chart->isGridYEnabled());
    mActions->actionChartEnableGridXMin->setChecked(chart->isGridXMinEnabled());
    mActions->actionChartEnableGridYMin->setChecked(chart->isGridYMinEnabled());
    bool c = mActions->actionChartEnableGrid->isChecked();
    mActions->actionChartEnableGridX->setEnabled(c);
    mActions->actionChartEnableGridY->setEnabled(c);
    mActions->actionChartEnableGridXMin->setEnabled(c);
    mActions->actionChartEnableGridYMin->setEnabled(c);
}

/**
 * @brief 更新Ribbon图表缩放相关的界面
 * @param chart 当前图表；为空时直接返回
 */
void DAAppRibbonArea::updateChartZoomPanAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    mActions->actionChartEnableZoom->setChecked(chart->isZoomEnabled());
    mActions->actionChartEnablePan->setChecked(chart->isPanEnabled());
    mActions->actionChartDisableZoomX->setChecked(!chart->isXAxisZoomEnabled());
    mActions->actionChartDisableZoomY->setChecked(!chart->isYAxisZoomEnabled());
}

/**
 * @brief 更新绘图的picker状态
 * @param chart 当前图表；为空时直接返回
 */
void DAAppRibbonArea::updateChartPickerAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    mActions->actionChartEnablePickerCross->setChecked(chart->isCrosshairEnabled());
    mActions->actionChartEnablePickerY->setChecked(chart->isYValuePickingEnabled());
    mActions->actionChartEnablePickerXY->setChecked(chart->isXYValuePickingEnabled());
}

/**
 * @brief 更新绘图的legend状态（含图例位置菜单勾选同步）
 * @param chart 当前图表；为空时直接返回
 */
void DAAppRibbonArea::updateChartLegendAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    mActions->actionChartEnableLegend->setChecked(chart->isLegendEnabled());
    // 图例位置菜单勾选同步
    Qt::Alignment align = chart->getLegendPosition();
    QAction* checked    = nullptr;
    if (align & Qt::AlignTop) {
        checked = mActions->actionChartLegendAtTop;
    } else if (align & Qt::AlignBottom) {
        checked = mActions->actionChartLegendAtBottom;
    } else if (align & Qt::AlignLeft) {
        checked = mActions->actionChartLegendAtLeft;
    } else if (align & Qt::AlignRight) {
        checked = mActions->actionChartLegendAtRight;
    }
    for (QAction* a : mActions->actionGroupChartLegendPosition->actions()) {
        a->setChecked(a == checked);
    }
}
