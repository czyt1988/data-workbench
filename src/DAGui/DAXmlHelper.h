#ifndef DAXMLHELPER_H
#define DAXMLHELPER_H
#include "DAGuiAPI.h"
#include <QDomDocument>
#include <QVariant>
#include "DAAbstractNode.h"
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

class DAGUI_API DAXmlHelper
{
    DA_IMPL(DAXmlHelper)
public:
    DAXmlHelper();
    ~DAXmlHelper();

public:
    //标准保存—— DAWorkFlowEditWidget
    QDomElement makeElement(DAWorkFlowEditWidget* wfe, const QString& tagName, QDomDocument* doc);
    bool loadElement(DAWorkFlowEditWidget* wfe, const QDomElement* ele);
    //标准保存—— DAWorkFlowEditWidget
    QDomElement makeElement(DAWorkFlowOperateWidget* wfo, const QString& tagName, QDomDocument* doc);
    bool loadElement(DAWorkFlowOperateWidget* wfo, const QDomElement* workflowsEle);

public:
    //生成一个qvariant element
    static QDomElement createVariantValueElement(QDomDocument& doc, const QVariant& var);
    static QVariant loadVariantValueElement(const QDomElement& item, const QVariant& defaultVal);
    //带提示的属性转double
    static qreal attributeToDouble(const QDomElement& item, const QString& att);
};
}

#endif  // DAXMLHELPER_H
