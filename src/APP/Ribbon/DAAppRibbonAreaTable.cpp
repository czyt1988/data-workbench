/**
 * @file DAAppRibbonAreaTable.cpp
 * @brief DAAppRibbonArea 的「表格」上下文标签页实现
 *
 * 负责 dataFrame 上下文下的三张页面（objectName 见 DAUiObjectNames.h）：
 * - 表格操作（da-ribbon-category-dataframe.operate）：坐标轴、列、类型、格式、排序、编辑、视图
 * - 数据操作（da-ribbon-category-dataframe.data-operate）：计算列、数据变换；插件在此页追加面板
 * - 表格样式（da-ribbon-category-dataframe.style）：底色、字体、清除、条件格式
 *
 * 依赖注入：父容器 SARibbonContextCategory* 与页面用到的菜单集合 Menus 全部入参，
 * 本文件不读 mContextDataFrame 以外的上下文成员。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
#include "AppMainWindow.h"
// Qt
#include <QSignalBlocker>
#include <QToolButton>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonLineWidgetContainer.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonColorToolButton.h"
// Widget
#include "DAPyDTypeComboBox.h"
#include "Table/Views/DATableDisplayFormatComboBox.h"
#include "DAFontEditPannelWidget.h"
// Py
#include "numpy/DAPyDType.h"
// api
#include "DAAppActions.h"

using namespace DA;

/**
 * @brief 构建表格上下文标签
 * objectname=da-ribbon-contextcategory-dataframe
 *
 * 建好上下文后把三张页面分派给下面的函数（顺序即标签顺序）。
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildContextCategoryTable(SARibbonBar* ribbon)
{
    mContextDataFrame = ribbon->addContextCategory(tr("DataFrame"));  // cn:DataFrame
    mContextDataFrame->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameContextCategory));
    buildContextCategoryTableOperate(mContextDataFrame, mMenus);
    buildContextCategoryTableDataOperate(mContextDataFrame);
    buildContextCategoryTableStyle(mContextDataFrame);
}

/**
 * @brief 构建「表格操作」页
 * objectname=da-ribbon-category-dataframe.operate
 *
 * 面板顺序：坐标轴、列、类型、格式、排序、编辑、视图。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 * @param menus        下拉菜单集合（由 buildMenu 创建，这里通过 setMenu 挂到按钮上）
 * @note action->setMenu() 必须早于 addLargeAction()，否则按钮上的下拉箭头不会出现。
 */
