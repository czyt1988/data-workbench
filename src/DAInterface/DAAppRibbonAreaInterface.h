#ifndef DAAPPRIBBONAREAINTERFACE_H
#define DAAPPRIBBONAREAINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include "DAAppUIInterface.h"
#include "DAAppUIExtendInterface.h"
class SARibbonMainWindow;
class SARibbonBar;
class SARibbonCategory;
class SARibbonPannel;
namespace DA
{
class DACoreInterface;
DA_IMPL_FORWARD_DECL(DAAppRibbonAreaInterface)

/**
 * @brief 这个接口管理了AppRibbon区域的相关操作
 *
 * 通过这个接口能改变总体的显示
 * 在da中固定的标签有固定的objectname，详细在APP中的DAAppRibbonArea定义
 * 插件获取标签可以通过objectname判断是否是对应的内容，如主页标签的objectname=da-ribbon-category-main
 * 插件可以通过获取到的SARibbonCategory*指针，的objectName函数进行判断哪个是主页标签
 */
class DAINTERFACE_API DAAppRibbonAreaInterface : public DAAppUIExtendInterface
{
    Q_OBJECT
    DA_IMPL(DAAppRibbonAreaInterface)
public:
    DAAppRibbonAreaInterface(DAAppUIInterface* u);
    ~DAAppRibbonAreaInterface();

public:
    //针对ribbon的操作
    SARibbonBar* ribbonBar() const;

    //获取所有的标签
    QList< SARibbonCategory* > getCategorys() const;

    //通过obj-name获取Category(O(n))
    SARibbonCategory* getCategoryByObjectName(const QString& objname) const;

    //通过obj-name获取pannel(O(n))
    SARibbonPannel* getPannelByObjectName(const QString& objname) const;
};
}  // namespace DA
#endif  // DAAPPRIBBONAREAINTERFACE_H
