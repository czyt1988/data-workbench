#ifndef DAXMLHELPER_H
#define DAXMLHELPER_H
#include "DAGuiAPI.h"
#include <QDomDocument>
#include <QVariant>
#include <QVersionNumber>
#include "DAAbstractNode.h"
#include "DAGraphicsItemGroup.h"
/**
 *@file xml文件的帮助类
 */

namespace DA
{
DA_IMPL_FORWARD_DECL(DAXmlHelper)
class DAWorkFlowEditWidget;
class DAWorkFlow;
class DAWorkFlowGraphicsScene;
class DAWorkFlowOperateWidget;
class DAGraphicsResizeableItem;

/**
 * @brief DAProjectInterface::getProjectVersion的版本号会通过setVersionNumber设置进DAXmlHelper
 * DAXmlHelper会根据QVersionNumber来进行低版本的兼容
 *
 * 低版本兼容主要体现在load函数，save函数统一都只有一个版本
 */
class DAGUI_API DAXmlHelper
{
    DA_IMPL(DAXmlHelper)
public:
    DAXmlHelper();
    ~DAXmlHelper();

public:
    void setLoadedVersionNumber(const QVersionNumber& v);
    QVersionNumber getLoaderVersionNumber() const;
    QVersionNumber getCurrentVersionNumber() const;
    // 标准保存—— DAWorkFlowEditWidget
    QDomElement makeElement(DAWorkFlowEditWidget* wfe, const QString& tagName, QDomDocument* doc);
    bool loadElement(DAWorkFlowEditWidget* wfe, const QDomElement* ele);
    // 标准保存—— DAWorkFlowEditWidget
    QDomElement makeElement(DAWorkFlowOperateWidget* wfo, const QString& tagName, QDomDocument* doc);
    bool loadElement(DAWorkFlowOperateWidget* wfo, const QDomElement* workflowsEle);
    // ResizeableItem的通用保存
    QDomElement makeElement(DAGraphicsResizeableItem* item, const QString& tagName, QDomDocument* doc);
    bool loadElement(DAGraphicsResizeableItem* item, const QDomElement* tag);

public:
    // 生成一个qvariant element
    static QDomElement createVariantValueElement(QDomDocument& doc, const QVariant& var);
    static QVariant loadVariantValueElement(const QDomElement& item, const QVariant& defaultVal);
    // 带提示的属性转double
    static qreal attributeToDouble(const QDomElement& item, const QString& att);
};
}

#endif  // DAXMLHELPER_H
