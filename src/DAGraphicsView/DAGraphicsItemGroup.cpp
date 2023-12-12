#include "DAGraphicsItemGroup.h"
#include <QDomDocument>
#include <QDomElement>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QDebug>
#include "DAGraphicsItem.h"
namespace DA
{

class DAGraphicsItemGroup::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsItemGroup)
public:
    PrivateData(DAGraphicsItemGroup* p);
    bool mIsShowBorder { false };      ///< 是否显示边框
    bool mIsShowBackground { false };  ///< 是否显示背景
    QPen mBorderPen;                   ///< 边框画笔
    QBrush mBackgroundBrush;           ///< 背景画刷
};

DAGraphicsItemGroup::PrivateData::PrivateData(DAGraphicsItemGroup* p) : q_ptr(p)
{
    mBorderPen.setColor(QColor(25, 152, 236));
    mBorderPen.setWidthF(1.1);
}

//===================================================
// DAGraphicsItem
//===================================================
DAGraphicsItemGroup::DAGraphicsItemGroup(QGraphicsItem* parent) : QGraphicsItemGroup(parent), DA_PIMPL_CONSTRUCT
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setHandlesChildEvents(false);
}

DAGraphicsItemGroup::~DAGraphicsItemGroup()
{
}
/**
 * @brief 保存到xml中
 * @param doc
 * @param parentElement
 * @return
 */
bool DAGraphicsItemGroup::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    // todo
    return true;
}

/**
 * @brief 从xml中加载
 * @param itemElement
 * @return
 */
bool DAGraphicsItemGroup::loadFromXml(const QDomElement* parentElement)
{
    // todo
    return true;
}

/**
 * @brief 设置边框画笔
 * @note 设置后不会重绘，重绘用户自己调用update
 * @param p
 */
void DAGraphicsItemGroup::setBorderPen(const QPen& p)
{
    d_ptr->mBorderPen = p;
}

/**
 * @brief 获取当前边框画笔
 * @return
 */
QPen DAGraphicsItemGroup::getBorderPen() const
{
    return d_ptr->mBorderPen;
}

/**
 * @brief 设置显示边框
 * @param on
 */
void DAGraphicsItemGroup::setShowBorder(bool on)
{
    d_ptr->mIsShowBorder = on;
}

/**
 * @brief 是否显示边框
 * @return
 */
bool DAGraphicsItemGroup::isShowBorder() const
{
    return d_ptr->mIsShowBorder;
}

/**
 * @brief 设置背景
 * @param b
 */
void DAGraphicsItemGroup::setBackgroundBrush(const QBrush& b)
{
    d_ptr->mBackgroundBrush = b;
}

/**
 * @brief 获取背景
 * @return
 */
QBrush DAGraphicsItemGroup::getBackgroundBrush() const
{
    return d_ptr->mBackgroundBrush;
}

/**
 * @brief 设置显示背景
 * @param on
 */
void DAGraphicsItemGroup::setShowBackground(bool on)
{
    d_ptr->mIsShowBackground = on;
}

/**
 * @brief 是否显示背景
 * @return
 */
bool DAGraphicsItemGroup::isShowBackground() const
{
    return d_ptr->mIsShowBackground;
}

void DAGraphicsItemGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (option->state & QStyle::State_Selected) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(d_ptr->mBorderPen);
        painter->drawRect(boundingRect());
    }
}

QVariant DAGraphicsItemGroup::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemPositionHasChanged: {
        QPointF p                   = value.toPointF();
        QList< QGraphicsItem* > cis = childItems();
        for (QGraphicsItem* i : std::as_const(cis)) {
            if (DAGraphicsItem* di = dynamic_cast< DAGraphicsItem* >(i)) {
                di->groupPositionChanged(p);
            }
        }
    }
    default:
        break;
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

}  // end of DA