void DAAppRibbonArea::buildContextCategoryTableOperate(SARibbonContextCategory* contextTable, const Menus& menus)
{
    mCategoryDataframeTableOperate = contextTable->addCategoryPage(tr("Table Operations"));  // cn:表格操作
    mCategoryDataframeTableOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateCategory));
    // 坐标轴面板：插入行（下拉含插入行到上方）、插入列（下拉含向左插入列）、删除单元格、删除行、删除列、更改列名、列转索引
    mPannelDataframeOperateAxes = mCategoryDataframeTableOperate->addPanel(tr("Axes"));  // cn:坐标轴
    mPannelDataframeOperateAxes->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateAxesPanel));
    mActions->actionInsertRow->setMenu(menus.insertRow);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertRow, QToolButton::MenuButtonPopup);
    mActions->actionInsertColumnRight->setMenu(menus.insertColumn);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertColumnRight, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionRemoveCell);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveRow);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveColumn);
    mPannelDataframeOperateAxes->addSeparator();
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionRenameColumns);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionChangeToIndex);
    // 列面板：重命名列、复制列名、定位最大、定位最小、描述统计、快速绘图
    mPannelDataframeOperateColumn = mCategoryDataframeTableOperate->addPanel(tr("Column"));  // cn:列
    mPannelDataframeOperateColumn->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateColumnPanel));
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionRenameColumn);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionCopyColumnName);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMax);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMin);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionShowColumnDescribe);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionQuickPlot);
    // 类型面板：列类型下拉框、转换为数值、转换为字符串、转换为日期
    mPannelDataframeOperateDType = mCategoryDataframeTableOperate->addPanel(tr("Type"));  // cn:类型
    mPannelDataframeOperateDType->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateDTypePanel));
    mComboxColumnTypesContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateDType);
    mComboxColumnTypes          = new DAPyDTypeComboBox(mComboxColumnTypesContainer);
    mComboxColumnTypes->setMinimumWidth(
        Qt5Qt6Compat_fontMetrics_width(mApp->fontMetrics(), "timedelta64(scoll)"));  // 设置最小宽度
    mComboxColumnTypesContainer->setPrefix(tr("Type"));  // cn:类型
    mComboxColumnTypesContainer->setWidget(mComboxColumnTypes);
    mPannelDataframeOperateDType->addWidget(mComboxColumnTypesContainer, SARibbonPanelItem::Medium);
    mCastActionsButtonGroup = new SARibbonButtonGroupWidget();
    mCastActionsButtonGroup->addAction(mActions->actionCastToNum);
    mCastActionsButtonGroup->addAction(mActions->actionCastToString);
    mCastActionsButtonGroup->addSeparator();
    mCastActionsButtonGroup->addAction(mActions->actionCastToDatetime);
    mPannelDataframeOperateDType->addWidget(mCastActionsButtonGroup, SARibbonPanelItem::Medium);

    // 格式面板：显示格式下拉框、设置单元格格式
    mPannelDataframeOperateFormat = mCategoryDataframeTableOperate->addPanel(tr("Format"));  // cn:格式
    mPannelDataframeOperateFormat->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateFormatPanel));
    mComboxDisplayFormatContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateFormat);
    mComboxDisplayFormat          = new DATableDisplayFormatComboBox(mComboxDisplayFormatContainer);
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));  // cn:格式
    mComboxDisplayFormatContainer->setWidget(mComboxDisplayFormat);
    mPannelDataframeOperateFormat->addWidget(mComboxDisplayFormatContainer, SARibbonPanelItem::Medium);
    mPannelDataframeOperateFormat->addMediumAction(mActions->actionTableFormatCells);

    // 排序面板（Excel/WPS 式）：大按钮主体 = 自定义排序对话框，下拉含升序/降序/自定义/清除排序；
    // 右侧中按钮「清除排序」= 按索引升序恢复原始行顺序（排序后行头的 index 变乱序时的回退口）
    // setMenu 必须在 addLargeAction 之前（与插入行/插入列同款顺序）
    mActions->actionSort->setMenu(menus.sort);
    mPannelDataframeOperateSort = mCategoryDataframeTableOperate->addPanel(tr("Sort"));  // cn:排序
    mPannelDataframeOperateSort->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateSortPanel));
    mPannelDataframeOperateSort->addLargeAction(mActions->actionSort, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateSort->addMediumAction(mActions->actionSortClear);

    // 编辑面板：查找、替换、填充（下拉）、批注（下拉）
    mPannelDataframeOperateEdit = mCategoryDataframeTableOperate->addPanel(tr("Edit"));  // cn:编辑
    mPannelDataframeOperateEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateEditPanel));
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionFind);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionReplace);
    // 填充按钮点击即向下填充（快捷键 Ctrl+D）；下拉含向下、向上、向下填充空值、向上填充空值、序列填充
    mActions->actionFill->setMenu(menus.fill);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionFill, QToolButton::MenuButtonPopup);
    // 批注按钮点击即插入或编辑当前单元格批注；下拉含插入、编辑、删除、清除本表批注
    mActions->actionNote->setMenu(menus.note);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionNote, QToolButton::MenuButtonPopup);

    // 视图面板：冻结列（下拉）、隐藏列、显示所有列、列视图、定位
    // setMenu 必须在 addLargeAction 之前（与插入行/插入列同款顺序）
    mActions->actionFreezeColumns->setMenu(menus.freezeColumns);
    mPannelDataframeOperateView = mCategoryDataframeTableOperate->addPanel(tr("View"));  // cn:视图
    mPannelDataframeOperateView->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateViewPanel));
    mPannelDataframeOperateView->addLargeAction(mActions->actionFreezeColumns, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateView->addLargeAction(mActions->actionHideColumn);
    mPannelDataframeOperateView->addLargeAction(mActions->actionShowAllColumns);
    mPannelDataframeOperateView->addLargeAction(mActions->actionColumnViewDialog);
    // 定位：跳转到指定行、索引标签或列；快捷键在 DADataTableView 中处理，action 本身不设快捷键
    mPannelDataframeOperateView->addMediumAction(mActions->actionGotoCell);
}

