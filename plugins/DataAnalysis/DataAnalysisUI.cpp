#include "DataAnalysisUI.h"
// Qt
#include <QMainWindow>
#include <QDebug>
#include <QLayoutItem>
#include <QToolButton>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonMenu.h"
#include "SARibbonPanel.h"
#include "SARibbonPanelLayout.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonMainWindow.h"
// ADS
#include "DockManager.h"
#include "DockWidget.h"
// DA
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DAUIExtendInterface.h"
#include "DADockingAreaInterface.h"
#include "DARibbonAreaInterface.h"
#include "DAActionsInterface.h"
#include "DAUiObjectNames.h"
// DataAnalysisPlugin
#include "DataframeIOWorker.h"
#include "DataframeCleanerWorker.h"
#include "DataframeOperateWorker.h"
/**
 * @brief 构造函数
 * @param par 父对象
 */
DataAnalysisUI::DataAnalysisUI(QObject* par) : QObject(par)
{
}

/**
 * @brief 析构函数
 */
DataAnalysisUI::~DataAnalysisUI()
{
}

/**
 * @brief 初始化UI
 * @param core 核心接口指针
 * @return 初始化是否成功
 */
bool DataAnalysisUI::initialize(DA::DACoreInterface* core)
{
    mCore    = core;
    mUi      = core->getUiInterface();
    mActions = mUi->getActionInterface();
    buildDataCategory();
    retranslateUi();
    return true;
}

/**
 * @brief 从panel中移除action对应的按钮
 * @param panel ribbon面板，可为nullptr
 * @param act action，可为nullptr
 */
static void removeActionFromPanel(SARibbonPanel* panel, QAction* act)
{
    if (nullptr == panel || nullptr == act) {
        return;
    }
    SARibbonPanelLayout* lay = panel->panelLayout();
    if (nullptr == lay) {
        return;
    }
    const int idx = lay->indexByAction(act);
    if (idx >= 0) {
        // takeAt内部已对按钮控件hide+deleteLater，item本身需手动释放
        QLayoutItem* item = lay->takeAt(idx);
        delete item;
    }
}

/**
 * @brief 清理initialize创建的全部ribbon资源
 *
 * 插件热卸载（finalize）前调用。清理顺序：
 * 1. 把action从宿主panel中移除（移除对应按钮）
 * 2. 把插件创建的panel从category中移除（SARibbon内部deleteLater）
 * 3. 经宿主DAActionsInterface销毁action——action的parent是宿主接口对象，
 *    不会随插件库卸载销毁，必须显式移除，否则注册表残留无效项
 */
