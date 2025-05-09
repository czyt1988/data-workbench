#include "DANodeTreeWidget.h"
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include "DAWorkFlowNodeListWidget.h"
#include "DANodeMimeData.h"
#include "DANodeMetaData.h"
#define ROLE_META_DATA (Qt::UserRole + 1)
namespace DA
{

DANodeTreeWidgetItem::DANodeTreeWidgetItem(const DANodeMetaData& md) : QTreeWidgetItem(ThisItemType)
{
    setNodeMetaData(md);
}

DANodeTreeWidgetItem::DANodeTreeWidgetItem(QTreeWidgetItem* parent, const DANodeMetaData& md)
    : QTreeWidgetItem(parent, ThisItemType)
{
    setNodeMetaData(md);
}

DANodeMetaData DANodeTreeWidgetItem::getNodeMetaData() const
{
    return (data(0, ROLE_META_DATA).value< DANodeMetaData >());
}

void DANodeTreeWidgetItem::setNodeMetaData(const DANodeMetaData& md)
{
	setIcon(0, md.getIcon());
	setText(0, md.getNodeName());
	QString tt = QString("<b>%1</b><br/>%2").arg(md.getNodeName(), md.getNodeTooltip());
	setToolTip(0, tt);
	setData(0, ROLE_META_DATA, QVariant::fromValue(md));
}

//===================================================
// DANodeTreeWidget
//===================================================

DANodeTreeWidget::DANodeTreeWidget(QWidget* par) : QTreeWidget(par), _favoriteItem(nullptr)
{
	// qRegisterMetaType< DA::DANodeMetaData >("DA::DANodeMetaData");
	setDragEnabled(true);  // 启用拖放
	setHeaderHidden(true);
}

/**
 * @brief 添加节点
 * @note 有潜在的内存泄漏风险，但实际上不会发生
 * @param nodeMetaDatas
 * @param grouped
 */
void DANodeTreeWidget::addItems(const QList< DANodeMetaData >& nodeMetaDatas)
{
	// 先提取分组，确认分组都建立
	QList< QString > orderGroup;
	QMap< DANodeMetaData, DANodeTreeWidgetItem* > nodeItems;
	for (const DANodeMetaData& md : qAsConst(nodeMetaDatas)) {
		// 1.每个md都生成一个item
		nodeItems[ md ] = new DANodeTreeWidgetItem(md);
		// 2.每个md的分组按顺序去重归集
		if (!orderGroup.contains(md.getGroup())) {
			orderGroup.append(md.getGroup());
		}
	}
	// 创建分组的topitem
	QHash< QString, QTreeWidgetItem* > groupItems;
	for (const QString& g : qAsConst(orderGroup)) {
		QTreeWidgetItem* gitem = new QTreeWidgetItem({ g });
		insertTopLevelItem(topLevelItemCount(), gitem);
		groupItems[ g ] = gitem;
	}
	// 把节点item挂载到分组中
	for (const DANodeMetaData& md : qAsConst(nodeMetaDatas)) {
		QTreeWidgetItem* gitem      = groupItems.value(md.getGroup(), nullptr);
		DANodeTreeWidgetItem* nitem = nodeItems.value(md, nullptr);
		if (gitem && nitem) {
			gitem->addChild(nitem);
		}
	}
}

/**
 * @brief 逐个添加
 *
 * @note 注意此效率非常低
 * @param md
 * @param grouped
 */
void DANodeTreeWidget::addItem(const DANodeMetaData& md)
{
	QTreeWidgetItem* root = nullptr;

	int rootcnt = topLevelItemCount();
	for (int i = 0; i < rootcnt; ++i) {
		QTreeWidgetItem* topit = topLevelItem(i);
		if (md.getGroup() == topit->text(0)) {
			root = topit;
			break;
		}
	}

	if (nullptr == root) {
		root = new QTreeWidgetItem({ md.getGroup() });
		insertTopLevelItem(topLevelItemCount(), root);
	}
	DANodeTreeWidgetItem* i = new DANodeTreeWidgetItem(root, md);
	Q_UNUSED(i);
}

/**
 * @brief 添加到收藏
 * @param md
 */
void DANodeTreeWidget::addToFavorite(const DANodeMetaData& md)
{
	QTreeWidgetItem* favItem = createFavoriteItem();
	DANodeTreeWidgetItem* i  = new DANodeTreeWidgetItem(favItem, md);
	Q_UNUSED(i);
}

/**
 * @brief 移除收藏
 * @param md
 */
void DANodeTreeWidget::removeFavorite(const DANodeMetaData& md)
{
	QTreeWidgetItem* favItem = getFavoriteItem();
	int c                    = favItem->childCount();
	QList< QTreeWidgetItem* > needDelete;
	for (int i = 0; i < c; ++i) {
		QTreeWidgetItem* item = favItem->child(i);
		if (DANodeTreeWidgetItem::ThisItemType == item->type()) {
			DANodeTreeWidgetItem* nitem = static_cast< DANodeTreeWidgetItem* >(item);
			if (md == nitem->getNodeMetaData()) {
				needDelete.append(item);
			}
		}
	}
	for (QTreeWidgetItem* i : needDelete) {
		delete i;
	}
}

/**
 * @brief 收藏item
 *
 * @note 如果没有会创建
 * @return
 */
QTreeWidgetItem* DANodeTreeWidget::getFavoriteItem()
{
	if (nullptr == _favoriteItem) {
		return createFavoriteItem();
	}
	return _favoriteItem;
}

QTreeWidgetItem* DANodeTreeWidget::createFavoriteItem()
{
	if (_favoriteItem) {
		return _favoriteItem;
	}
	_favoriteItem = new QTreeWidgetItem({ tr("Favorite") });
	_favoriteItem->setIcon(0, QIcon(":/DAGui/icon/favorite.svg"));
	insertTopLevelItem(0, _favoriteItem);
	return _favoriteItem;
}

/**
 * @brief 通过位置获取对应的md
 * @param p
 * @return
 */
DANodeMetaData DANodeTreeWidget::getNodeMetaData(const QPoint& p) const
{
	QTreeWidgetItem* item = itemAt(p);
	if (!item) {
		return DANodeMetaData();
	}
	if (DANodeTreeWidgetItem::ThisItemType != item->type()) {
		return DANodeMetaData();
	}
	DANodeTreeWidgetItem* nitem = static_cast< DANodeTreeWidgetItem* >(item);
	return nitem->getNodeMetaData();
}

void DANodeTreeWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		_startPressPos = event->pos();
	}
	QTreeWidget::mousePressEvent(event);
}

void DANodeTreeWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		if ((event->pos() - _startPressPos).manhattanLength() > QApplication::startDragDistance()) {
			QTreeWidgetItem* pitem = itemAt(_startPressPos);
			if (DANodeTreeWidgetItem::ThisItemType != pitem->type()) {
				QTreeWidget::mouseMoveEvent(event);
				return;
			}
			DANodeTreeWidgetItem* nitem = static_cast< DANodeTreeWidgetItem* >(pitem);
			DANodeMetaData nodemd       = nitem->getNodeMetaData();
			QDrag* drag                 = DAWorkFlowNodeListWidget::createDrag(this, nodemd);
			drag->exec(Qt::MoveAction | Qt::CopyAction);
			return;

			return;
		}
	}
}
}
