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
 * @brief 清理initialize创建的全部UI资源
 *
 * 插件热卸载（finalize）前调用。清理要点：
 * 1. 把action从宿主ribbon panel中移除（移除对应按钮），插件自建的panel
 *    经SARibbonCategory::removePanel移除
 * 2. action的parent是宿主DAActionsInterface，不会随插件库卸载销毁，
 *    必须显式调用mActions->removeAction移除，否则注册表残留无效项
 * 3. 清理后将成员指针置空，防止悬空访问
 */
void {{plugin-base-name}}UI::finalize()
{
    //! 在此移除initialize中创建的panel/action，参考：
    //! removeActionFromPanel(panel, action);           // 从panel移除按钮
    //! category->removePanel(panel);                   // 移除插件自建panel
    //! mActions->removeAction(action);                 // 经宿主接口销毁action
}

/**
 * @brief 重新翻译UI字符串
 */
void {{plugin-base-name}}UI::retranslateUi()
{

}