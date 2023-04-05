#include "DAAppUIInterface.h"
#include "DAAppUIExtendInterface.h"
#include "DAAppActionsInterface.h"
#include <QEvent>
#include "SARibbonMainWindow.h"

namespace DA
{
class DAAppUIInterfacePrivate
{
    DA_IMPL_PUBLIC(DAAppUIInterface)
public:
    DAAppUIInterfacePrivate(DAAppUIInterface* p, SARibbonMainWindow* m);

public:
    SARibbonMainWindow* _mainWindow;
    DACommandInterface* _cmd;
    DAAppActionsInterface* _actionMgr;
    QList< DAAppUIExtendInterface* > _extends;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppUIInterfacePrivate
//===================================================

DAAppUIInterfacePrivate::DAAppUIInterfacePrivate(DAAppUIInterface* p, SARibbonMainWindow* m)
    : q_ptr(p), _mainWindow(m), _cmd(nullptr), _actionMgr(nullptr)
{
}
//===================================================
// DAAppUIInterface
//===================================================
DAAppUIInterface::DAAppUIInterface(SARibbonMainWindow* m, DACoreInterface* c)
    : DABaseInterface(c, m), d_ptr(new DAAppUIInterfacePrivate(this, m))
{
    if (m) {
        m->installEventFilter(this);
    }
}

DAAppUIInterface::~DAAppUIInterface()
{
    if (mainWindow()) {
        mainWindow()->removeEventFilter(this);
    }
}

/**
 * @brief 获取主窗口
 * @return 返回主窗口指针
 */
SARibbonMainWindow* DAAppUIInterface::mainWindow() const
{
    return (d_ptr->_mainWindow);
}

/**
 * @brief 发生语言变更时会调用此函数
 *
 * @sa DAAppUIInterface 的 @sa retranslateUi 不做任何动作，其继承的类可以通过重载此函数实现翻译
 */
void DAAppUIInterface::retranslateUi()
{
    int es = getExtendCount();
    for (int i = 0; i < es; ++i) {
        getExtend(i)->retranslateUi();
    }
    if (d_ptr->_actionMgr) {
        d_ptr->_actionMgr->retranslateUi();
    }
}

void DAAppUIInterface::registeAction(DAAppActionsInterface* ac)
{
    d_ptr->_actionMgr = ac;
}

/**
 * @brief 注册扩展
 *
 * 界面是由各个扩展组成，包括最原始的界面，是由DAAppDockingAreaInterface和DAAppRibbonAreaInterface，
 * 这两个扩展组成
 *
 * @param ex
 */
void DAAppUIInterface::registeExtend(DAAppUIExtendInterface* ex)
{
    d_ptr->_extends.append(ex);
}

/**
 * @brief 注册命令接口
 * @param cmd
 */
void DAAppUIInterface::registeCommand(DACommandInterface* cmd)
{
    d_ptr->_cmd = cmd;
}

/**
 * @brief 获取扩展的数量
 * @return
 */
int DAAppUIInterface::getExtendCount() const
{
    return d_ptr->_extends.size();
}

/**
 * @brief 获取扩展
 * @param index
 * @return 如果超出索引范围，返回nullptr
 */
DAAppUIExtendInterface* DAAppUIInterface::getExtend(int index)
{
    return d_ptr->_extends.value(index, nullptr);
}

/**
 * @brief 获取命令接口
 * @sa registeCommand
 * @return 如果没有注册命令接口，返回nullptr
 */
DACommandInterface* DAAppUIInterface::getCommandInterface() const
{
    return d_ptr->_cmd;
}

/**
 * @brief 获取action管理器
 * @return
 */
DAAppActionsInterface* DAAppUIInterface::getActionInterface() const
{
    return d_ptr->_actionMgr;
}

bool DAAppUIInterface::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mainWindow()) {
        //如果发现语言变更，调用retranslateUi()
        if (event && (event->type() == QEvent::LanguageChange)) {
            retranslateUi();
        }
    }
    return (QObject::eventFilter(watched, event));
}
