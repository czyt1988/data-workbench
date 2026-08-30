#ifndef DARIBBONAREAINTERFACE_H
#define DARIBBONAREAINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include "DAUIInterface.h"
#include "DAUIExtendInterface.h"
class SARibbonMainWindow;
class SARibbonBar;
class SARibbonCategory;
class SARibbonPanel;
namespace DA
{
class DACoreInterface;

/**
 * @brief 这个接口管理了AppRibbon区域的相关操作
 *
 * 通过这个接口能改变总体的显示
 * 在da中固定的标签有固定的objectname，契约统一定义在 @ref DAUiObjectNames.h（单一事实来源）
 * 插件获取标签可以通过objectname判断是否是对应的内容，如主页标签的objectname=da-ribbon-category-main
 * 插件可以通过获取到的SARibbonCategory*指针，的objectName函数进行判断哪个是主页标签
 */
class DAINTERFACE_API DARibbonAreaInterface : public DAUIExtendInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DARibbonAreaInterface)
public:
    DARibbonAreaInterface(DAUIInterface* u);
    virtual ~DARibbonAreaInterface() override;

public:
    // 针对ribbon的操作
    SARibbonBar* ribbonBar() const;

    // 获取所有的标签
    QList< SARibbonCategory* > getCategories() const;

    // 通过obj-name获取Category(O(n))
    SARibbonCategory* getCategoryByObjectName(const QString& objname) const;

    // 通过obj-name获取pannel(O(n))
    SARibbonPanel* getPanelByObjectName(const QString& objname) const;

    // 通过obj-name隐藏固定标签，成功返回true，找不到对应标签返回false
    bool hideCategory(const QString& objname);

    // 通过obj-name显示被隐藏的标签，成功返回true，找不到对应标签返回false
    bool showCategory(const QString& objname);

    // 通过obj-name隐藏上下文标签，成功返回true，找不到对应标签返回false
    bool hideContextCategory(const QString& objname);

    // 通过obj-name显示上下文标签，成功返回true，找不到对应标签返回false
    bool showContextCategory(const QString& objname);

    // 通过obj-name隐藏pannel，成功返回true，找不到对应pannel返回false
    bool hidePanel(const QString& objname);

    // 通过obj-name显示pannel，成功返回true，找不到对应pannel返回false
    bool showPanel(const QString& objname);
};
}  // namespace DA
#endif  // DARIBBONAREAINTERFACE_H
