#include "DAStandardGraphicsTextItem.h"
#include <QFont>
#include <QDebug>
#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QDomDocument>
#include <QDomElement>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAStandardGraphicsTextItem
//===================================================
DAStandardGraphicsTextItem::DAStandardGraphicsTextItem(QGraphicsItem* parent) : QGraphicsTextItem(parent)
{
    initItem();
}

DAStandardGraphicsTextItem::DAStandardGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    initItem();
    setPlainText(str);
    setFont(f);
}

DAStandardGraphicsTextItem::DAStandardGraphicsTextItem(const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    initItem();
    setFont(f);
}

/**
 * @brief 设置编辑模式
 * @param on
 */
void DAStandardGraphicsTextItem::setEditMode(bool on)
{
    setTextInteractionFlags(on ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
}

bool DAStandardGraphicsTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    QDomElement textItemEle = doc->createElement("text-info");
    textItemEle.setAttribute("x", x());
    textItemEle.setAttribute("y", y());
    textItemEle.setAttribute("color", defaultTextColor().name());
    QDomElement textEle = doc->createElement("text");
    textEle.appendChild(doc->createTextNode(toPlainText()));

    textItemEle.appendChild(DAXMLFileInterface::makeElement(font(), "font", doc));
    textItemEle.appendChild(textEle);

    parentElement->appendChild(textItemEle);
    return true;
}

bool DAStandardGraphicsTextItem::loadFromXml(const QDomElement* itemElement)
{
    QDomElement textItemEle = itemElement->firstChildElement("text-info");
    if (textItemEle.isNull()) {
        return false;
    }
    QPointF pos;
    if (getStringRealValue(textItemEle.attribute("x"), pos.rx()) && getStringRealValue(textItemEle.attribute("y"), pos.ry())) {
        setPos(pos);
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

void DAStandardGraphicsTextItem::focusOutEvent(QFocusEvent* focusEvent)
{
    //在有选中的情况下，把选中的内容取消
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    cursor.setPosition(QTextCursor::Start);
    setTextCursor(cursor);
    setTextInteractionFlags(Qt::NoTextInteraction);
    QGraphicsTextItem::focusOutEvent(focusEvent);
}

void DAStandardGraphicsTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // czy:setTextInteractionFlags必须在setFocus之前，否则会出现异常
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setFocus();
    }
    QGraphicsTextItem::mouseDoubleClickEvent(event);
}

void DAStandardGraphicsTextItem::initItem()
{
    setDefaultTextColor(Qt::black);  //设置字体颜色
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    setTextInteractionFlags(Qt::TextEditorInteraction);
}