/**
 * @brief 构建「数据操作」页
 * objectname=da-ribbon-category-dataframe.data-operate
 *
 * 核心数据加工入口：计算列、数据变换；插件（DataAnalysis）加载后按 objectName
 * 找到本 category 并追加数据清洗、数据过滤、数据统计等面板。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 */
void DAAppRibbonArea::buildContextCategoryTableDataOperate(SARibbonContextCategory* contextTable)
{
    mCategoryDataframeDataOperate = contextTable->addCategoryPage(tr("Data Operations"));  // cn:数据操作
    mCategoryDataframeDataOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateCategory));

    // 计算列面板：计算列、全部重算
    mPannelDataframeDataOperateComputed = mCategoryDataframeDataOperate->addPanel(tr("Computed Column"));  // cn:计算列
    mPannelDataframeDataOperateComputed->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateComputedPanel));
    mPannelDataframeDataOperateComputed->addLargeAction(mActions->actionComputedColumn);
    mPannelDataframeDataOperateComputed->addMediumAction(mActions->actionRecomputeAllComputedColumns);

    // 数据变换面板：分列、合并列、分组聚合、转置、宽转长、校验规则、执行校验
    mPannelDataframeDataOperateTransform = mCategoryDataframeDataOperate->addPanel(tr("Data Transform"));  // cn:数据变换
    mPannelDataframeDataOperateTransform->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateTransformPanel));
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionSplitColumn);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionMergeColumns);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionGroupBy);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionTranspose);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionMelt);
    // 校验规则管理，以及按启用规则校验全表（只读，不进撤销栈）
    mPannelDataframeDataOperateTransform->addSeparator();
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionValidationRules);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionRunValidation);
}

/**
 * @brief 构建「表格样式」页
 * objectname=da-ribbon-category-dataframe.style
 *
 * 面板顺序：底色、字体、清除、条件格式。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 */
void DAAppRibbonArea::buildContextCategoryTableStyle(SARibbonContextCategory* contextTable)
{
    mCategoryDataframeStyle = contextTable->addCategoryPage(tr("Table Style"));  // cn:表格样式
    mCategoryDataframeStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleCategory));

    // 底色面板：设置选中单元格或区域的填充颜色
    mPannelDataframeStyleFill = mCategoryDataframeStyle->addPanel(tr("Fill"));  // cn:底色
    mPannelDataframeStyleFill->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFillPanel));
    mBtnTableFillColor = new SARibbonColorToolButton(mPannelDataframeStyleFill);
    mBtnTableFillColor->setColorStyle(SARibbonColorToolButton::ColorFillToIcon);
    mBtnTableFillColor->setupStandardColorMenu();
    mPannelDataframeStyleFill->addWidget(mBtnTableFillColor, SARibbonPanelItem::Large);

    // 字体面板：设置字体、字号、颜色等
    mPannelDataframeStyleFont = mCategoryDataframeStyle->addPanel(tr("Font"));  // cn:字体
    mPannelDataframeStyleFont->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFontPanel));
    mWidgetTableFont = new DAFontEditPannelWidget(mPannelDataframeStyleFont);
    mPannelDataframeStyleFont->addWidget(mWidgetTableFont, SARibbonPanelItem::Large);

    // 清除面板：清除选中样式、清除所有样式
    mPannelDataframeStyleClear = mCategoryDataframeStyle->addPanel(tr("Clear"));  // cn:清除
    mPannelDataframeStyleClear->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleClearPanel));
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleSelected);
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleAll);

    // 条件格式面板：打开条件格式设置，与表体右键菜单中的「条件格式」共用同一个页面
    mPannelDataframeStyleConditional = mCategoryDataframeStyle->addPanel(tr("Conditional"));  // cn:条件格式
    mPannelDataframeStyleConditional->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleConditionalPanel));
    // 点击按钮直接弹出条件格式预设菜单
    mPannelDataframeStyleConditional->addLargeAction(mActions->actionConditionalFormat, QToolButton::InstantPopup);
}

/**
 * @brief 设置DataFrame的类型，[Context Category - dataframe] [Type] -> Type
 * @param d 目标类型；本函数只回灌界面，忽略下拉框自身发出的信号
 */
void DAAppRibbonArea::setDataframeOperateCurrentDType(const DAPyDType& d)
{
    // 先阻塞
    QSignalBlocker blocker(mComboxColumnTypes);
    Q_UNUSED(blocker);
    mComboxColumnTypes->setCurrentDType(d);
}
