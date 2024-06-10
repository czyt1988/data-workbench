#include "DAGraphicsTextItem.h"
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QTextItem>
#include <QTextDocument>
#include <QGraphicsSceneResizeEvent>
#include <QTextCursor>
#include "DAGraphicsStandardTextItem.h"
// 设置相对位置的锚点，如果设置了锚点，RelativePosition将无效
namespace DA
{

class DAGraphicsTextItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsTextItem)
public:
	PrivateData(DAGraphicsTextItem* p);
	// 判断锚点是否固定了
	bool isAnchorFixBoth() const;
	// 只有，没有固定任何方向
	bool isAnchorFree() const;

public:
	DAGraphicsStandardTextItem* mTextItem { nullptr };
};

DAGraphicsTextItem::PrivateData::PrivateData(DAGraphicsTextItem* p) : q_ptr(p)
{
	mTextItem = new DAGraphicsStandardTextItem(p);
	mTextItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

//----------------------------------------------------
// DAGraphicsTextItem
//----------------------------------------------------

DAGraphicsTextItem::DAGraphicsTextItem(QGraphicsItem* parent) : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    init(tr("Text"));
}

DAGraphicsTextItem::DAGraphicsTextItem(const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
	init(tr("Text"));
	setSelectTextFont(f);
}

DAGraphicsTextItem::DAGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
	init(QString());
	setSelectTextFont(f);
	setPlainText(str);
}

DAGraphicsTextItem::~DAGraphicsTextItem()
{
}

void DAGraphicsTextItem::init(const QString& initText)
{
	setAcceptDrops(true);
	setAcceptHoverEvents(true);
	setFocusProxy(d_ptr->mTextItem);
	setEnableResize(true);
	setShowBorder(false);
	setEditable(true);
	setSelectable(true);
	setMovable(true);
	d_ptr->mTextItem->setFlag(ItemIsSelectable, false);
	d_ptr->mTextItem->setFlag(ItemIsMovable, false);
	if (!initText.isEmpty()) {
		setPlainText(initText);  // cn:文本
	}
}

bool DAGraphicsTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	DAGraphicsResizeableItem::saveToXml(doc, parentElement, ver);
	QDomElement e = doc->createElement("textItem");
	d_ptr->mTextItem->saveToXml(doc, &e, ver);
	parentElement->appendChild(e);
	return true;
}

bool DAGraphicsTextItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	DAGraphicsResizeableItem::loadFromXml(itemElement, ver);
	auto e = itemElement->firstChildElement("textItem");
	if (e.isNull()) {
		return false;
	}
	if (!d_ptr->mTextItem->loadFromXml(&e, ver)) {
		return false;
	}
	return true;
}

DAGraphicsStandardTextItem* DAGraphicsTextItem::textItem() const
{
	return d_ptr->mTextItem;
}

void DAGraphicsTextItem::setBodySize(const QSizeF& s)
{
	DAGraphicsResizeableItem::setBodySize(s);
	d_ptr->mTextItem->setPos(0, 0);
	d_ptr->mTextItem->document()->setPageSize(s);
	// d_ptr->mTextItem->setTextWidth(s.width());
}

/**
 * @brief 设置文本
 * @param v
 */
void DAGraphicsTextItem::setPlainText(const QString& v)
{
	d_ptr->mTextItem->setPlainText(v);
}

/**
 * @brief 文本
 * @return
 */
QString DAGraphicsTextItem::getPlainText() const
{
    return d_ptr->mTextItem->toPlainText();
}

/**
 * @brief 设置文本颜色
 * @param v
 */
void DAGraphicsTextItem::setSelectTextColor(const QColor& v)
{
    d_ptr->mTextItem->setSelectTextColor(v);
}

/**
 * @brief 获取文本颜色
 * @return
 */
QColor DAGraphicsTextItem::getSelectTextColor() const
{
    return d_ptr->mTextItem->getSelectTextColor();
}

/**
 * @brief 设置字体
 * @param v
 */
void DAGraphicsTextItem::setSelectTextFont(const QFont& v)
{
    d_ptr->mTextItem->setSelectTextFont(v);
}

/**
 * @brief 获取字体
 * @return
 */
QFont DAGraphicsTextItem::getSelectTextFont() const
{
    return d_ptr->mTextItem->getSelectTextFont();
}

/**
 * @brief 设置编辑模式
 * @param on
 */
void DAGraphicsTextItem::setEditable(bool on)
{
    d_ptr->mTextItem->setEditable(on);
}

/**
 * @brief 是否可编辑
 * @return
 */
bool DAGraphicsTextItem::isEditable() const
{
    return d_ptr->mTextItem->isEditable();
}

/**
 * @brief 获取doc
 * @return
 */
QTextDocument* DAGraphicsTextItem::document() const
{
    return d_ptr->mTextItem->document();
}

/**
 * @brief QGraphicsTextItem::textCursor
 * @return
 */
QTextCursor DAGraphicsTextItem::textCursor() const
{
    return d_ptr->mTextItem->textCursor();
}

/**
 * @brief 保存为富文本
 * @return
 */
QString DAGraphicsTextItem::toHtml() const
{
    return d_ptr->mTextItem->toHtml();
}

void DAGraphicsTextItem::setHtml(const QString& html)
{
    d_ptr->mTextItem->setHtml(html);
}

/**
 * @brief paintBody
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsTextItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
	// DAGraphicsResizeableItem::paintBody(painter, option, widget, bodyRect);
	if (isShowBorder()) {
		painter->save();
		painter->setPen(getBorderPen());
		painter->drawRect(bodyRect);
		painter->restore();
	}
}

void DAGraphicsTextItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	// qDebug() << "DAGraphicsTextItem::mousePressEvent";
	DAGraphicsResizeableItem::mousePressEvent(e);
	if (!isResizing()) {
		auto br = d_ptr->mTextItem->boundingRect();
		if (br.contains(e->pos())) {
			QTextCursor cursor(d_ptr->mTextItem->document());
			cursor.movePosition(QTextCursor::End);
			d_ptr->mTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
			d_ptr->mTextItem->setTextCursor(cursor);
			d_ptr->mTextItem->setFocus();  // 确保获得焦点以显示光标
		}
	}
}

}  // end da
