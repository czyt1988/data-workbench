#include "DAGraphicsRectItem.h"
#include <QPainter>
#include <QDomDocument>
#include <QDomElement>
#include "DAQtEnumTypeStringUtils.h"
namespace DA
{

//===================================================
// DAGraphicsRectItem::PrivateData
//===================================================
class DAGraphicsRectItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsRectItem)
public:
	PrivateData(DAGraphicsRectItem* p);
	QPen getTextPen() const;
	void setTextPen(const QPen& p);

public:
	QString mText;
	Qt::Alignment mTextAlignment { Qt::AlignCenter };
	QPen mTextPen { Qt::black };
	QBrush mRectFillBrush { Qt::transparent };  ///< 矩形填充画笔
};

/**
 * @brief 构造函数
 * @param p 父对象指针
 */
DAGraphicsRectItem::PrivateData::PrivateData(DAGraphicsRectItem* p) : q_ptr(p)
{
}

/**
 * @brief 获取文本画笔
 * @return 文本画笔
 */
QPen DAGraphicsRectItem::PrivateData::getTextPen() const
{
	return mTextPen;
}

/**
 * @brief 设置文本画笔
 * @param p 文本画笔
 */
void DAGraphicsRectItem::PrivateData::setTextPen(const QPen& p)
{
	mTextPen = p;
}
//===================================================
// DAGraphicsRectItem
//===================================================
/**
 * @brief 构造函数
 * @param parent 父图形项
 */
DAGraphicsRectItem::DAGraphicsRectItem(QGraphicsItem* parent) : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
	enableShowBackground(false);
	setShowBorder(true);
	setBorderPen(QPen(QColor(Qt::black)));
}

/**
 * @brief 析构函数
 */
DAGraphicsRectItem::~DAGraphicsRectItem()
{
}

/**
 * @brief 设置文本
 * @param t 文本内容
 */
void DAGraphicsRectItem::setText(const QString& t)
{
	d_ptr->mText = t;
}

/**
 * @brief 获取文本
 * @return 文本内容
 */
QString DAGraphicsRectItem::getText() const
{
	return d_ptr->mText;
}

/**
 * @brief 设置文本对齐方式
 * @param al 对齐方式
 */
void DAGraphicsRectItem::setTextAlignment(Qt::Alignment al)
{
	d_ptr->mTextAlignment = al;
}

/**
 * @brief 获取文本对齐方式
 * @return 对齐方式
 */
Qt::Alignment DAGraphicsRectItem::getTextAlignment() const
{
	return d_ptr->mTextAlignment;
}

/**
 * @brief 获取文本画笔
 * @return 文本画笔
 */
QPen DAGraphicsRectItem::getTextPen() const
{
	return d_ptr->getTextPen();
}

/**
 * @brief 设置文本画笔
 * @param p 文本画笔
 */
void DAGraphicsRectItem::setTextPen(const QPen& p)
{
	d_ptr->setTextPen(p);
}

/**
 * @brief 获取矩形填充画笔
 * @return 矩形填充画笔
 */
QBrush DAGraphicsRectItem::getRectFillBrush() const
{
	return d_ptr->mRectFillBrush;
}

/**
 * @brief 设置矩形填充画笔
 * @param b 矩形填充画笔
 */
void DAGraphicsRectItem::setRectFillBrush(const QBrush& b)
{
	d_ptr->mRectFillBrush = b;
	update();
}

/**
 * @brief 保存到XML
 * @param doc XML文档对象
 * @param parentElement 父XML元素
 * @param ver 版本号
 * @return 保存成功返回true
 */
bool DAGraphicsRectItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	DAGraphicsResizeableItem::saveToXml(doc, parentElement, ver);
	QDomElement rectEle = doc->createElement("rect-info");

	QDomElement textEle = doc->createElement("text");
	textEle.setAttribute("al", enumToString(d_ptr->mTextAlignment));
	textEle.appendChild(doc->createTextNode(d_ptr->mText));
	rectEle.appendChild(textEle);
	// text-pen
	rectEle.appendChild(DAXMLFileInterface::makeElement(d_ptr->mTextPen, "text-pen", doc));
	parentElement->appendChild(rectEle);
	return true;
}

/**
 * @brief 从XML加载
 * @param itemElement XML元素
 * @param ver 版本号
 * @return 加载成功返回true
 */
bool DAGraphicsRectItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	if (!DAGraphicsResizeableItem::loadFromXml(itemElement, ver)) {
		return false;
	}
	QDomElement rectEle = itemElement->firstChildElement("rect-info");
	if (rectEle.isNull()) {
		return false;
	}
	QDomElement textEle = rectEle.firstChildElement("text");
	if (!textEle.isNull()) {
		d_ptr->mTextAlignment = stringToEnum< Qt::AlignmentFlag >(textEle.attribute("al"), Qt::AlignCenter);
		d_ptr->mText          = textEle.text();
	}
	// 加载 text-pen
	QDomElement textPenEle = rectEle.firstChildElement("text-pen");
	if (!textPenEle.isNull()) {
		QPen p;
		if (DAXMLFileInterface::loadElement(p, &textPenEle)) {
			d_ptr->setTextPen(p);
		}
	}
	return true;
}

/**
 * @brief 绘制矩形主体
 * @param painter 画笔对象
 * @param option 样式选项
 * @param widget 所属widget
 * @param bodyRect 矩形主体区域
 */
void DAGraphicsRectItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->save();
	painter->setBrush(d_ptr->mRectFillBrush);
	painter->setPen(Qt::NoPen);
	painter->drawRect(bodyRect);
	if (!(d_ptr->mText.isEmpty())) {
		painter->setPen(d_ptr->mTextPen);
		painter->drawText(bodyRect, d_ptr->mTextAlignment, d_ptr->mText);
	}
	painter->restore();
}

}
