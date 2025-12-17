#include "DAUIInterface.h"
#include "DAUIExtendInterface.h"
#include "DAActionsInterface.h"
#include <QEvent>
#include "SARibbonMainWindow.h"
#include <QApplication>
namespace DA
{
class DAUIInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DAUIInterface)
public:
    PrivateData(DAUIInterface* p);

public:
    SARibbonMainWindow* mainWindow;
    DACommandInterface* commandInterface { nullptr };
    DAActionsInterface* actionManager { nullptr };
    QList< DAUIExtendInterface* > extendsList;
    DAColorTheme colorTheme;
};

//===================================================
// DAAppUIInterfacePrivate
//===================================================

DAUIInterface::PrivateData::PrivateData(DAUIInterface* p) : q_ptr(p)
{
}
//===================================================
// DAAppUIInterface
//===================================================
DAUIInterface::DAUIInterface(SARibbonMainWindow* m, DACoreInterface* c) : DABaseInterface(c, m), DA_PIMPL_CONSTRUCT
{
    d_ptr->mainWindow = m;
    if (m) {
        m->installEventFilter(this);
    }
}

DAUIInterface::~DAUIInterface()
{
    if (mainWindow()) {
        mainWindow()->removeEventFilter(this);
    }
}

/**
 * @brief 获取主窗口
 * @return 返回主窗口指针
 */
SARibbonMainWindow* DAUIInterface::mainWindow() const
{
    return (d_ptr->mainWindow);
}

/**
 * @brief 发生语言变更时会调用此函数
 *
 * @sa DAAppUIInterface 的 @sa retranslateUi 不做任何动作，其继承的类可以通过重载此函数实现翻译
 */
void DAUIInterface::retranslateUi()
{
    int es = getExtendCount();
    for (int i = 0; i < es; ++i) {
        getExtend(i)->retranslateUi();
    }
    if (d_ptr->actionManager) {
        d_ptr->actionManager->retranslateUi();
    }
}

void DAUIInterface::registeAction(DAActionsInterface* ac)
{
    d_ptr->actionManager = ac;
}

/**
 * @brief 注册扩展
 *
 * 界面是由各个扩展组成，包括最原始的界面，是由DAAppDockingAreaInterface和DAAppRibbonAreaInterface，
 * 这两个扩展组成
 *
 * @param ex
 */
void DAUIInterface::registeExtend(DAUIExtendInterface* ex)
{
    d_ptr->extendsList.append(ex);
}

/**
 * @brief 注册命令接口
 * @param cmd
 */
void DAUIInterface::registeCommand(DACommandInterface* cmd)
{
    d_ptr->commandInterface = cmd;
}

/**
 * @brief 获取扩展的数量
 * @return
 */
int DAUIInterface::getExtendCount() const
{
    return d_ptr->extendsList.size();
}

/**
 * @brief 获取扩展
 * @param index
 * @return 如果超出索引范围，返回nullptr
 */
DAUIExtendInterface* DAUIInterface::getExtend(int index)
{
    return d_ptr->extendsList.value(index, nullptr);
}

/**
 * @brief 获取命令接口
 * @sa registeCommand
 * @return 如果没有注册命令接口，返回nullptr
 */
DACommandInterface* DAUIInterface::getCommandInterface() const
{
    return d_ptr->commandInterface;
}

/**
 * @brief 获取action管理器
 * @return
 */
DAActionsInterface* DAUIInterface::getActionInterface() const
{
    return d_ptr->actionManager;
}

/**
 * @brief 手动处理一下事件
 */
void DAUIInterface::processEvents() const
{
    QApplication::processEvents();
}

/**
 * @brief 设置程序主题
 * @param th
 */
void DAUIInterface::setColorTheme(const DAColorTheme& th)
{
    d_ptr->colorTheme = th;
}

/**
 * @brief 获取程序主题
 * @return
 */
DAColorTheme DAUIInterface::getColorTheme() const
{
    return d_ptr->colorTheme;
}

bool DAUIInterface::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mainWindow()) {
        // 如果发现语言变更，调用retranslateUi()
        if (event && (event->type() == QEvent::LanguageChange)) {
            retranslateUi();
        }
    }
    return (QObject::eventFilter(watched, event));
}
}  // namespace DA
