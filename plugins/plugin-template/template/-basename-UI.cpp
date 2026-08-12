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

/**
 * @brief 构造函数
 * @param par 父对象
 */
{{plugin-base-name}}UI::{{plugin-base-name}}UI(QObject* par) : QObject(par)
{
}

/**
 * @brief 析构函数
 */
{{plugin-base-name}}UI::~{{plugin-base-name}}UI()
{
}

/**
 * @brief 初始化UI
 * @param core 核心接口指针
 * @return 初始化成功返回true
 */
bool {{plugin-base-name}}UI::initialize(DA::DACoreInterface* core)
{
	mCore = core;
    mUi = core->getUiInterface();
    mActions = mUi->getActionInterface();
    return true;
}

/**
 * @brief 重新翻译UI字符串
 */
void {{plugin-base-name}}UI::retranslateUi()
{

}