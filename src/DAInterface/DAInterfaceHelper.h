#ifndef DAINTERFACEHELPER_H
#define DAINTERFACEHELPER_H
#include "DAInterfaceAPI.h"
class QMainWindow;
namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DADockingAreaInterface;
class DARibbonAreaInterface;
class DADataManagerInterface;
class DACommandInterface;
class DAProjectInterface;
class DAStatusBarInterface;
/**
 * @brief interface的辅助类，可以继承它，也可以持有它
 *
 * 这里可以快速获取所有接口函数
 */
class DAINTERFACE_API DAInterfaceHelper
{
public:
    explicit DAInterfaceHelper();
    virtual ~DAInterfaceHelper();
    virtual void initialize(DACoreInterface* core);
    DACoreInterface* core() const;
    DAUIInterface* uiInterface() const;
    DADockingAreaInterface* dockAreaInterface() const;
    DARibbonAreaInterface* ribbonAreaInterface() const;
    DAStatusBarInterface* statusBarAreaInterface() const;
    DADataManagerInterface* dataManagerInterface() const;
    DACommandInterface* commandInterface() const;
    DAProjectInterface* projectInterface() const;
    QMainWindow* mainWindow();

private:
    DACoreInterface* m_core { nullptr };
    DAUIInterface* m_ui { nullptr };
    DADockingAreaInterface* m_dockArea { nullptr };
    DARibbonAreaInterface* m_ribbonArea { nullptr };
    DAStatusBarInterface* m_statusBarArea { nullptr };
    DADataManagerInterface* m_dataManager { nullptr };
    DACommandInterface* m_cmd { nullptr };
    DAProjectInterface* m_project { nullptr };
};
}
#endif  // DAINTERFACEHELPER_H
