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
        // 新建数据清洗panel
        panelDataCleaner = dataframeContextCategory->addPanel(tr("Data Cleaning"));  // cn：数据清洗
        panelDataCleaner->setObjectName(QStringLiteral("da-panel-dataframe.operate.datacleaner"));
        actionDataFrameDropNone =
            m_actions->createAction("actionDataFrameDropNone", ":/DataAnalysisPluginIcon/icon/dataframe-drop-none.svg");

        panelDataCleaner->addLargeAction(actionDataFrameDropNone);
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
        panelDataCleaner->setPanelName(tr("Data Cleaning"));                                // cn：数据清洗
        actionDataFrameDropNone->setText(tr("Drop None"));                                  // cn:删除\n缺失值
        actionDataFrameDropNone->setToolTip(tr("Drop rows which contain missing values"));  // cn:删除包含缺失值的行
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
}
