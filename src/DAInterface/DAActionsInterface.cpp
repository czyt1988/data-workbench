#include "DAActionsInterface.h"
#include "DACoreInterface.h"
#include <QHash>
#include <QDebug>
#include <QActionGroup>
#include "DALogCategory.h"
namespace DA
{
class DAActionsInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DAActionsInterface)
public:
    PrivateData(DAActionsInterface* p);

public:
    QHash< QString, QAction* > mObjectToAction;
};

}

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppActionsInterfacePrivate
//===================================================

DAActionsInterface::PrivateData::PrivateData(DAActionsInterface* p) : q_ptr(p)
{
}
//===================================================
// DAAppActionsInterface
//===================================================

DAActionsInterface::DAActionsInterface(DAUIInterface* u) : DABaseInterface(u->core(), u), DA_PIMPL_CONSTRUCT
{
}

DAActionsInterface::~DAActionsInterface()
{
}
/**
 * @brief 创建一个action，并记录它
 *
 * action的Enabled状态会根据PythonInterpreterInitialized进行设置
 * @param objname
 * @param iconpath
 * @return
 */
QAction* DAActionsInterface::createAction(const char* objname)
{
    QAction* act = new QAction(this);
    act->setObjectName(QString::fromUtf8(objname));
    recordAction(act);
    return act;
}

/**
 * @brief DAAppActionsInterface::createAction
 * @param objname
 * @param checkable
 * @param checked
 * @return
 */
QAction* DAActionsInterface::createAction(const char* objname, bool checkable, bool checked, QActionGroup* actGroup)
{
    QAction* act = createAction(objname);
    act->setCheckable(checkable);
    act->setChecked(checked);
    if (actGroup) {
        act->setActionGroup(actGroup);
    }
    return act;
}

/**
 * @brief 创建一个action，并记录它
 *
 * action的Enabled状态会根据PythonInterpreterInitialized进行设置
 * @param objname
 * @param iconpath
 * @return
 */
QAction* DAActionsInterface::createAction(const char* objname, const char* iconpath)
{
    QAction* act = createAction(objname);
    act->setIcon(QIcon(iconpath));
    return act;
}
/**
 * @brief 创建一个action，并记录它
 * @param objname
 * @param iconpath
 * @param checkable
 * @param checked
 * @return
 */
QAction*
DAActionsInterface::createAction(const char* objname, const char* iconpath, bool checkable, bool checked, QActionGroup* actGroup)
{
    QAction* act = createAction(objname, iconpath);
    act->setCheckable(checkable);
    act->setChecked(checked);
    if (actGroup) {
        act->setActionGroup(actGroup);
    }
    return act;
}
/**
 * @brief 记录action，action要保证有独立的object name
 * @param act
 */
void DAActionsInterface::recordAction(QAction* act)
{
    if (nullptr == act) {
        daWarning << tr(
            "DAActionsInterface::recordAction received a null action");  // cn:DAActionsInterface::recordAction 收到空 action
        return;
    }
#ifdef QT_DEBUG
    if (d_ptr->mObjectToAction.contains(act->objectName())) {
        daWarning << tr("DAActionsInterface::recordAction(QAction objname=%1) received a duplicate object name, the "
                        "previous record will be overwritten")
                         .arg(act->objectName());  // cn:DAActionsInterface::recordAction(QAction objname=%1) 收到重复的对象名，之前的记录将被覆盖
    }
#endif
    d_ptr->mObjectToAction[ act->objectName() ] = act;
}

/**
 * @brief 实例化的接口需要继承此函数实现语言切换的文本变更
 */
void DAActionsInterface::retranslateUi()
{
}

/**
 * @brief 按object name移除并销毁action
 *
 * 供插件热卸载时清理经createAction创建的action。action的parent是本接口对象，
 * 插件库释放后不会随插件销毁，必须显式调用此函数移除，否则注册表中残留无效项。
 * 调用前应先将action从其所在的ribbon panel/菜单等控件中移除
 * @param objname action的object name
 * @return 是否找到并移除
 */
bool DAActionsInterface::removeAction(const QString& objname)
{
    auto it = d_ptr->mObjectToAction.find(objname);
    if (it == d_ptr->mObjectToAction.end()) {
        qWarning() << "DAActionsInterface::removeAction: no action named" << objname;
        return false;
    }
    QAction* act = it.value();
    d_ptr->mObjectToAction.erase(it);
    if (act) {
        act->deleteLater();
    }
    return true;
}

/**
 * @brief 移除并销毁action
 * @param act action指针
 * @return 是否找到并移除
 */
bool DAActionsInterface::removeAction(QAction* act)
{
    if (nullptr == act) {
        return false;
    }
    return removeAction(act->objectName());
}

/**
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAActionsInterface::findAction(const char* objname)
{
    QAction* act = d_ptr->mObjectToAction.value(objname, nullptr);
    if (nullptr == act) {
        qWarning() << "DAActionsInterface::findAction: no action named" << objname;
    }
    return act;
}

/**
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAActionsInterface::findAction(const QString& objname)
{
    QAction* act = d_ptr->mObjectToAction.value(objname, nullptr);
    if (nullptr == act) {
        qWarning() << "DAActionsInterface::findAction: no action named" << objname;
    }
    return act;
}
