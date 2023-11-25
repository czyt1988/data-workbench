#include "DAActionsInterface.h"
#include "DACoreInterface.h"
#include <QHash>
#include <QDebug>
#include <QActionGroup>
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
QAction* DAActionsInterface::createAction(const char* objname, const char* iconpath, bool checkable, bool checked, QActionGroup* actGroup)
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
        qWarning() << tr("DAAppActionsInterface::recordAction get null action");
        return;
    }
#ifdef QT_DEBUG
    if (d_ptr->mObjectToAction.contains(act->objectName())) {
        qWarning() << tr("DAAppActionsInterface::recordAction(QAction objname=%1) receive same object name, and the "
                         "previous record will be overwritten")
                          .arg(act->objectName());
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
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAActionsInterface::findAction(const char* objname)
{
    return d_ptr->mObjectToAction.value(objname, nullptr);
}

/**
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAActionsInterface::findAction(const QString& objname)
{
    return d_ptr->mObjectToAction.value(objname, nullptr);
}
