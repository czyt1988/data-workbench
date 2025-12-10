#include "DataAnalysisUI.h"
// Qt
#include <QMainWindow>
#include <QDebug>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
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
// DataAnalysisPlugin
#include "DataframeIOWorker.h"
#include "DataframeCleanerWorker.h"
#include "DataframeOperateWorker.h"
DataAnalysisUI::DataAnalysisUI(QObject* par) : QObject(par)
{
}

DataAnalysisUI::~DataAnalysisUI()
{
}

bool DataAnalysisUI::initialize(DA::DACoreInterface* core)
{
    m_core    = core;
    m_ui      = core->getUiInterface();
    m_actions = m_ui->getActionInterface();
    buildDataCategory();
    retranslateUi();
    return true;
}

void DataAnalysisUI::buildDataCategory()
{
    DA::DARibbonAreaInterface* ribbonArea = m_ui->getRibbonArea();
    SARibbonCategory* dataCategory = ribbonArea->getCategoryByObjectName(QStringLiteral("da-ribbon-category-data"));
    if (dataCategory) {
        panelDataOperate = ribbonArea->getPannelByObjectName(QStringLiteral("da-pannel-data.data-opt"));
        // 导出单个数据
        actionExportIndividualData = m_actions->createAction("actionExportIndividualData",
                                                             ":/DataAnalysisPluginIcon/icon/exportIndividualData.svg");
        actionExportMultipleData =
            m_actions->createAction("actionExportMultipleData", ":/DataAnalysisPluginIcon/icon/exportMultipleData.svg");
        actionExportToOneExcel =
            m_actions->createAction("actionExportToOneExcel", ":/DataAnalysisPluginIcon/icon/export-to-one-xlsx.svg");
        panelDataOperate->addLargeAction(actionExportIndividualData);
        panelDataOperate->addLargeAction(actionExportMultipleData);
        panelDataOperate->addLargeAction(actionExportToOneExcel);
    }

    // 获取Dataframe Context Category
    SARibbonCategory* dataframeContextCategory =
        ribbonArea->getCategoryByObjectName(QStringLiteral("da-ribbon-category-dataframe.operate"));
    if (dataframeContextCategory) {
        //----------------------------------------------------
        // 数据清洗 panel
        //----------------------------------------------------
        // 新建数据清洗panel
        panelDataCleaner = dataframeContextCategory->addPanel(tr("Data Cleaning"));  // cn：数据清洗
        panelDataCleaner->setObjectName(QStringLiteral("da-panel-dataframe.operate.datacleaner"));
        // 删除Nan值
        actionDataFrameDropNone =
            m_actions->createAction("actionDataFrameDropNone", ":/DataAnalysisPluginIcon/icon/dataframe-drop-none.svg");
        // 重复值处理
        actionDropDuplicates =
            m_actions->createAction("actionDropDuplicates", ":/DataAnalysisPluginIcon/icon/process-duplicate-data.svg");
        // 填充缺失值
        actionDataFrameFillNone =
            m_actions->createAction("actionDataFrameFillNone", ":/DataAnalysisPluginIcon/icon/dataframe-fill-none.svg");
        // 插值法填充缺失值
        actionDataFrameFillInterpolate = m_actions->createAction(
            "actionDataFrameInterpolate", ":/DataAnalysisPluginIcon/icon/dataframe-interpolate.svg");
        // 过滤异常值
        actionDataFrameRemoveOutlierIQR = m_actions->createAction(
            "actionDataFrameRemoveOutlierIQR", ":/DataAnalysisPluginIcon/icon/dataframe-clip-outlier.svg");
        // 基于Z-score替换异常值
        actionDataFrameRemoveOutliersZScore = m_actions->createAction(
            "actionDataFrameReplaceOutliersZScore", ":/DataAnalysisPluginIcon/icon/dataframe-clip-outlier.svg");
        // 转换偏态数值数据以改善分布
        actionDataFrameTransformSkewedData = m_actions->createAction(
            "actionDataFrameTransformSkewedData", ":/DataAnalysisPluginIcon/icon/dataframe-clip-outlier.svg");
        panelDataCleaner->addLargeAction(actionDataFrameDropNone);
        panelDataCleaner->addLargeAction(actionDropDuplicates);
        panelDataCleaner->addLargeAction(actionDataFrameFillNone);
        panelDataCleaner->addLargeAction(actionDataFrameFillInterpolate);
        panelDataCleaner->addLargeAction(actionDataFrameRemoveOutlierIQR);
        panelDataCleaner->addLargeAction(actionDataFrameRemoveOutliersZScore);
        panelDataCleaner->addLargeAction(actionDataFrameTransformSkewedData);
        //----------------------------------------------------
        // Data Filtering Panel 数据过滤panel
        //----------------------------------------------------
        pannelDataFiltering = dataframeContextCategory->addPanel(tr("Data Filtering"));  // cn :数据过滤
        pannelDataFiltering->setObjectName(QStringLiteral("DataAnalysisPlugin.pannelDataFiltering"));
        actionDataFrameEvalDatas     = m_actions->createAction("actionDataFrameEvalDatas",
                                                           ":/DataAnalysisPluginIcon/icon/dataframe-eval-datas.svg");
        actionDataFrameQueryDatas    = m_actions->createAction("actionDataFrameQueryDatas",
                                                            ":/DataAnalysisPluginIcon/icon/dataframe-query-datas.svg");
        actionDataFrameDataRetrieval = m_actions->createAction(
            "actionDataFrameDataRetrieval", ":/DataAnalysisPluginIcon/icon/dataframe-data-retrieval.svg");
        actionDataFrameDataFilterColumn = m_actions->createAction(
            "actionDataFrameDataSelect", ":/DataAnalysisPluginIcon/icon/dataframe-data-select.svg");
        actionDataFrameSort =
            m_actions->createAction("actionDataFrameSort", ":/DataAnalysisPluginIcon/icon/dataframe-sort.svg");
        pannelDataFiltering->addLargeAction(actionDataFrameEvalDatas);
        pannelDataFiltering->addLargeAction(actionDataFrameQueryDatas);
        pannelDataFiltering->addMediumAction(actionDataFrameDataRetrieval);
        pannelDataFiltering->addMediumAction(actionDataFrameDataFilterColumn);
        pannelDataFiltering->addLargeAction(actionDataFrameSort);
        //----------------------------------------------------
        // Statistic Panel 数据统计panel
        //----------------------------------------------------

        panelDataStatistic = dataframeContextCategory->addPanel(tr("Statistic"));  // cn：数据统计
        // 数据描述
        actionCreateDataDescribe =
            m_actions->createAction("actionCreateDataDescribe", ":/DataAnalysisPluginIcon/icon/dataframe-describe.svg");
        // 创建数据透视表
        actionCreatePivotTable = m_actions->createAction("actionDataFrameCreatePivotTable",
                                                         ":/DataAnalysisPluginIcon/icon/dataframe-pivot-table.svg");

        panelDataStatistic->addLargeAction(actionCreateDataDescribe);
        panelDataStatistic->addLargeAction(actionCreatePivotTable);
    }
}