void DataAnalysisUI::finalize()
{
    // 数据标签宿主panel中的导出action
    removeActionFromPanel(panelDataOperate, actionExportIndividualData);
    removeActionFromPanel(panelDataOperate, actionExportMultipleData);
    removeActionFromPanel(panelDataOperate, actionExportToOneExcel);
    // 数据操作 category 中插件自建的 panel，以及插件挂进核心「排序与筛选」面板的过滤动作
    SARibbonCategory* dataframeDataOperateCategory = nullptr;
    SARibbonPanel* panelSortFilter                 = nullptr;
    if (mUi && mUi->getRibbonArea()) {
        dataframeDataOperateCategory = mUi->getRibbonArea()->getCategoryByObjectName(
            QString::fromUtf8(DA::UiNames::Ribbon::DataFrameDataOperateCategory));
        panelSortFilter = mUi->getRibbonArea()->getPanelByObjectName(
            QString::fromUtf8(DA::UiNames::Ribbon::DataFrameDataOperateSortFilterPanel));
    }
    // 过滤动作挂在核心面板上，卸载时只摘按钮，不删宿主面板
    removeActionFromPanel(panelSortFilter, actionDataFrameQueryDatas);
    removeActionFromPanel(panelSortFilter, actionDataFrameDataFilterColumn);
    if (dataframeDataOperateCategory) {
        if (panelDataCleaner) {
            dataframeDataOperateCategory->removePanel(panelDataCleaner);
        }
        if (panelDataStatistic) {
            dataframeDataOperateCategory->removePanel(panelDataStatistic);
        }
    }
    // 数据清洗分组按钮从面板摘除；菜单不是 ribbon 的 action，不会被 action 销毁带走，
    // 且创建时无父对象（SARibbonMenu 的父对象必须是 QWidget），必须在此显式 delete
    removeActionFromPanel(panelDataCleaner, actionDataFrameCleanMissingGroup);
    removeActionFromPanel(panelDataCleaner, actionDataFrameCleanOutlierGroup);
    delete menuCleanMissing;
    delete menuCleanOutlier;
    // 经宿主接口销毁全部action
    if (mActions) {
        const QList< QAction* > allActions = {
            actionExportIndividualData,       actionExportMultipleData,
            actionExportToOneExcel,           actionDataFrameDropNone,
            actionDropDuplicates,             actionDataFrameFillNone,
            actionDataFrameFillInterpolate,   actionDataFrameRemoveOutlierIQR,
            actionDataFrameRemoveOutliersZScore, actionDataFrameTransformSkewedData,
            actionDataFrameQueryDatas,        actionDataFrameDataFilterColumn,
            actionCreateDataDescribe,         actionCreatePivotTable,
            actionDataFrameCleanMissingGroup, actionDataFrameCleanOutlierGroup
        };
        for (QAction* act : allActions) {
            if (act) {
                mActions->removeAction(act);
            }
        }
    }
    // 全部置空，防止悬空访问
    panelDataOperate                 = nullptr;
    actionExportIndividualData       = nullptr;
    actionExportMultipleData         = nullptr;
    actionExportToOneExcel           = nullptr;
    panelDataCleaner                 = nullptr;
    menuCleanMissing                 = nullptr;
    menuCleanOutlier                 = nullptr;
    actionDataFrameCleanMissingGroup = nullptr;
    actionDataFrameCleanOutlierGroup = nullptr;
    actionDataFrameDropNone          = nullptr;
    actionDropDuplicates             = nullptr;
    actionDataFrameFillNone          = nullptr;
    actionDataFrameFillInterpolate   = nullptr;
    actionDataFrameRemoveOutlierIQR  = nullptr;
    actionDataFrameRemoveOutliersZScore = nullptr;
    actionDataFrameTransformSkewedData  = nullptr;
    actionDataFrameQueryDatas        = nullptr;
    actionDataFrameDataFilterColumn  = nullptr;
    panelDataStatistic               = nullptr;
    actionCreateDataDescribe         = nullptr;
    actionCreatePivotTable           = nullptr;
}

/**
 * @brief 构建数据操作相关的Ribbon界面
 */
