#ifndef DAACTIONSINTERFACE_H
#define DAACTIONSINTERFACE_H
#include <QAction>
#include "DAGlobals.h"
#include "DAUIInterface.h"
class QActionGroup;
namespace DA
{
class DACoreInterface;

/**
 * @brief 这是app所有action的管理器
 */
class DAINTERFACE_API DAActionsInterface : public DABaseInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAActionsInterface)
public:
    DAActionsInterface(DAUIInterface* u);
    ~DAActionsInterface();
    //创建一个action,并管理
    QAction* createAction(const char* objname);
    QAction* createAction(const char* objname, bool checkable, bool checked = false, QActionGroup* actGroup = nullptr);
    QAction* createAction(const char* objname, const char* iconpath);
    QAction* createAction(const char* objname, const char* iconpath, bool checkable, bool checked = false, QActionGroup* actGroup = nullptr);
    //记录action，action要保证有独立的object name
    void recordAction(QAction* act);
    //发生语言变更时会调用此函数
    virtual void retranslateUi();
    //查找action
    QAction* findAction(const char* objname);
    QAction* findAction(const QString& objname);
};
}  // end of DA
#endif  // DAACTIONSINTERFACE_H
