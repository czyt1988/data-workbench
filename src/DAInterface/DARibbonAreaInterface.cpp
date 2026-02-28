#include "DARibbonAreaInterface.h"
#include "SARibbonMainWindow.h"
#include "SARibbonCategory.h"
#include "SARibbonBar.h"
#include "SARibbonMainWindow.h"
#include "SARibbonPanel.h"
namespace DA
{
class DARibbonAreaInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DARibbonAreaInterface)
public:
    PrivateData(DARibbonAreaInterface* p);

public:
    DAUIInterface* mUiInterface { nullptr };  // 不调用父类的parent，这样是为了不进行qobject_cast，加快效率
};

//===================================================
// DAAppRibbonAreaInterfacePrivate
//===================================================
DARibbonAreaInterface::PrivateData::PrivateData(DARibbonAreaInterface* p) : q_ptr(p)
{
}

/**
 * @brief 构造函数，必须在主窗口之后构造
 * @note 此接口的生命周期跟随SARibbonMainWindow，DAAppRibbonAreaInterface将作为SARibbonMainWindow的子对象
 * @param mainwindow
 */
DARibbonAreaInterface::DARibbonAreaInterface(DAUIInterface* u) : DAUIExtendInterface(u), DA_PIMPL_CONSTRUCT
{
    d_ptr->mUiInterface = u;
}

DARibbonAreaInterface::~DARibbonAreaInterface()
{
}

/**
 * @brief 获取ribbonbar
 * @return
 */
SARibbonBar* DARibbonAreaInterface::ribbonBar() const
{
    return (d_ptr->mUiInterface->mainWindow()->ribbonBar());
}

/**
 * @brief 获取所有标签
 * @return
 */
QList< SARibbonCategory* > DARibbonAreaInterface::getCategorys() const
{
    return (d_ptr->mUiInterface->mainWindow()->ribbonBar()->categoryPages());
}

/**
 * @brief 通过obj-name获取Category
 * @note 运行复杂度为O(n)
 * @param objname
 * @return 如果没找到，会返回nullptr
 */
SARibbonCategory* DARibbonAreaInterface::getCategoryByObjectName(const QString& objname) const
{
    QList< SARibbonCategory* > categorys = getCategorys();
    for (SARibbonCategory* c : std::as_const(categorys)) {
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
SARibbonPanel* DARibbonAreaInterface::getPannelByObjectName(const QString& objname) const
{
    QList< SARibbonCategory* > categorys = getCategorys();
    for (SARibbonCategory* category : std::as_const(categorys)) {
        QList< SARibbonPanel* > pannels = category->panelList();
        for (SARibbonPanel* pannel : std::as_const(pannels)) {
            if (pannel) {
                if (pannel->objectName() == objname) {
                    return pannel;
                }
            }
        }
    }
    return nullptr;
}

}  // namespace DA