void DataAnalysisUI::buildDataCategory()
{
    DA::DARibbonAreaInterface* ribbonArea = mUi->getRibbonArea();
    SARibbonCategory* dataCategory =
        ribbonArea->getCategoryByObjectName(QString::fromUtf8(DA::UiNames::Ribbon::DataCategory));
    if (dataCategory) {
        panelDataOperate =
            ribbonArea->getPanelByObjectName(QString::fromUtf8(DA::UiNames::Ribbon::DataOperatePanel));
        // 导出单个数据
        actionExportIndividualData =
            mActions->createAction("actionExportIndividualData", ":/DataAnalysisPluginIcon/icon/exportIndividualData.svg");
        actionExportMultipleData =
            mActions->createAction("actionExportMultipleData", ":/DataAnalysisPluginIcon/icon/exportMultipleData.svg");
        actionExportToOneExcel =
            mActions->createAction("actionExportToOneExcel", ":/DataAnalysisPluginIcon/icon/export-to-one-xlsx.svg");
        panelDataOperate->addLargeAction(actionExportIndividualData);
        panelDataOperate->addLargeAction(actionExportMultipleData);
        panelDataOperate->addLargeAction(actionExportToOneExcel);
    }

    // 获取数据操作 category（数据清洗/过滤/统计挂到该页）
    SARibbonCategory* dataframeDataOperateCategory = ribbonArea->getCategoryByObjectName(
        QString::fromUtf8(DA::UiNames::Ribbon::DataFrameDataOperateCategory));
    if (dataframeDataOperateCategory) {
        //----------------------------------------------------
        // 数据清洗 panel（7 条动作压缩为 3 个大按钮：缺失值▾ / 删除重复值 / 异常值▾）
        // 缺失值/异常值为 InstantPopup 菜单按钮（与功能区「条件格式」同款交互），
        // 6 条动作收进下拉菜单；删除重复值是单一动作直接执行。
        // 分组按钮文案 3–5 字，不依赖功能区自动折行，三种功能区样式下均不截断
        //----------------------------------------------------
        panelDataCleaner = dataframeDataOperateCategory->addPanel(tr("Missing & Outliers"));  // cn:缺失与异常
        panelDataCleaner->setObjectName(QStringLiteral("da-pannel-dataframe.data-operate.clean"));
        // 下拉菜单：无父创建（SARibbonMenu 的父对象必须是 QWidget，本类是纯 QObject），
        // finalize 显式 delete；objectName 纳入插件契约，供外部（测试/脚本）按名定位
        menuCleanMissing = new SARibbonMenu();
        menuCleanMissing->setObjectName(QStringLiteral("DataAnalysisPlugin.menu.clean-missing"));
        menuCleanOutlier = new SARibbonMenu();
        menuCleanOutlier->setObjectName(QStringLiteral("DataAnalysisPlugin.menu.clean-outlier"));
        // 删除Nan值
        actionDataFrameDropNone =
            mActions->createAction("actionDataFrameDropNone", ":/DataAnalysisPluginIcon/icon/dataframe-drop-none.svg");
        // 重复值处理
        actionDropDuplicates =
            mActions->createAction("actionDropDuplicates", ":/DataAnalysisPluginIcon/icon/process-duplicate-data.svg");
        // 填充缺失值
        actionDataFrameFillNone =
            mActions->createAction("actionDataFrameFillNone", ":/DataAnalysisPluginIcon/icon/dataframe-fill-none.svg");
        // 插值法填充缺失值
        actionDataFrameFillInterpolate =
            mActions->createAction("actionDataFrameInterpolate", ":/DataAnalysisPluginIcon/icon/dataframe-interpolate.svg");
        // 过滤异常值
        actionDataFrameRemoveOutlierIQR =
            mActions->createAction("actionDataFrameRemoveOutlierIQR", ":/DataAnalysisPluginIcon/icon/dataframe-IQR.svg");
        // 基于Z-score替换异常值
        actionDataFrameRemoveOutliersZScore =
            mActions->createAction("actionDataFrameReplaceOutliersZScore", ":/DataAnalysisPluginIcon/icon/dataframe-Z-score.svg");
        // 转换偏态数值数据以改善分布
        actionDataFrameTransformSkewedData =
            mActions->createAction("actionDataFrameTransformSkewedData", ":/DataAnalysisPluginIcon/icon/dataframe-clip-outlier.svg");
        // 缺失值/异常值分组按钮（菜单条目复用上面的 action，triggered 连接不变）
        actionDataFrameCleanMissingGroup =
            mActions->createAction("actionDataFrameCleanMissingGroup", ":/DataAnalysisPluginIcon/icon/dataframe-drop-none.svg");
        actionDataFrameCleanOutlierGroup =
            mActions->createAction("actionDataFrameCleanOutlierGroup", ":/DataAnalysisPluginIcon/icon/dataframe-IQR.svg");
        menuCleanMissing->addAction(actionDataFrameDropNone);
        menuCleanMissing->addAction(actionDataFrameFillNone);
        menuCleanMissing->addAction(actionDataFrameFillInterpolate);
        menuCleanOutlier->addAction(actionDataFrameRemoveOutlierIQR);
        menuCleanOutlier->addAction(actionDataFrameRemoveOutliersZScore);
        menuCleanOutlier->addAction(actionDataFrameTransformSkewedData);
        // setMenu 必须早于 addLargeAction，否则按钮不出下拉箭头（同 DAAppRibbonAreaTable 已知坑）
        actionDataFrameCleanMissingGroup->setMenu(menuCleanMissing);
        actionDataFrameCleanOutlierGroup->setMenu(menuCleanOutlier);
        panelDataCleaner->addLargeAction(actionDataFrameCleanMissingGroup, QToolButton::InstantPopup);
        panelDataCleaner->addLargeAction(actionDropDuplicates);
        panelDataCleaner->addLargeAction(actionDataFrameCleanOutlierGroup, QToolButton::InstantPopup);
        //----------------------------------------------------
        // 数据过滤：挂进核心「排序与筛选」面板（筛选与排序同组，不再单占一个面板）
        //----------------------------------------------------
        SARibbonPanel* panelSortFilter = ribbonArea->getPanelByObjectName(
            QString::fromUtf8(DA::UiNames::Ribbon::DataFrameDataOperateSortFilterPanel));
        // 公式（Eval Data）action 已按 004 计划收口移除（内核计算列替代，D0/R2）
        actionDataFrameQueryDatas =
            mActions->createAction("actionDataFrameQueryDatas", ":/DataAnalysisPluginIcon/icon/dataframe-query-datas.svg");
        actionDataFrameDataFilterColumn =
            mActions->createAction("actionDataFrameDataSelect", ":/DataAnalysisPluginIcon/icon/dataframe-data-select.svg");
        if (panelSortFilter) {
            panelSortFilter->addLargeAction(actionDataFrameQueryDatas);
            panelSortFilter->addMediumAction(actionDataFrameDataFilterColumn);
        } else {
            qWarning() << "DataAnalysisUI: sort-filter panel not found, data filtering actions are not shown";
        }
        //----------------------------------------------------
        // Statistic Panel 数据统计panel
        //----------------------------------------------------

        panelDataStatistic = dataframeDataOperateCategory->addPanel(tr("Statistics"));  // cn:数据统计
        // 数据描述
        actionCreateDataDescribe =
            mActions->createAction("actionCreateDataDescribe", ":/DataAnalysisPluginIcon/icon/dataframe-describe.svg");
        // 创建数据透视表
        actionCreatePivotTable =
            mActions->createAction("actionDataFrameCreatePivotTable", ":/DataAnalysisPluginIcon/icon/dataframe-pivot-table.svg");

        panelDataStatistic->addLargeAction(actionCreateDataDescribe);
        panelDataStatistic->addLargeAction(actionCreatePivotTable);
    }
}

