#include "DARibbonAreaInterface.h"
#include <QDebug>
#include "SARibbonMainWindow.h"
#include "SARibbonCategory.h"
#include "SARibbonBar.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
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
QList< SARibbonCategory* > DARibbonAreaInterface::getCategories() const
{
    return (d_ptr->mUiInterface->mainWindow()->ribbonBar()->categoryPages());
}

/**
 * @brief 通过obj-name获取Category
 * @note 运行复杂度为O(n)，转发SARibbonBar::categoryByObjectName
 * @param objname
 * @return 如果没找到，会返回nullptr
 */
SARibbonCategory* DARibbonAreaInterface::getCategoryByObjectName(const QString& objname) const
{
    SARibbonCategory* c = ribbonBar()->categoryByObjectName(objname);
    if (nullptr == c) {
        qWarning() << "DARibbonAreaInterface::getCategoryByObjectName: no category named" << objname;
    }
    return c;
}

/**
 * @brief 通过obj-name获取pannel(O(n))
 * @note 遍历所有category，每层转发SARibbonCategory::panelByObjectName
 * @param objname
 * @return 如果没找到，会返回nullptr
 */
SARibbonPanel* DARibbonAreaInterface::getPanelByObjectName(const QString& objname) const
{
    QList< SARibbonCategory* > categorys = getCategories();
    for (SARibbonCategory* category : std::as_const(categorys)) {
        if (category) {
            if (SARibbonPanel* pannel = category->panelByObjectName(objname)) {
                return pannel;
            }
        }
    }
    qWarning() << "DARibbonAreaInterface::getPanelByObjectName: no panel named" << objname;
    return nullptr;
}

/**
 * @brief 通过obj-name隐藏固定标签
 * @param objname
 * @return 找不到对应标签返回false
 */
bool DARibbonAreaInterface::hideCategory(const QString& objname)
{
    SARibbonCategory* c = ribbonBar()->categoryByObjectName(objname);
    if (nullptr == c) {
        qWarning() << "DARibbonAreaInterface::hideCategory: no category named" << objname;
        return false;
    }
    ribbonBar()->hideCategory(c);
    return true;
}

/**
 * @brief 通过obj-name显示被隐藏的标签
 * @param objname
 * @return 找不到对应标签返回false
 */
bool DARibbonAreaInterface::showCategory(const QString& objname)
{
    SARibbonCategory* c = ribbonBar()->categoryByObjectName(objname);
    if (nullptr == c) {
        qWarning() << "DARibbonAreaInterface::showCategory: no category named" << objname;
        return false;
    }
    ribbonBar()->showCategory(c);
    return true;
}

/**
 * @brief 通过obj-name隐藏上下文标签
 * @param objname 上下文标签的objectname，如da-ribbon-contextcategory-workflow
 * @return 找不到对应上下文标签返回false
 */
bool DARibbonAreaInterface::hideContextCategory(const QString& objname)
{
    QList< SARibbonContextCategory* > ctxs = ribbonBar()->contextCategoryList();
    for (SARibbonContextCategory* ctx : std::as_const(ctxs)) {
        if (ctx && ctx->objectName() == objname) {
            ribbonBar()->hideContextCategory(ctx);
            return true;
        }
    }
    qWarning() << "DARibbonAreaInterface::hideContextCategory: no context category named" << objname;
    return false;
}

/**
 * @brief 通过obj-name显示上下文标签
 * @param objname 上下文标签的objectname，如da-ribbon-contextcategory-workflow
 * @return 找不到对应上下文标签返回false
 */
bool DARibbonAreaInterface::showContextCategory(const QString& objname)
{
    QList< SARibbonContextCategory* > ctxs = ribbonBar()->contextCategoryList();
    for (SARibbonContextCategory* ctx : std::as_const(ctxs)) {
        if (ctx && ctx->objectName() == objname) {
            ribbonBar()->showContextCategory(ctx);
            return true;
        }
    }
    qWarning() << "DARibbonAreaInterface::showContextCategory: no context category named" << objname;
    return false;
}

/**
 * @brief 通过obj-name隐藏pannel
 * @param objname
 * @return 找不到对应pannel返回false
 */
bool DARibbonAreaInterface::hidePanel(const QString& objname)
{
    SARibbonPanel* p = getPanelByObjectName(objname);
    if (nullptr == p) {
        return false;
    }
    p->setVisible(false);
    return true;
}

/**
 * @brief 通过obj-name显示pannel
 * @param objname
 * @return 找不到对应pannel返回false
 */
bool DARibbonAreaInterface::showPanel(const QString& objname)
{
    SARibbonPanel* p = getPanelByObjectName(objname);
    if (nullptr == p) {
        return false;
    }
    p->setVisible(true);
    return true;
}

}  // namespace DA
