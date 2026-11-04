/**
 * @file DAAppRibbonAreaTable.cpp
 * @brief DAAppRibbonArea 的「表格」上下文标签页实现
 *
 * 负责 dataFrame 上下文下的三张页面（objectName 见 DAUiObjectNames.h）：
 * - 表格操作（da-ribbon-category-dataframe.operate）：行与列、列、类型与格式、编辑、视图
 * - 数据操作（da-ribbon-category-dataframe.data-operate）：排序与筛选、计算列、数据变换、数据校验；
 *   插件在此页追加数据清洗与统计面板，并把数据过滤动作挂进「排序与筛选」面板
 * - 表格样式（da-ribbon-category-dataframe.style）：字体、填充、条件格式、清除
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
    buildContextCategoryTableDataOperate(mContextDataFrame, mMenus);
    buildContextCategoryTableStyle(mContextDataFrame);
}

/**
 * @brief 构建「表格操作」页
 * objectname=da-ribbon-category-dataframe.operate
 *
 * 面板顺序：行与列、列、类型与格式、编辑、视图。
 * 本页只收「表格本身的结构、显示与导航」；数据加工类动作（排序）放在「数据操作」页。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 * @param menus        下拉菜单集合（由 buildMenu 创建，这里通过 setMenu 挂到按钮上）
 * @note action->setMenu() 必须早于 addLargeAction()，否则按钮上的下拉箭头不会出现。
 */
void DAAppRibbonArea::buildContextCategoryTableOperate(SARibbonContextCategory* contextTable, const Menus& menus)
{
    mCategoryDataframeTableOperate = contextTable->addCategoryPage(tr("Table Operations"));  // cn:表格操作
    mCategoryDataframeTableOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateCategory));
    // 行与列面板：插入行（下拉含插入行到上方）、插入列（下拉含向左插入列）、删除行、删除列、
    // 列转索引；objectName 沿用 axes（插件按它找面板）。「列管理」统一收在「视图」面板一个入口
    mPannelDataframeOperateAxes = mCategoryDataframeTableOperate->addPanel(tr("Rows & Columns"));  // cn:行与列
    mPannelDataframeOperateAxes->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateAxesPanel));
    mActions->actionInsertRow->setMenu(menus.insertRow);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertRow, QToolButton::MenuButtonPopup);
    mActions->actionInsertColumnRight->setMenu(menus.insertColumn);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertColumnRight, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveRow);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveColumn);
    mPannelDataframeOperateAxes->addSeparator();
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
    // 类型与格式面板：列类型下拉框、转换为数值/字符串/日期；显示格式下拉框、设置单元格格式。
    // 两者都是「列的属性」，合并在一个面板里，避免两个各只有一两个控件的窄面板占位。
    mPannelDataframeOperateDType = mCategoryDataframeTableOperate->addPanel(tr("Type & Format"));  // cn:类型与格式
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
    // 显示格式：显示格式下拉框、设置单元格格式（原独立的 Format 面板已并入本面板）
    mComboxDisplayFormatContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateDType);
    mComboxDisplayFormat          = new DATableDisplayFormatComboBox(mComboxDisplayFormatContainer);
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));  // cn:格式
    mComboxDisplayFormatContainer->setWidget(mComboxDisplayFormat);
    mPannelDataframeOperateDType->addWidget(mComboxDisplayFormatContainer, SARibbonPanelItem::Medium);
    mPannelDataframeOperateDType->addMediumAction(mActions->actionTableFormatCells);

    // 排序已迁入「数据操作」页的「排序与筛选」面板（见 buildContextCategoryTableDataOperate），
    // 本页不再保留排序面板

    // 编辑面板：查找、替换、填充（下拉）、批注（下拉）、清空单元格
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
    // 清空单元格：把选中单元格置为空值（nan），不是结构删除，因此与填充/批注同属「编辑」
    mPannelDataframeOperateEdit->addMediumAction(mActions->actionRemoveCell);

    // 视图面板：冻结列（下拉）、隐藏列、显示所有列、列管理、定位
    // setMenu 必须在 addLargeAction 之前（与插入行/插入列同款顺序）
    mActions->actionFreezeColumns->setMenu(menus.freezeColumns);
    mPannelDataframeOperateView = mCategoryDataframeTableOperate->addPanel(tr("View"));  // cn:视图
    mPannelDataframeOperateView->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateViewPanel));
    mPannelDataframeOperateView->addLargeAction(mActions->actionFreezeColumns, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateView->addLargeAction(mActions->actionHideColumn);
    mPannelDataframeOperateView->addLargeAction(mActions->actionShowAllColumns);
    mPannelDataframeOperateView->addLargeAction(mActions->actionManageColumns);
    // 定位：跳转到指定行、索引标签或列；快捷键在 DADataTableView 中处理，action 本身不设快捷键
    mPannelDataframeOperateView->addMediumAction(mActions->actionGotoCell);
}

/**
 * @brief 构建「数据操作」页
 * objectname=da-ribbon-category-dataframe.data-operate
 *
 * 核心数据加工入口，面板顺序：排序与筛选、计算列、数据变换、数据校验。
 * 插件（DataAnalysis）加载后按 objectName 找到本 category，追加数据清洗与统计面板，
 * 并把数据过滤动作挂进核心创建的「排序与筛选」面板（排序与筛选属于同一类操作）。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 * @param menus        下拉菜单集合（排序下拉菜单由 buildMenu 创建）
 */
