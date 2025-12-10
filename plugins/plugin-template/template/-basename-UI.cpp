#include "{{plugin-base-name}}UI.h"
//Qt
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
{{plugin-base-name}}UI::{{plugin-base-name}}UI(QObject* par) : QObject(par)
{
}

{{plugin-base-name}}UI::~{{plugin-base-name}}UI()
{
}

bool {{plugin-base-name}}UI::initialize(DA::DACoreInterface* core)
{
	m_core = core;
    m_ui = core->getUiInterface();
    m_actions = m_ui->getActionInterface();
    return true;
}

void {{plugin-base-name}}UI::retranslateUi()
{

}