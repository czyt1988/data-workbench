#ifndef DAUIINTERFACE_H
#define DAUIINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include <QObject>
#include "DABaseInterface.h"
class SARibbonMainWindow;
class QMainWindow;
namespace DA
{
class DACoreInterface;
class DAUIExtendInterface;
class DAActionsInterface;
class DACommandInterface;
class DADockingAreaInterface;
class DARibbonAreaInterface;
class DAStatusBarInterface;
/**
 * @brief 界面相关的接口
 *
 * 界面相关的接口都继承此接口
 */
class DAINTERFACE_API DAUIInterface : public DABaseInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAUIInterface)
public:
    DAUIInterface(SARibbonMainWindow* m, DACoreInterface* c);
    ~DAUIInterface();

    // 获取主窗口
    SARibbonMainWindow* mainWindow() const;

    // 发生语言变更时会调用此函数,此函数将调用所有扩展的retranslateUi函数
    virtual void retranslateUi();

    // 注册action管理器
    void registeAction(DAActionsInterface* ac);

    // 注册扩展
    void registeExtend(DAUIExtendInterface* ex);

    // 注册命令接口
    void registeCommand(DACommandInterface* cmd);

    // 获取扩展数量
    int getExtendCount() const;

    // 获取扩展
    DAUIExtendInterface* getExtend(int index);

    // 获取命令接口，如果没有注册命令接口，返回nullptr，当前设计为一个命令接口
    DACommandInterface* getCommandInterface() const;

    // 获取action管理器
    DAActionsInterface* getActionInterface() const;

    // 添加信息在程序的日志窗口里显示
    virtual void addInfoLogMessage(const QString& msg) = 0;

    // 添加信息在程序的日志窗口里显示
    virtual void addWarningLogMessage(const QString& msg) = 0;

    // 添加信息在程序的日志窗口里显示
    virtual void addCriticalLogMessage(const QString& msg) = 0;

    // QApplication::processEvents();的wrapper
    void processEvents() const;

public:
    // 下面是默认的extend
    // 获取主程序,此函数和getRibbonArea()->app()是一样的返回结果
    virtual QMainWindow* getMainWindow() const = 0;

    // 获取界面的docking区域
    virtual DADockingAreaInterface* getDockingArea() = 0;

    // 获取界面的ribbon区域
    virtual DARibbonAreaInterface* getRibbonArea() = 0;

    // 获取界面的StatusBar区域
    virtual DAStatusBarInterface* getStatusBar() = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};
}  // namespace DA
#endif  // DAUIINTERFACE_H