void DAAppRibbonArea::buildContextCategoryTableDataOperate(SARibbonContextCategory* contextTable, const Menus& menus)
{
    mCategoryDataframeDataOperate = contextTable->addCategoryPage(tr("Data Operations"));  // cn:数据操作
    mCategoryDataframeDataOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateCategory));

    // 排序与筛选面板（对应 Excel「数据→排序和筛选」）：大按钮主体 = 自定义排序对话框，
    // 下拉含升序/降序/自定义/清除排序；插件的数据过滤动作也挂进本面板。
    // setMenu 必须在 addLargeAction 之前，否则按钮上的下拉箭头不会出现
    mActions->actionSort->setMenu(menus.sort);
    mPannelDataframeDataOperateSortFilter = mCategoryDataframeDataOperate->addPanel(tr("Sort & Filter"));  // cn:排序与筛选
    mPannelDataframeDataOperateSortFilter->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateSortFilterPanel));
    mPannelDataframeDataOperateSortFilter->addLargeAction(mActions->actionSort, QToolButton::MenuButtonPopup);
    mPannelDataframeDataOperateSortFilter->addMediumAction(mActions->actionSortClear);

    // 计算列面板：计算列、全部重算
    mPannelDataframeDataOperateComputed = mCategoryDataframeDataOperate->addPanel(tr("Computed Column"));  // cn:计算列
    mPannelDataframeDataOperateComputed->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateComputedPanel));
    mPannelDataframeDataOperateComputed->addLargeAction(mActions->actionComputedColumn);
    mPannelDataframeDataOperateComputed->addMediumAction(mActions->actionRecomputeAllComputedColumns);

    // 数据变换面板：分列、合并列、分组聚合、转置、宽转长（按数据形状重组，不含校验）
    mPannelDataframeDataOperateTransform = mCategoryDataframeDataOperate->addPanel(tr("Data Transform"));  // cn:数据变换
    mPannelDataframeDataOperateTransform->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateTransformPanel));
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionSplitColumn);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionMergeColumns);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionGroupBy);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionTranspose);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionMelt);

    // 数据校验面板：规则管理，以及按启用规则校验全表（只读，不进撤销栈）。
    // 校验属于数据质量，不再挤在「数据变换」面板里靠分隔符区隔。
    mPannelDataframeDataOperateValidation = mCategoryDataframeDataOperate->addPanel(tr("Data Validation"));  // cn:数据校验
    mPannelDataframeDataOperateValidation->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateValidationPanel));
    mPannelDataframeDataOperateValidation->addLargeAction(mActions->actionValidationRules);
    mPannelDataframeDataOperateValidation->addMediumAction(mActions->actionRunValidation);
}

/**
 * @brief 构建「表格样式」页
 * objectname=da-ribbon-category-dataframe.style
 *
 * 面板顺序：字体、底色、条件格式、清除。
 * @param contextTable 表格上下文（由 buildContextCategoryTable 创建）
 */
void DAAppRibbonArea::buildContextCategoryTableStyle(SARibbonContextCategory* contextTable)
{
    mCategoryDataframeStyle = contextTable->addCategoryPage(tr("Table Style"));  // cn:表格样式
    mCategoryDataframeStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleCategory));

    // 面板顺序对齐 Excel「开始→字体」的习惯：字体在前、底色其次，条件格式在清除之前。

    // 字体面板：设置字体、字号、颜色等
    mPannelDataframeStyleFont = mCategoryDataframeStyle->addPanel(tr("Font"));  // cn:字体
    mPannelDataframeStyleFont->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFontPanel));
    mWidgetTableFont = new DAFontEditPannelWidget(mPannelDataframeStyleFont);
    mPannelDataframeStyleFont->addWidget(mWidgetTableFont, SARibbonPanelItem::Large);

    // 底色面板：设置选中单元格或区域的填充颜色
    mPannelDataframeStyleFill = mCategoryDataframeStyle->addPanel(tr("Fill"));  // cn:底色
    mPannelDataframeStyleFill->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFillPanel));
    mBtnTableFillColor = new SARibbonColorToolButton(mPannelDataframeStyleFill);
    mBtnTableFillColor->setColorStyle(SARibbonColorToolButton::ColorFillToIcon);
    mBtnTableFillColor->setupStandardColorMenu();
    mPannelDataframeStyleFill->addWidget(mBtnTableFillColor, SARibbonPanelItem::Large);

    // 条件格式面板：打开条件格式设置，与表体右键菜单中的「条件格式」共用同一个页面
    mPannelDataframeStyleConditional = mCategoryDataframeStyle->addPanel(tr("Conditional"));  // cn:条件格式
    mPannelDataframeStyleConditional->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleConditionalPanel));
    // 点击按钮直接弹出条件格式预设菜单
    mPannelDataframeStyleConditional->addLargeAction(mActions->actionConditionalFormat, QToolButton::InstantPopup);

    // 清除面板：清除选中样式、清除所有样式（放在样式页最后，避免误触）
    mPannelDataframeStyleClear = mCategoryDataframeStyle->addPanel(tr("Clear"));  // cn:清除
    mPannelDataframeStyleClear->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleClearPanel));
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleSelected);
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleAll);
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
