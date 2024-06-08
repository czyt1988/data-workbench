#include "DAGraphicsStandardTextItem.h"
#include <QFont>
#include <QDebug>
#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QDomDocument>
#include <QDomElement>
#include "DAGraphicsItemFactory.h"
namespace DA
{

//===================================================
// DAStandardGraphicsTextItem
//===================================================
DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(QGraphicsItem* parent) : QGraphicsTextItem(parent)
{
    initItem();
}

DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setPlainText(str);
	setFont(f);
}

DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setFont(f);
}

/**
 * @brief 设置编辑模式
 * @param on
 */
void DAGraphicsStandardTextItem::setEditable(bool on)
{
    setTextInteractionFlags(on ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
}

/**
 * @brief 是否可编辑
 * @return
 */
bool DAGraphicsStandardTextItem::isEditable() const
{
    return textInteractionFlags().testFlag(Qt::TextEditorInteraction);
}

bool DAGraphicsStandardTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	QDomElement textItemEle = doc->createElement("text-info");
	QPointF scPos           = scenePos();
	textItemEle.setAttribute("id", getItemID());
	textItemEle.setAttribute("x", scPos.x());
	textItemEle.setAttribute("y", scPos.y());
	textItemEle.setAttribute("color", defaultTextColor().name());
	QDomElement textEle = doc->createElement("text");
	textEle.appendChild(doc->createTextNode(toPlainText()));

	textItemEle.appendChild(DAXMLFileInterface::makeElement(font(), "font", doc));
	textItemEle.appendChild(textEle);

	parentElement->appendChild(textItemEle);
	return true;
}

bool DAGraphicsStandardTextItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	QDomElement textItemEle = itemElement->firstChildElement("text-info");
	if (textItemEle.isNull()) {
		return false;
	}
	QPointF pos;
	uint64_t id;
	if (getStringRealValue(textItemEle.attribute("x"), pos.rx())
		&& getStringRealValue(textItemEle.attribute("y"), pos.ry())) {
		setScenePos(pos);
	}
	if (getStringULongLongValue(textItemEle.attribute("id"), id)) {
		setItemID(id);
	}
	QColor color(textItemEle.attribute("color"));
	setDefaultTextColor(color);
	QDomElement fontEle = textItemEle.firstChildElement("font");
	if (!fontEle.isNull()) {
		QFont f = font();
		DAXMLFileInterface::loadElement(f, &fontEle);
		setFont(f);
	}
	QDomElement textEle = textItemEle.firstChildElement("text");
	if (!textEle.isNull()) {
		setPlainText(textEle.text());
	}
	return true;
}

void DAGraphicsStandardTextItem::setScenePos(const QPointF& p)
{
	setPos(mapToParent(mapFromScene(p)));
}

void DAGraphicsStandardTextItem::setScenePos(qreal x, qreal y)
{
	setScenePos(QPointF(x, y));
}

uint64_t DAGraphicsStandardTextItem::getItemID() const
{
	return mID;
}

void DAGraphicsStandardTextItem::setItemID(uint64_t id)
{
	mID = id;
}

/**
 * @brief 设置选中的颜色，如果没有选中对象，尝试全选
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextColor(const QColor& v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	QTextCharFormat format;
	// 设置文本颜色
	format.setForeground(QBrush(v));
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 获取选中的颜色，如果没有选中对象，尝试全选
 * @return
 */
QColor DAGraphicsStandardTextItem::getSelectTextColor() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	return cursor.charFormat().foreground().color();
}

/**
 * @brief 设置选中文本字体，如果没选中，将设置全部
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextFont(const QFont& v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 说明有选中文本
	QTextCharFormat format;
	format.setFont(v);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 选中文本字体，如果没选中，将设置全部
 * @return
 */
QFont DAGraphicsStandardTextItem::getSelectTextFont() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 获取该位置的字符格式
	QTextCharFormat format = cursor.charFormat();
	return format.font();
}

void DAGraphicsStandardTextItem::focusOutEvent(QFocusEvent* focusEvent)
{
	// 在有选中的情况下，把选中的内容取消
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	cursor.setPosition(QTextCursor::Start);
	setTextCursor(cursor);
	setTextInteractionFlags(Qt::NoTextInteraction);
	QGraphicsTextItem::focusOutEvent(focusEvent);
}

void DAGraphicsStandardTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		// czy:setTextInteractionFlags必须在setFocus之前，否则会出现异常
		setTextInteractionFlags(Qt::TextEditorInteraction);
		setFocus();
	}
	QGraphicsTextItem::mouseDoubleClickEvent(event);
}

void DAGraphicsStandardTextItem::initItem()
{
	union Combine__ {
		uint32_t a;
		void* b;
	};
	Combine__ tmp;
	tmp.b = this;
	mID   = DAGraphicsItemFactory::generateID(tmp.a);
	setDefaultTextColor(Qt::black);  // 设置字体颜色
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	setTextInteractionFlags(Qt::TextEditorInteraction);
}
}