/**
 * @brief 翻译界面文本
 */
void DataAnalysisUI::retranslateUi()
{
    if (panelDataOperate) {
        actionExportIndividualData->setText(tr("Export Individual Data"));  // cn:导出\n单个数据
        actionExportMultipleData->setText(tr("Export Multiple Data"));      // cn:导出\n多个数据
        actionExportMultipleData->setToolTip(tr("Export all data from the data management area to a folder, with each "
                                                "dataset saved as an individual data file.")
        );  // cn:把数据管理区所有数据导出到一个文件夹中，每个数据形成一个数据文件
        actionExportToOneExcel->setText(tr("Export To Excel"));  // cn:导出Excel
        actionExportToOneExcel->setToolTip(
            tr("Export all data from the data management area to an Excel file, with each dataset as a separate sheet.")
        );  // cn:把数据管理区所有数据导出到一个excel文件中，每个数据将作为excel的一个sheet
    }
    if (panelDataCleaner) {
        //----------------------------------------------------
        // 数据清洗 panel（3 按钮：缺失值▾ / 删除重复值 / 异常值▾）
        //----------------------------------------------------
        panelDataCleaner->setPanelName(tr("Missing & Outliers"));  // cn:缺失与异常
        // 分组按钮（文案 3–5 字，菜单项单行短语；不再作为按钮正文，无手写换行）
        actionDataFrameCleanMissingGroup->setText(tr("Missing Values"));  // cn:缺失值
        actionDataFrameCleanMissingGroup->setToolTip(tr("Missing value handling: drop, fill or interpolate"));  // cn:缺失值处理：删除、填充或插值
        actionDataFrameCleanOutlierGroup->setText(tr("Outliers"));  // cn:异常值
        actionDataFrameCleanOutlierGroup->setToolTip(tr("Outlier handling: IQR, Z-Score or skewed data transform"));  // cn:异常值处理：IQR、Z-Score 或偏态转换
        actionDataFrameDropNone->setText(tr("Drop None"));                                  // cn:删除缺失值
        actionDataFrameDropNone->setToolTip(tr("Drop rows which contain missing values"));  // cn:删除包含缺失值的行
        actionDropDuplicates->setText(tr("Drop Duplicates"));                               // cn:删除重复值
        actionDropDuplicates->setToolTip(tr("Drop duplicate data"));  // cn:删除数据中的重复记录
        actionDataFrameFillNone->setText(tr("Fill None"));             // cn:填充缺失值
        actionDataFrameFillNone->setToolTip(tr("Fill rows which contain missing values"));  // cn:填充包含缺失值的行
        actionDataFrameFillInterpolate->setText(tr("Fill Interpolate"));                    // cn:插值填充
        actionDataFrameFillInterpolate->setToolTip(tr("Fill rows which contain missing values by interpolation")
        );  // cn:插值法填充包含缺失值的行
        actionDataFrameRemoveOutlierIQR->setText(tr("IQR Outlier Handling"));  // cn:IQR 异常值处理
        actionDataFrameRemoveOutlierIQR->setToolTip(
            tr("The IQR (Interquartile Range) outlier handling method is a non-parametric approach based on data "
               "distribution. It identifies extreme values deviating from the overall distribution using the "
               "interquartile range, unaffected by outliers themselves and featuring strong stability.")
        );  // cn:IQR（四分位距）异常值处理是一种基于数据分布的非参数方法，核心逻辑是通过数据的四分位数范围识别偏离整体分布的极端值，不受异常值本身影响，稳定性强。
        actionDataFrameRemoveOutliersZScore->setText(tr("Z-Score Outlier Handling"));  // cn:Z-Score 异常值处理
        actionDataFrameRemoveOutliersZScore->setToolTip(
            tr("The Z-Score outlier replacement method is a parametric approach based on the normal distribution "
               "assumption. It identifies outliers by quantifying how many standard deviations a data point deviates "
               "from the mean, and replaces outliers with reasonable strategies to preserve data integrity.")
        );  // cn:Z-Score（标准化分数）异常值替换方法是一种基于正态分布假设的参数化方法，通过量化数据点偏离均值的标准差倍数识别异常值，并采用合理策略替换异常值以保留数据完整性。

        actionDataFrameTransformSkewedData->setText(tr("Transform Skewed"));  // cn:转换偏态数据
        actionDataFrameTransformSkewedData->setToolTip(tr("Transform skewed numerical data to improve distribution")
        );  // cn:转换偏态数值数据以改善分布
        //----------------------------------------------------
        // 数据过滤 action（挂在核心「排序与筛选」面板，不设独立面板）
        //----------------------------------------------------
        actionDataFrameQueryDatas->setText(tr("Query Data"));                                   // cn:条件筛选
        actionDataFrameQueryDatas->setToolTip(tr("Filter data outside of the given criteria"));  // cn:过滤给定条件外的数据
        actionDataFrameDataFilterColumn->setText(tr("Filter by Column"));                       // cn:列数据过滤

        //----------------------------------------------------
        // Statistic Panel 数据统计panel
        //----------------------------------------------------
        panelDataStatistic->setPanelName(tr("Statistics"));            // cn:统计
        actionCreateDataDescribe->setText(tr("Data Description"));     // cn:数据描述
        actionCreatePivotTable->setText(tr("Pivot Table"));            // cn:数据\n透视表
        actionCreatePivotTable->setToolTip(tr("Create Pivot Table"));  // cn:创建数据透视表
    }
}

