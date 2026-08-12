#include "DAGraphicsItemGroup.h"
#include "DAGraphicsItemFactory.h"
#include <QDomDocument>
#include <QDomElement>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QDebug>
#include "DAGraphicsItem.h"
#include "DAGraphicsScene.h"
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
	uint64_t mID { 0 };                ///< id
};

/**
 * @brief PrivateData 构造函数
 * @param p 父 DAGraphicsItemGroup 指针
 */
DAGraphicsItemGroup::PrivateData::PrivateData(DAGraphicsItemGroup* p) : q_ptr(p)
{
	// DAGraphicsItemFactory::generateID通过一个uint32_t生成一个uint64_t的id
	union Combine__ {
		uint32_t a;
		void* b;
	};
	Combine__ tmp;
	tmp.b = p;
	mID   = DAGraphicsItemFactory::generateID(tmp.a);
	mBorderPen.setColor(QColor(25, 152, 236));
	mBorderPen.setWidthF(1.1);
}

//===================================================
// DAGraphicsItem
//===================================================

/**
 * @brief DAGraphicsItemGroup 构造函数
 * @param parent 父 QGraphicsItem
 */
DAGraphicsItemGroup::DAGraphicsItemGroup(QGraphicsItem* parent) : QGraphicsItemGroup(parent), DA_PIMPL_CONSTRUCT
{
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemIsFocusable);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
	setHandlesChildEvents(false);
}

/**
 * @brief DAGraphicsItemGroup 析构函数
 */
DAGraphicsItemGroup::~DAGraphicsItemGroup()
{
}
/**
 * @brief 保存到xml中
 * @param doc
 * @param parentElement
 * @return
 */
bool DAGraphicsItemGroup::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	Q_UNUSED(doc);
	Q_UNUSED(parentElement);
	return true;
}

/**
  @brief 从xml中加载

  @note 注意，DAGraphicsItemGroup要先加入scene
  @param itemElement
  @return
 */
bool DAGraphicsItemGroup::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
	Q_UNUSED(parentElement);
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

/**
 * @brief 获取/设置 Item 的 ID
 * @param id Item 的 ID（仅 setter）
 * @return Item 的 ID（仅 getter）
 * @sa getItemID, setItemID
 */
uint64_t DAGraphicsItemGroup::getItemID() const
{
    return d_ptr->mID;
}

void DAGraphicsItemGroup::setItemID(uint64_t id)
{
    d_ptr->mID = id;
}

/**
   @brief 获取分组下的分组
   @return
 */
QList< DAGraphicsItemGroup* > DAGraphicsItemGroup::childGroups() const
{
	QList< DAGraphicsItemGroup* > res;
	QList< QGraphicsItem* > ci = childItems();
	for (auto i : std::as_const(ci)) {
		if (i->type() == ItemType_DAGraphicsItemGroup) {
			res.append(static_cast< DAGraphicsItemGroup* >(i));
		}
	}
	return res;
}

/**
   @brief 获取不包含分组的子item
   @return
 */
QList< QGraphicsItem* > DAGraphicsItemGroup::childItemsExcludingGrouping() const
{
	QList< QGraphicsItem* > res;
	QList< QGraphicsItem* > ci = childItems();
	for (auto i : std::as_const(ci)) {
		if (i->type() != ItemType_DAGraphicsItemGroup) {
			res.append(i);
		}
	}
	return res;
}

/**
 * @brief 绘制选中状态的边框
 * @param painter 画笔
 * @param option 图形项样式选项
 * @param widget 所属 widget
 */
void DAGraphicsItemGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (option->state & QStyle::State_Selected) {
		painter->setBrush(Qt::NoBrush);
		painter->setPen(d_ptr->mBorderPen);
		painter->drawRect(boundingRect());
	}
}

/**
 * @brief 图形项变化通知回调，处理位置变化时通知子 DAGraphicsItem
 * @param change 变化类型
 * @param value 变化的值
 * @return 传递给基类的处理结果
 */
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
	} break;
	default:
		break;
	}
	return QGraphicsItemGroup::itemChange(change, value);
}

}  // end of DA