void DataAnalysisUI::retranslateUi()
{
    if (panelDataOperate) {
        actionExportIndividualData->setText(tr("Export \nIndividual Data"));  // cn:导出\n单个数据
        actionExportMultipleData->setText(tr("Export \nMultiple Data"));      // cn:导出\n多个数据
        actionExportMultipleData->setToolTip(tr(
            "Export all data from the data management area to a folder, with each "
            "dataset saved as an individual data file."));  // cn:把数据管理区所有数据导出到一个文件夹中，每个数据形成一个数据文件
        actionExportToOneExcel->setText(tr("Export \nTo Excel"));  // cn:导出Excel
        actionExportToOneExcel->setToolTip(tr(
            "Export all data from the data management area to an Excel file, with each dataset as a separate sheet."));  // cn:把数据管理区所有数据导出到一个excel文件中，每个数据将作为excel的一个sheet
    }
    if (panelDataCleaner) {
        //----------------------------------------------------
        // 数据清洗 panel
        //----------------------------------------------------
        panelDataCleaner->setPanelName(tr("Data Cleaning"));                                // cn：数据清洗
        actionDataFrameDropNone->setText(tr("Drop None"));                                  // cn:删除\n缺失值
        actionDataFrameDropNone->setToolTip(tr("Drop rows which contain missing values"));  // cn:删除包含缺失值的行
        actionDropDuplicates->setText(tr("Drop Duplicates"));                               // cn:删除\n重复值
        actionDropDuplicates->setToolTip(tr("Drop duplicate datas"));  // cn:删除数据中的重复记录
        actionDataFrameFillNone->setText(tr("Fill None"));             // cn:填充\n缺失值
        actionDataFrameFillNone->setToolTip(tr("Fill rows which contain missing values"));  // cn:填充包含缺失值的行
        actionDataFrameFillInterpolate->setText(tr("Fill Interpolate"));                    // cn:插值填充
        actionDataFrameFillInterpolate->setToolTip(
            tr("Fill rows which contain missing values by interpolate"));  // cn:插值法填充包含缺失值的行
        actionDataFrameRemoveOutlierIQR->setText(tr("IQR Outlier Handling"));  // cn: IQR\n异常值处理
        // cn:IQR（四分位距）异常值处理是一种基于数据分布的非参数方法，
        // 核心逻辑是通过数据的四分位数范围识别偏离整体分布的极端值，不受异常值本身影响，稳定性强。
        actionDataFrameRemoveOutlierIQR->setToolTip(
            tr("The IQR (Interquartile Range) outlier handling method is a non-parametric approach based on data "
               "distribution. It identifies extreme values deviating from the overall distribution using the "
               "interquartile range, unaffected by outliers themselves and featuring strong stability."));
        actionDataFrameRemoveOutliersZScore->setText(tr("Z-Score Outlier Handling"));  // cn: Z-Score\n异常值处理
        // cn:Z-Score（标准化分数）异常值替换方法是一种基于正态分布假设的参数化方法，
        // 通过量化数据点偏离均值的标准差倍数识别异常值，并采用合理策略替换异常值以保留数据完整性。
        actionDataFrameRemoveOutliersZScore->setToolTip(
            tr("The Z-Score outlier replacement method is a parametric approach based on the normal distribution "
               "assumption. It identifies outliers by quantifying how many standard deviations a data point deviates "
               "from the mean, and replaces outliers with reasonable strategies to preserve data integrity"));

        actionDataFrameTransformSkewedData->setText(tr("Transform skewed"));  // cn:转换偏态数据
        actionDataFrameTransformSkewedData->setToolTip(
            tr("Transform skewed numerical data to improve distribution"));  // cn:转换偏态数值数据以改善分布
        //----------------------------------------------------
        // Data Filtering Panel 数据过滤panel
        //----------------------------------------------------
        pannelDataFiltering->setPanelName(tr("Data Filtering"));  // cn :数据过滤
        actionDataFrameEvalDatas->setText(tr("Eval Datas"));      // cn:数值计算
        actionDataFrameEvalDatas->setToolTip(
            tr("Evaluate a string describing operations on DataFrame columns"));  // cn:输入关于列操作的表达式字符串并执行计算
        actionDataFrameQueryDatas->setText(tr("Query Datas"));                                   // cn:条件筛选
        actionDataFrameQueryDatas->setToolTip(tr("Filter data outside of the given criteria"));  // cn:过滤给定条件外的数据
        actionDataFrameDataRetrieval->setText(tr("Data Retrieval"));                             // cn:数据检索
        actionDataFrameDataRetrieval->setToolTip(tr("Retrieve data for a certain condition"));  // cn:检索某条件的数据
        actionDataFrameDataFilterColumn->setText(tr("Filter by Column"));                       // cn:列数据过滤
        actionDataFrameSort->setText(tr("Sort"));                                               // cn:数据排序
        actionDataFrameSort->setToolTip(tr("Sort Data"));  // cn:对数据进行排序

        //----------------------------------------------------
        // Statistic Panel 数据统计panel
        //----------------------------------------------------
        panelDataStatistic->setPanelName(tr("Statistic"));             // cn:统计
        actionCreateDataDescribe->setText(tr("Data Describe"));        // cn:数据描述
        actionCreatePivotTable->setText(tr("Pivot Table"));            // cn: 数据\n透视表
        actionCreatePivotTable->setToolTip(tr("Create Pivot Table"));  // cn: 创建数据透视表
    }
}

void DataAnalysisUI::bind(DataframeIOWorker* worker)
{
    connect(actionExportIndividualData, &QAction::triggered, worker, &DataframeIOWorker::exportIndividualData);
    connect(actionExportMultipleData, &QAction::triggered, worker, &DataframeIOWorker::exportMultipleData);
    connect(actionExportToOneExcel, &QAction::triggered, worker, &DataframeIOWorker::exportToOneExcelFile);
}

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

void DataAnalysisUI::bind(DataframeOperateWorker* worker)
{
    connect(actionCreateDataDescribe, &QAction::triggered, worker, &DataframeOperateWorker::createDataframeDescribe);
    connect(actionDataFrameEvalDatas, &QAction::triggered, worker, &DataframeOperateWorker::evalDatas);
    connect(actionDataFrameQueryDatas, &QAction::triggered, worker, &DataframeOperateWorker::queryDatas);
    connect(actionDataFrameDataRetrieval, &QAction::triggered, worker, &DataframeOperateWorker::searchData);
    connect(actionDataFrameDataFilterColumn, &QAction::triggered, worker, &DataframeOperateWorker::filterByColumn);
    connect(actionDataFrameSort, &QAction::triggered, worker, &DataframeOperateWorker::sortDatas);
}