/**
 * @brief 绑定数据导入导出工作者
 * @param worker 工作者对象
 */
void DataAnalysisUI::bind(DataframeIOWorker* worker)
{
    connect(actionExportIndividualData, &QAction::triggered, worker, &DataframeIOWorker::exportIndividualData);
    connect(actionExportMultipleData, &QAction::triggered, worker, &DataframeIOWorker::exportMultipleData);
    connect(actionExportToOneExcel, &QAction::triggered, worker, &DataframeIOWorker::exportToOneExcelFile);
}

/**
 * @brief 绑定数据清洗工作者
 * @param worker 工作者对象
 */
void DataAnalysisUI::bind(DataframeCleanerWorker* worker)
{
    connect(actionDataFrameDropNone, &QAction::triggered, worker, &DataframeCleanerWorker::dropna);
    connect(actionDropDuplicates, &QAction::triggered, worker, &DataframeCleanerWorker::drop_duplicates);
    connect(actionDataFrameFillNone, &QAction::triggered, worker, &DataframeCleanerWorker::fillna);
    connect(actionDataFrameFillInterpolate, &QAction::triggered, worker, &DataframeCleanerWorker::fill_interpolate);
    connect(actionDataFrameRemoveOutlierIQR, &QAction::triggered, worker, &DataframeCleanerWorker::remove_outliers_iqr);
    connect(actionDataFrameRemoveOutliersZScore, &QAction::triggered, worker, &DataframeCleanerWorker::remove_outliers_zscore);
    connect(actionDataFrameTransformSkewedData, &QAction::triggered, worker, &DataframeCleanerWorker::transform_skewed_data);
}

/**
 * @brief 绑定数据操作工作者
 * @param worker 工作者对象
 */
void DataAnalysisUI::bind(DataframeOperateWorker* worker)
{
    connect(actionCreateDataDescribe, &QAction::triggered, worker, &DataframeOperateWorker::createDataframeDescribe);
    connect(actionDataFrameQueryDatas, &QAction::triggered, worker, QOverload<>::of(&DataframeOperateWorker::queryDatas));
    connect(actionDataFrameDataFilterColumn, &QAction::triggered, worker, QOverload<>::of(&DataframeOperateWorker::filterByColumn));
}
