﻿#ifndef DAAPPACTIONSINTERFACE_H
#define DAAPPACTIONSINTERFACE_H
#include <QAction>
#include "DAGlobals.h"
#include "DAAppUIInterface.h"
namespace DA
{
class DACoreInterface;
DA_IMPL_FORWARD_DECL(DAAppActionsInterface)

/**
 * @brief 这是app所有action的管理器
 */
class DAINTERFACE_API DAAppActionsInterface : public DABaseInterface
{
    Q_OBJECT
    DA_IMPL(DAAppActionsInterface)
public:
    DAAppActionsInterface(DAAppUIInterface* u);
    ~DAAppActionsInterface();
    //创建一个action,并管理
    QAction* createAction(const char* objname);
    QAction* createAction(const char* objname, bool checkable, bool checked = false);
    QAction* createAction(const char* objname, const char* iconpath);
    QAction* createAction(const char* objname, const char* iconpath, bool checkable, bool checked = false);
    //记录action，action要保证有独立的object name
    void recordAction(QAction* act);
    //发生语言变更时会调用此函数
    virtual void retranslateUi();
    //查找action
    QAction* findAction(const char* objname);
    QAction* findAction(const QString& objname);
};
}  // end of DA
#endif  // DAAPPACTIONSINTERFACE_H
