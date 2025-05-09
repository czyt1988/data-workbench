#include "DANodeListWidget.h"
#include "DANodeMimeData.h"
#include "DAWorkFlowNodeListWidget.h"
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDrag>
#include <QApplication>
#define ROLE_META_DATA (Qt::UserRole + 1)
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DANodeListWidgetItem
//===================================================

DANodeListWidgetItem::DANodeListWidgetItem(const DANodeMetaData& node, QListWidget* listview)
    : QListWidgetItem(listview, ThisItemType)
{
    setNodeMetaData(node);
}

DANodeMetaData DANodeListWidgetItem::getNodeMetaData() const
{
    return (data(ROLE_META_DATA).value< DANodeMetaData >());
}

void DANodeListWidgetItem::setNodeMetaData(const DANodeMetaData& md)
{
	setIcon(md.getIcon());
	setText(md.getNodeName());
	QString tt = QString("<b>%1</b><br/>%2").arg(md.getNodeName(), md.getNodeTooltip());
	setToolTip(tt);
	setData(ROLE_META_DATA, QVariant::fromValue(md));
}

//===================================================
// DANodeListWidget
//===================================================
DANodeListWidget::DANodeListWidget(QWidget* p) : QListWidget(p)
{
	// qRegisterMetaType< DA::DANodeMetaData >("DA::DANodeMetaData");
	setViewMode(QListView::IconMode);
	setMovement(QListView::Snap);
	setIconSize(QSize(40, 40));
	setGridSize(QSize(100, fontMetrics().height() + 40 * 1.05));
	setFlow(QListView::LeftToRight);
	setWrapping(true);
	setResizeMode(QListView::Adjust);  // 一定要设置这个否则不会自动resize
	setDragDropMode(DragOnly);
}

void DANodeListWidget::addItems(const QList< DANodeMetaData >& nodeMetaDatas)
{
	for (const DANodeMetaData& d : nodeMetaDatas) {
		addItem(d);
	}
}

void DANodeListWidget::addItem(const DANodeMetaData& nodeMetaData)
{
	DANodeListWidgetItem* item = new DANodeListWidgetItem(nodeMetaData);
	QListWidget::addItem(item);
}

DANodeMetaData DANodeListWidget::getNodeMetaData(const QPoint& p) const
{
	QListWidgetItem* i = itemAt(p);
	if (!i) {
		return DANodeMetaData();
	}
	DANodeListWidgetItem* ni = static_cast< DANodeListWidgetItem* >(i);
	return ni->getNodeMetaData();
}

void DANodeListWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		_startPressPos = event->pos();
	}
	QListWidget::mousePressEvent(event);
}

void DANodeListWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		if ((event->pos() - _startPressPos).manhattanLength() > QApplication::startDragDistance()) {
			DANodeListWidgetItem* item = static_cast< DANodeListWidgetItem* >(itemAt(_startPressPos));
			if (item) {
				DANodeMetaData nodemd = item->getNodeMetaData();
				QDrag* drag           = DAWorkFlowNodeListWidget::createDrag(this, nodemd);
				drag->exec(Qt::MoveAction | Qt::CopyAction);
				return;
			}
		}
	}
	QListWidget::mouseMoveEvent(event);
}
