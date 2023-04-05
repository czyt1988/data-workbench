#include "DAAppActionsInterface.h"
#include "DACoreInterface.h"
#include <QHash>
#include <QDebug>
namespace DA
{
class DAAppActionsInterfacePrivate
{
    DA_IMPL_PUBLIC(DAAppActionsInterface)
public:
    DAAppActionsInterfacePrivate(DAAppActionsInterface* p);

public:
    QHash< QString, QAction* > _objectToAction;
};

}

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppActionsInterfacePrivate
//===================================================

DAAppActionsInterfacePrivate::DAAppActionsInterfacePrivate(DAAppActionsInterface* p) : q_ptr(p)
{
}
//===================================================
// DAAppActionsInterface
//===================================================

DAAppActionsInterface::DAAppActionsInterface(DAAppUIInterface* u)
    : DABaseInterface(u->core(), u), d_ptr(new DAAppActionsInterfacePrivate(this))
{
}

DAAppActionsInterface::~DAAppActionsInterface()
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
QAction* DAAppActionsInterface::createAction(const char* objname)
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
QAction* DAAppActionsInterface::createAction(const char* objname, bool checkable, bool checked)
{
    QAction* act = createAction(objname);
    act->setCheckable(checkable);
    act->setChecked(checked);
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
QAction* DAAppActionsInterface::createAction(const char* objname, const char* iconpath)
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
QAction* DAAppActionsInterface::createAction(const char* objname, const char* iconpath, bool checkable, bool checked)
{
    QAction* act = createAction(objname, iconpath);
    act->setCheckable(checkable);
    act->setChecked(checked);
    return act;
}
/**
 * @brief 记录action，action要保证有独立的object name
 * @param act
 */
void DAAppActionsInterface::recordAction(QAction* act)
{
    if (nullptr == act) {
        qWarning() << tr("DAAppActionsInterface::recordAction get null action");
        return;
    }
#ifdef QT_DEBUG
    if (d_ptr->_objectToAction.contains(act->objectName())) {
        qWarning() << tr("DAAppActionsInterface::recordAction(QAction objname=%1) receive same object name, and the "
                         "previous record will be overwritten")
                              .arg(act->objectName());
    }
#endif
    d_ptr->_objectToAction[ act->objectName() ] = act;
}

/**
 * @brief 实例化的接口需要继承此函数实现语言切换的文本变更
 */
void DAAppActionsInterface::retranslateUi()
{
}

/**
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAAppActionsInterface::findAction(const char* objname)
{
    return d_ptr->_objectToAction.value(objname, nullptr);
}

/**
 * @brief 查找action
 * @param objname
 * @return 如果没有返回nullptr
 */
QAction* DAAppActionsInterface::findAction(const QString& objname)
{
    return d_ptr->_objectToAction.value(objname, nullptr);
}
