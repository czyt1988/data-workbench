#ifndef DAXMLHELPER_H
#define DAXMLHELPER_H
#include "DAGuiAPI.h"
#include <QDomDocument>
#include <QVariant>
#include <QVersionNumber>
#include <QHash>
#include "DAData.h"
#include "DADataManager.h"
#include "DAAbstractNode.h"
#include "DAGraphicsItemGroup.h"
/**
 *@file xml文件的帮助类
 */

namespace DA
{
class DAWorkFlowEditWidget;
class DAWorkFlow;
class DAWorkFlowGraphicsScene;
class DAWorkFlowGraphicsView;
class DAWorkFlowOperateWidget;
class DAGraphicsItem;
class DAGraphicsScene;
class DAGraphicsResizeableItem;
class DAColorTheme;
class DAFigureWidget;
class DAChartWidget;

/**
 * @brief DAProjectInterface::getProjectVersion的版本号会通过setVersionNumber设置进DAXmlHelper
 * DAXmlHelper会根据QVersionNumber来进行低版本的兼容
 *
 * 低版本兼容主要体现在load函数，save函数统一都只有一个版本
 *
 * v1.1.0 最初始版本
 * v1.3.0 优化了item的保存信息，优化了连接点的保存
 * v1.4.0 优化了属性的保存
 */
class DAGUI_API DAXmlHelper
{
	DA_DECLARE_PRIVATE(DAXmlHelper)
public:
	DAXmlHelper();
	~DAXmlHelper();

public:
	void setLoadedVersionNumber(const QVersionNumber& v);
	QVersionNumber getLoaderVersionNumber() const;
	static QVersionNumber getCurrentVersionNumber();
	// 标准保存—— DAWorkFlowEditWidget
	QDomElement makeElement(DAWorkFlowEditWidget* wfe, const QString& tagName, QDomDocument* doc);
	bool loadElement(DAWorkFlowEditWidget* wfe, const QDomElement* ele);
	// 标准保存—— DAWorkFlowEditWidget
	QDomElement makeElement(DAWorkFlowOperateWidget* wfo, const QString& tagName, QDomDocument* doc);
	bool loadElement(DAWorkFlowOperateWidget* wfo, const QDomElement* workflowsEle);
	// 创建剪切板描述xml
	QDomElement makeClipBoardElement(const QList< DAGraphicsItem* > its,
									 const QString& tagName,
									 QDomDocument* doc,
									 bool isCopyType = true);
	bool loadClipBoardElement(const QDomElement* clipBoardElement, DAWorkFlowGraphicsScene* sc);
	// DAGraphicsItem的通用保存
	static QDomElement makeElement(const DAGraphicsItem* item, const QString& tagName, QDomDocument* doc);
	static bool loadElement(DAGraphicsItem* item, const QDomElement* tag, const QVersionNumber& v = QVersionNumber());
	static QGraphicsItem* loadItemElement(const QDomElement* itemEle, const QVersionNumber& v = QVersionNumber());
	// DAGraphicsItemGroup的通用保存,注意！！！此函数并不会把子item的信息保存，仅仅记录子item的id
	static QDomElement makeElement(const DAGraphicsItemGroup* itemGroup, const QString& tagName, QDomDocument* doc);
	static bool loadElement(DAGraphicsScene* scene,
							DAGraphicsItemGroup* group,
							const QDomElement* groupElement,
							const QVersionNumber& v = QVersionNumber());
	// DA支持的所有QGraphicsItem的通用保存
	static QDomElement makeElement(const QGraphicsItem* item, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QGraphicsItem* item, const QDomElement* tag, const QVersionNumber& v = QVersionNumber());
	// 获取所有处理过的item
	QList< QGraphicsItem* > getAllDealItems() const;
	// DAColorTheme
	static QDomElement makeElement(const DAColorTheme* ct, const QString& tagName, QDomDocument* doc);
	static bool loadElement(DAColorTheme* ct, const QDomElement* tag, const QVersionNumber& v = QVersionNumber());
	// DAFigureWidget
	static QDomElement makeElement(const DAFigureWidget* fig, const QString& tagName, QDomDocument* doc);
	static bool loadElement(DAFigureWidget* fig, const QDomElement* tag, const QVersionNumber& v = QVersionNumber());
	// DAChartWidget
	static QDomElement makeElement(const DAChartWidget* chart, const QString& tagName, QDomDocument* doc);
	static bool loadElement(DAChartWidget* chart, const QDomElement* tag, const QVersionNumber& v = QVersionNumber());

public:
	// 生成一个qvariant element
	static QDomElement createVariantValueElement(QDomDocument& doc, const QString& tagName, const QVariant& var);
	static QVariant loadVariantValueElement(const QDomElement& item, const QVariant& defaultVal);
	// 带提示的属性转double
	static qreal attributeToDouble(const QDomElement& item, const QString& att);
};
}

#endif  // DAXMLHELPER_H
