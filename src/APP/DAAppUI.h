#ifndef DAAPPUI_H
#define DAAPPUI_H
#include "DAUIInterface.h"
namespace DA
{
class DAAppCore;
class DAAppCommand;
class DAAppActions;
class DAAppDockingArea;
class DAAppRibbonArea;
class DAAppStatusBar;
/**
 * @brief 总体界面接口，负责生成DAAppDockingArea和DAAppRibbonArea
 */
class DAAppUI : public DAUIInterface
{
    Q_OBJECT
public:
    DAAppUI(SARibbonMainWindow* m, DACoreInterface* c);

    // 获取主程序
    virtual QMainWindow* getMainWindow() const override;

    // 获取界面的docking区域
    virtual DADockingAreaInterface* getDockingArea() override;

    // 获取界面的ribbon区域
    virtual DARibbonAreaInterface* getRibbonArea() override;

    // 获取界面的StatusBar区域
    virtual DAStatusBarInterface* getStatusBar() override;

    // 添加信息在程序的日志窗口里显示
    virtual void addInfoLogMessage(const QString& msg, bool showInStatusBar = true) override;

    // 添加信息在程序的日志窗口里显示
    virtual void addWarningLogMessage(const QString& msg, bool showInStatusBar = true) override;

    // 添加信息在程序的日志窗口里显示
    virtual void addCriticalLogMessage(const QString& msg, bool showInStatusBar = true) override;

    // 创建ui
    void createUi();

public:
    // 获取core
    DAAppCore* getAppCore();
    // 减少一次dynamic_cast
    DAAppActions* getAppActions();
    // 减少一次dynamic_cast
    DAAppCommand* getAppCmd();
    // 获取dock
    DAAppDockingArea* getAppDockingArea();
    // 获取ribbon
    DAAppRibbonArea* getAppRibbonArea();
    // 获取StatusBar
    DAAppStatusBar* getAppStatusBar();

protected:
    void createActions();
    void createCmd();
    void createDockingArea();
    void createRibbonArea();
    void createStatusBar();

public:
    DAAppActions* m_actions;
    DAAppCommand* m_cmd;
    DAAppDockingArea* m_dockingArea;
    DAAppRibbonArea* m_ribbonArea;
    DAAppStatusBar* m_statusBar;
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
