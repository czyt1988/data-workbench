#include "DAAppRibbonAreaInterface.h"
#include "SARibbonMainWindow.h"
#include "SARibbonCategory.h"
#include "SARibbonBar.h"
#include "SARibbonMainWindow.h"
#include "SARibbonPannel.h"
namespace DA
{
class DAAppRibbonAreaInterfacePrivate
{
    DA_IMPL_PUBLIC(DAAppRibbonAreaInterface)
public:
    DAAppRibbonAreaInterfacePrivate(DAAppRibbonAreaInterface* p, DAAppUIInterface* u);

public:
    DAAppUIInterface* _ui;  //不调用父类的parent，这样是为了不进行qobject_cast，加快效率
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppRibbonAreaInterfacePrivate
//===================================================
DAAppRibbonAreaInterfacePrivate::DAAppRibbonAreaInterfacePrivate(DAAppRibbonAreaInterface* p, DAAppUIInterface* u)
    : q_ptr(p), _ui(u)
{
}

/**
 * @brief 构造函数，必须在主窗口之后构造
 * @note 此接口的生命周期跟随SARibbonMainWindow，DAAppRibbonAreaInterface将作为SARibbonMainWindow的子对象
 * @param mainwindow
 */
DAAppRibbonAreaInterface::DAAppRibbonAreaInterface(DAAppUIInterface* u)
    : DAAppUIExtendInterface(u), d_ptr(new DAAppRibbonAreaInterfacePrivate(this, u))
{
}

DAAppRibbonAreaInterface::~DAAppRibbonAreaInterface()
{
}

/**
 * @brief 获取ribbonbar
 * @return
 */
SARibbonBar* DAAppRibbonAreaInterface::ribbonBar() const
{
    return (d_ptr->_ui->mainWindow()->ribbonBar());
}

/**
 * @brief 获取所有标签
 * @return
 */
QList< SARibbonCategory* > DAAppRibbonAreaInterface::getCategorys() const
{
    return (d_ptr->_ui->mainWindow()->ribbonBar()->categoryPages());
}

/**
 * @brief 通过obj-name获取Category
 * @note 运行复杂度为O(n)
 * @param objname
 * @return 如果没找到，会返回nullptr
 */
SARibbonCategory* DAAppRibbonAreaInterface::getCategoryByObjectName(const QString& objname) const
{
    QList< SARibbonCategory* > categorys = getCategorys();
    for (SARibbonCategory* c : qAsConst(categorys)) {
        if (c) {
            if (c->objectName() == objname) {
                return c;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 通过obj-name获取pannel(O(n))
 * @note 运行复杂度为O(n)
 * @param objname
 * @return 如果没找到，会返回nullptr
 */
SARibbonPannel* DAAppRibbonAreaInterface::getPannelByObjectName(const QString& objname) const
{
    QList< SARibbonCategory* > categorys = getCategorys();
    for (SARibbonCategory* category : qAsConst(categorys)) {
        QList< SARibbonPannel* > pannels = category->pannelList();
        for (SARibbonPannel* pannel : qAsConst(pannels)) {
            if (pannel) {
                if (pannel->objectName() == objname) {
                    return pannel;
                }
            }
        }
    }
    return nullptr;
}
