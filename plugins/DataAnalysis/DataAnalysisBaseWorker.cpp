#include "DataAnalysisBaseWorker.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "SARibbonMainWindow.h"
#include "DACommandInterface.h"
DataAnalysisBaseWorker::DataAnalysisBaseWorker(QObject* par) : QObject(par)
{
}

DataAnalysisBaseWorker::~DataAnalysisBaseWorker()
{
}

void DataAnalysisBaseWorker::initialize(DA::DACoreInterface* core)
{
    m_core        = core;
    m_ui          = core->getUiInterface();
    m_dataManager = core->getDataManagerInterface();
    m_project     = core->getProjectInterface();
    m_dockArea    = m_ui->getDockingArea();
    m_cmd         = m_ui->getCommandInterface();
}

DA::DACoreInterface* DataAnalysisBaseWorker::core() const
{
    return m_core;
}

DA::DAUIInterface* DataAnalysisBaseWorker::uiInterface() const
{
    return m_ui;
}

DA::DADockingAreaInterface* DataAnalysisBaseWorker::dockAreaInterface() const
{
    return m_dockArea;
}

DA::DADataManagerInterface* DataAnalysisBaseWorker::dataManagerInterface() const
{
    return m_dataManager;
}

DA::DACommandInterface* DataAnalysisBaseWorker::commandInterface() const
{
    return m_cmd;
}

DA::DAProjectInterface* DataAnalysisBaseWorker::projectInterface() const
{
    return m_project;
}

QMainWindow* DataAnalysisBaseWorker::mainWindow()
{
    return m_ui->mainWindow();
}
