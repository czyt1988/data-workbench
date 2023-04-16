#include "DAUIInterface.h"
#include "DAUIExtendInterface.h"
#include "DAActionsInterface.h"
#include <QEvent>
#include "SARibbonMainWindow.h"

namespace DA
{
class DAUIInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DAUIInterface)
public:
    DAUIInterface::PrivateData(DAUIInterface* p);

public:
    SARibbonMainWindow* mMainWindow;
    DACommandInterface* mCmd { nullptr };
    DAActionsInterface* mActionMgr { nullptr };
    QList< DAUIExtendInterface* > mExtends;
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
    d_ptr->mMainWindow = m;
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
    return (d_ptr->mMainWindow);
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
    if (d_ptr->mActionMgr) {
        d_ptr->mActionMgr->retranslateUi();
    }
}

void DAUIInterface::registeAction(DAActionsInterface* ac)
{
    d_ptr->mActionMgr = ac;
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
    d_ptr->mExtends.append(ex);
}

/**
 * @brief 注册命令接口
 * @param cmd
 */
void DAUIInterface::registeCommand(DACommandInterface* cmd)
{
    d_ptr->mCmd = cmd;
}

/**
 * @brief 获取扩展的数量
 * @return
 */
int DAUIInterface::getExtendCount() const
{
    return d_ptr->mExtends.size();
}

/**
 * @brief 获取扩展
 * @param index
 * @return 如果超出索引范围，返回nullptr
 */
DAUIExtendInterface* DAUIInterface::getExtend(int index)
{
    return d_ptr->mExtends.value(index, nullptr);
}

/**
 * @brief 获取命令接口
 * @sa registeCommand
 * @return 如果没有注册命令接口，返回nullptr
 */
DACommandInterface* DAUIInterface::getCommandInterface() const
{
    return d_ptr->mCmd;
}

/**
 * @brief 获取action管理器
 * @return
 */
DAActionsInterface* DAUIInterface::getActionInterface() const
{
    return d_ptr->mActionMgr;
}

bool DAUIInterface::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mainWindow()) {
        //如果发现语言变更，调用retranslateUi()
        if (event && (event->type() == QEvent::LanguageChange)) {
            retranslateUi();
        }
    }
    return (QObject::eventFilter(watched, event));
}
}  // namespace DA
