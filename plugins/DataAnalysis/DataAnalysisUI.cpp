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
    return true;
}

void DataAnalysisUI::buildDataCategory()
{
    DA::DARibbonAreaInterface* ribbonArea = m_ui->getRibbonArea();
    SARibbonCategory* dataCategory = ribbonArea->getCategoryByObjectName(QStringLiteral("da-ribbon-category-data"));
    if (!dataCategory) {
        return;
    }
    // 导出单个数据
    actionExportIndividualData =
        m_actions->createAction("actionExportIndividualData", ":/DataAnalysisPluginIcon/icon/exportIndividualData.svg");
}

void DataAnalysisUI::retranslateUi()
{
    actionExportIndividualData->setText(tr("Export \nIndividual Data"));  // cn:导出\n单个数据
}
