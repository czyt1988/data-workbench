#include "DAGraphicsStandardTextItem.h"
#include <QFont>
#include <QDebug>
#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QDomDocument>
#include <QDomElement>
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
void DAGraphicsStandardTextItem::setEditMode(bool on)
{
    setTextInteractionFlags(on ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
}

bool DAGraphicsStandardTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    QDomElement textItemEle = doc->createElement("text-info");
    QPointF scPos           = scenePos();
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

bool DAGraphicsStandardTextItem::loadFromXml(const QDomElement* itemElement)
{
    QDomElement textItemEle = itemElement->firstChildElement("text-info");
    if (textItemEle.isNull()) {
        return false;
    }
    QPointF pos;
    if (getStringRealValue(textItemEle.attribute("x"), pos.rx()) && getStringRealValue(textItemEle.attribute("y"), pos.ry())) {
        setScenePos(pos);
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
    setDefaultTextColor(Qt::black);  // 设置字体颜色
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    setTextInteractionFlags(Qt::TextEditorInteraction);
}
}
