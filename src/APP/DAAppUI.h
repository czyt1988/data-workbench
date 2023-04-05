#ifndef DAAPPUI_H
#define DAAPPUI_H
#include "DAAppUIInterface.h"
namespace DA
{
class DAAppCore;
class DAAppCommand;
class DAAppActions;
class DAAppDockingArea;
class DAAppRibbonArea;
/**
 * @brief 总体界面接口，负责生成DAAppDockingArea和DAAppRibbonArea
 */
class DAAppUI : public DAAppUIInterface
{
    Q_OBJECT
public:
    DAAppUI(SARibbonMainWindow* m, DACoreInterface* c);

    //获取主程序
    virtual QMainWindow* getMainWindow() const override;

    //获取界面的docking区域
    virtual DAAppDockingAreaInterface* getDockingArea() override;

    //获取界面的ribbon区域
    virtual DAAppRibbonAreaInterface* getRibbonArea() override;

    //创建ui
    void createUi();

public:
    //获取core
    DAAppCore* appCore();
    //减少一次dynamic_cast
    DAAppActions* getActions();
    //减少一次dynamic_cast
    DAAppCommand* getAppCmd();
    //获取dock
    DAAppDockingArea* getAppDockingArea();

protected:
    void createActions();
    void createCmd();
    void createDockingArea();
    void createRibbonArea();

public:
    DAAppActions* m_actions;
    DAAppCommand* m_cmd;
    DAAppDockingArea* m_dockingArea;
    DAAppRibbonArea* m_ribbonArea;
};
}  // namespace DA

#ifndef DA_APP_UI
/**
 * @def 获取@sa DAAppCore 实例
 * @note 使用此宏需要以下头文件：
 * -# DAAppCore.h
 */
#define DA_APP_UI DA::DAAppCore::getInstance().getUi()
#endif

#endif  // DAAPPUI_H
