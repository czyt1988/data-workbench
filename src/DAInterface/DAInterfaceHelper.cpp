#include "DAInterfaceHelper.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "DARibbonAreaInterface.h"
#include "DAStatusBarInterface.h"
#include "SARibbonMainWindow.h"
#include "DACommandInterface.h"
namespace DA
{
DAInterfaceHelper::DAInterfaceHelper()
{
}

DAInterfaceHelper::~DAInterfaceHelper()
{
}

void DAInterfaceHelper::initialize(DACoreInterface* core)
{
    m_core          = core;
    m_ui            = core->getUiInterface();
    m_dataManager   = core->getDataManagerInterface();
    m_project       = core->getProjectInterface();
    m_dockArea      = m_ui->getDockingArea();
    m_ribbonArea    = m_ui->getRibbonArea();
    m_statusBarArea = m_ui->getStatusBar();
    m_cmd           = m_ui->getCommandInterface();
}

DACoreInterface* DAInterfaceHelper::core() const
{
    return m_core;
}

DAUIInterface* DAInterfaceHelper::uiInterface() const
{
    return m_ui;
}

DADockingAreaInterface* DAInterfaceHelper::dockAreaInterface() const
{
    return m_dockArea;
}

DARibbonAreaInterface* DAInterfaceHelper::ribbonAreaInterface() const
{
    return m_ribbonArea;
}

DAStatusBarInterface* DAInterfaceHelper::statusBarAreaInterface() const
{
    return m_statusBarArea;
}

DADataManagerInterface* DAInterfaceHelper::dataManagerInterface() const
{
    return m_dataManager;
}

DACommandInterface* DAInterfaceHelper::commandInterface() const
{
    return m_cmd;
}

DAProjectInterface* DAInterfaceHelper::projectInterface() const
{
    return m_project;
}

QMainWindow* DAInterfaceHelper::mainWindow()
{
    return m_ui->mainWindow();
}
}
