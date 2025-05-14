#include "DANodeItemSettingWidget.h"
#include <QDebug>
#include <QSignalBlocker>
#include <QActionGroup>
#include <QPointer>
#include <QButtonGroup>
#include "ui_DANodeItemSettingWidget.h"
#include "DANodeGraphicsScene.h"
#include "DAGraphicsResizeableItem.h"
#include "DACommandsForGraphics.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAGraphicsCommandsFactory.h"
namespace DA
{
class DANodeItemSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DANodeItemSettingWidget)
public:
	PrivateData(DANodeItemSettingWidget* p);
	QPointer< DAGraphicsResizeableItem > mItem { nullptr };
	QPointer< DANodeGraphicsScene > mScene { nullptr };
	QButtonGroup* mButtonGroupInputLocation { nullptr };
	QButtonGroup* mButtonGroupOutputLocation { nullptr };
};

DANodeItemSettingWidget::PrivateData::PrivateData(DANodeItemSettingWidget* p) : q_ptr(p)
{
}

////////////////////////////////////////////////
/// DANodeItemSettingWidget
////////////////////////////////////////////////
DANodeItemSettingWidget::DANodeItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DANodeItemSettingWidget), DA_PIMPL_CONSTRUCT
{
	ui->setupUi(this);
	d_ptr->mButtonGroupInputLocation = new QButtonGroup(this);
	d_ptr->mButtonGroupInputLocation->setExclusive(true);
	d_ptr->mButtonGroupInputLocation->addButton(ui->toolButtonInLeft, 0);
	d_ptr->mButtonGroupInputLocation->addButton(ui->toolButtonInTop, 1);
	d_ptr->mButtonGroupInputLocation->addButton(ui->toolButtonInRight, 2);
	d_ptr->mButtonGroupInputLocation->addButton(ui->toolButtonInBottom, 3);
	d_ptr->mButtonGroupOutputLocation = new QButtonGroup(this);
	d_ptr->mButtonGroupOutputLocation->setExclusive(true);
	d_ptr->mButtonGroupOutputLocation->addButton(ui->toolButtonOutLeft, 4);
	d_ptr->mButtonGroupOutputLocation->addButton(ui->toolButtonOutTop, 5);
	d_ptr->mButtonGroupOutputLocation->addButton(ui->toolButtonOutRight, 6);
	d_ptr->mButtonGroupOutputLocation->addButton(ui->toolButtonOutBottom, 7);

	connect(ui->doubleSpinBoxBodyWidth,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DANodeItemSettingWidget::onDoubleSpinBoxBodyWidthValueChanged);
	connect(ui->doubleSpinBoxBodyHeight,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DANodeItemSettingWidget::onDoubleSpinBoxBodyHeightValueChanged);
	connect(ui->doubleSpinBoxRotation,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DANodeItemSettingWidget::onDoubleSpinBoxRotationValueChanged);
	connect(ui->doubleSpinBoxX,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DANodeItemSettingWidget::onDoubleSpinBoxXValueChanged);
	connect(ui->doubleSpinBoxY,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DANodeItemSettingWidget::onDoubleSpinBoxYValueChanged);
	connect(ui->checkBoxMovable, &QCheckBox::stateChanged, this, &DANodeItemSettingWidget::onCheckBoxMovableStateChanged);
	connect(ui->checkBoxResizable, &QCheckBox::stateChanged, this, &DANodeItemSettingWidget::onCheckBoxResizableStateChanged);
#if QT_VERSION_MAJOR >= 6
	connect(d_ptr->mButtonGroupInputLocation, &QButtonGroup::idClicked, this, &DANodeItemSettingWidget::onButtonGroupClicked);
	connect(d_ptr->mButtonGroupOutputLocation, &QButtonGroup::idClicked, this, &DANodeItemSettingWidget::onButtonGroupClicked);
#else
	connect(d_ptr->mButtonGroupInputLocation,
			QOverload< int >::of(&QButtonGroup::buttonPressed),
			this,
			&DANodeItemSettingWidget::onButtonGroupClicked);
	connect(d_ptr->mButtonGroupOutputLocation,
			QOverload< int >::of(&QButtonGroup::buttonPressed),
			this,
			&DANodeItemSettingWidget::onButtonGroupClicked);
#endif
	connect(ui->textEditTooltip, &QTextEdit::textChanged, this, &DANodeItemSettingWidget::onTextEditTooltipTextChanged);
}

DANodeItemSettingWidget::~DANodeItemSettingWidget()
{
	delete ui;
}

void DANodeItemSettingWidget::setItem(DAGraphicsResizeableItem* item)
{
	d_ptr->mItem = item;
	updateData();
}
/**
 * @brief 获取维护的item
 * @return
 */
DAGraphicsResizeableItem* DANodeItemSettingWidget::getItem() const
{
    return d_ptr->mItem.data();
}

/**
 * @brief 设置了scene能实现redo/undo
 *
 * 如果设置nullptr，将不使用redo/undo
 * @param sc
 */
void DANodeItemSettingWidget::setScene(DANodeGraphicsScene* sc)
{
	if (d_ptr->mScene == sc) {
		return;
	}
	if (d_ptr->mScene) {
		disconnect(
			d_ptr->mScene.data(), &DANodeGraphicsScene::nodeItemsRemoved, this, &DANodeItemSettingWidget::onNodeItemsRemoved);
	}
	d_ptr->mScene = sc;
	if (d_ptr->mScene) {
		connect(d_ptr->mScene.data(), &DANodeGraphicsScene::nodeItemsRemoved, this, &DANodeItemSettingWidget::onNodeItemsRemoved);
	}
}

void DANodeItemSettingWidget::updateData()
{
	if (nullptr == d_ptr->mItem) {
		resetValue();
		return;
	}
	updatePosition();
	updateBodySize();
	updateItemState();
	updateRotation();
	updateLinkPointLocation();
}

void DANodeItemSettingWidget::updatePosition()
{
	QSignalBlocker b1(ui->doubleSpinBoxX), b2(ui->doubleSpinBoxY);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	ui->doubleSpinBoxX->setValue(d_ptr->mItem->x());
	ui->doubleSpinBoxY->setValue(d_ptr->mItem->y());
}

void DANodeItemSettingWidget::updateRotation()
{
	QSignalBlocker b1(ui->doubleSpinBoxRotation);
	Q_UNUSED(b1);
	ui->doubleSpinBoxRotation->setValue(d_ptr->mItem->rotation());
}

void DANodeItemSettingWidget::updateBodySize()
{
	QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	QSizeF originSize = d_ptr->mItem->getBodySize();
	QSizeF minSize    = d_ptr->mItem->getBodyMinimumSize();
	QSizeF maxSize    = d_ptr->mItem->getBodyMaximumSize();
	ui->doubleSpinBoxBodyWidth->setValue(originSize.width());
	ui->doubleSpinBoxBodyWidth->setMinimum(minSize.width());
	ui->doubleSpinBoxBodyWidth->setMaximum(maxSize.width());
	ui->doubleSpinBoxBodyHeight->setValue(originSize.height());
	ui->doubleSpinBoxBodyHeight->setMinimum(minSize.height());
	ui->doubleSpinBoxBodyHeight->setMaximum(maxSize.height());
}

void DANodeItemSettingWidget::updateItemState()
{
	QSignalBlocker b1(ui->checkBoxMovable), b2(ui->checkBoxResizable), b3(ui->textEditTooltip);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	Q_UNUSED(b3);
	// 网格的尺寸作为x,y的步长
	if (d_ptr->mScene) {
		QSize gs = d_ptr->mScene->getGridSize();
		ui->doubleSpinBoxX->setSingleStep(gs.width());
		ui->doubleSpinBoxY->setSingleStep(gs.height());
	}
	ui->checkBoxMovable->setChecked(d_ptr->mItem->isMovable());
	ui->checkBoxResizable->setChecked(d_ptr->mItem->isResizable());
	ui->doubleSpinBoxBodyWidth->setEnabled(d_ptr->mItem->isResizable());
	ui->doubleSpinBoxBodyHeight->setEnabled(d_ptr->mItem->isResizable());
	ui->doubleSpinBoxX->setEnabled(d_ptr->mItem->isMovable());
	ui->doubleSpinBoxY->setEnabled(d_ptr->mItem->isMovable());
	ui->textEditTooltip->setText(d_ptr->mItem->toolTip());
}

/**
 * @brief 更新连接点的位置
 */
void DANodeItemSettingWidget::updateLinkPointLocation()
{
	if (nullptr == d_ptr->mItem) {
		return;
	}
	DAAbstractNodeGraphicsItem* nodeItem = qobject_cast< DAAbstractNodeGraphicsItem* >(d_ptr->mItem);
	if (nullptr == nodeItem) {
		// 说明不是node link
		return;
	}
	QSignalBlocker b1(d_ptr->mButtonGroupInputLocation), b2(d_ptr->mButtonGroupOutputLocation);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	DAAbstractNodeGraphicsItem::LinkPointLocation ll = nodeItem->getLinkPointLocation(DANodeLinkPoint::Input);
	switch (ll) {
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
		ui->toolButtonInLeft->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
		ui->toolButtonInTop->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		ui->toolButtonInRight->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		ui->toolButtonInBottom->setChecked(true);
		break;
	default:
		break;
	}
	ll = nodeItem->getLinkPointLocation(DANodeLinkPoint::Output);
	switch (ll) {
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
		ui->toolButtonOutLeft->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
		ui->toolButtonOutTop->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		ui->toolButtonOutRight->setChecked(true);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		ui->toolButtonOutBottom->setChecked(true);
		break;
	default:
		break;
	}
}

void DANodeItemSettingWidget::onDoubleSpinBoxBodyWidthValueChanged(double v)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	QSizeF originSize = d->mItem->getBodySize();
	QSizeF willSetSize;
	if (ui->checkBoxLockAspectRatio->isChecked()) {
		willSetSize = originSize.scaled(v, originSize.height(), Qt::KeepAspectRatio);
	} else {
		willSetSize = QSizeF(v, originSize.height());
	}

	if (d->mScene) {
		// 如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
		if (originSize != willSetSize) {
			auto cmd = d->mScene->commandsFactory()->createItemResized(d->mItem, originSize, willSetSize);
			d->mScene->push(cmd);
		}
	} else {
		d->mItem->setBodySize(willSetSize);
	}

	QSizeF settedSize = d->mItem->getBodySize();
	QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
	ui->doubleSpinBoxBodyWidth->setValue(settedSize.width());
	ui->doubleSpinBoxBodyHeight->setValue(settedSize.height());
}

void DANodeItemSettingWidget::onDoubleSpinBoxBodyHeightValueChanged(double v)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	QSizeF originSize = d->mItem->getBodySize();
	QSizeF willSetSize;
	if (ui->checkBoxLockAspectRatio->isChecked()) {
		willSetSize = originSize.scaled(originSize.width(), v, Qt::KeepAspectRatio);
	} else {
		willSetSize = QSizeF(originSize.width(), v);
	}

	if (d->mScene) {
		// 如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
		if (d->mItem->getBodySize().height() == originSize.height()) {
			// 说明没设置成功,跳过
			return;
		}
		auto cmd = d->mScene->commandsFactory()->createItemResized(d->mItem, originSize, willSetSize);
		d->mScene->push(cmd);
	} else {
		d->mItem->setBodySize(willSetSize);
	}

	// 设置成功了，设置高度
	QSizeF settedSize = d->mItem->getBodySize();
	QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
	ui->doubleSpinBoxBodyWidth->setValue(settedSize.width());
	ui->doubleSpinBoxBodyHeight->setValue(settedSize.height());
}

void DANodeItemSettingWidget::onDoubleSpinBoxRotationValueChanged(double v)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	qreal oldRotation = d->mItem->rotation();
	if (qFuzzyCompare(oldRotation, v)) {
		return;
	}

	if (d->mScene) {
		// 如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
		if (!qFuzzyCompare(d->mItem->rotation(), v)) {
			// 说明没设置成功,跳过
			QSignalBlocker b(ui->doubleSpinBoxRotation);
			Q_UNUSED(b);
			ui->doubleSpinBoxRotation->setValue(d->mItem->rotation());
			return;
		}
		auto cmd = d->mScene->commandsFactory()->createItemRotation(d->mItem, oldRotation, v);
		d->mScene->push(cmd);
	} else {
		d->mItem->setRotation(v);
	}
}

void DANodeItemSettingWidget::onDoubleSpinBoxXValueChanged(double v)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	QPointF oldpos = d->mItem->pos();

	d->mItem->setScenePos(v, oldpos.y());
	QPointF newPos = d->mItem->pos();
	// 在存在网格的情况下，可能设置的位置不是显示的位置，
	QSignalBlocker b1(ui->doubleSpinBoxX), b2(ui->doubleSpinBoxY);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	ui->doubleSpinBoxX->setValue(newPos.x());
	ui->doubleSpinBoxY->setValue(newPos.y());
	if (d->mScene) {
		auto cmd = d->mScene->commandsFactory()->createItemMoved(d->mItem, oldpos, newPos, true);
		d->mScene->push(cmd);
	}
}

void DANodeItemSettingWidget::onDoubleSpinBoxYValueChanged(double v)
{
	DA_D(d);
	if (nullptr == d->mItem) {
		return;
	}
	QPointF oldpos = d->mItem->pos();
	d->mItem->setScenePos(oldpos.x(), v);
	QPointF newPos = d->mItem->pos();
	QSignalBlocker b1(ui->doubleSpinBoxX), b2(ui->doubleSpinBoxY);
	Q_UNUSED(b1);
	Q_UNUSED(b2);
	ui->doubleSpinBoxX->setValue(newPos.x());
	ui->doubleSpinBoxY->setValue(newPos.y());
	if (d->mScene) {
		auto cmd = d->mScene->commandsFactory()->createItemMoved(d->mItem, oldpos, newPos, true);
		d->mScene->push(cmd);
	}
}

void DANodeItemSettingWidget::onCheckBoxMovableStateChanged(int state)
{
	if (d_ptr->mItem) {
		d_ptr->mItem->setFlag(QGraphicsItem::ItemIsMovable, state == Qt::Checked);
		ui->doubleSpinBoxX->setEnabled(d_ptr->mItem->isMovable());
		ui->doubleSpinBoxY->setEnabled(d_ptr->mItem->isMovable());
	}
}

void DANodeItemSettingWidget::onCheckBoxResizableStateChanged(int state)
{
	if (d_ptr->mItem) {
		d_ptr->mItem->setEnableResize(state == Qt::Checked);
		d_ptr->mItem->update();
		ui->doubleSpinBoxBodyWidth->setEnabled(d_ptr->mItem->isResizable());
		ui->doubleSpinBoxBodyHeight->setEnabled(d_ptr->mItem->isResizable());
	}
}

void DANodeItemSettingWidget::onNodeItemsRemoved(const QList< DAAbstractNodeGraphicsItem* >& items)
{
	if (d_ptr->mItem) {
		for (const DAAbstractNodeGraphicsItem* i : qAsConst(items)) {
			if (i == d_ptr->mItem.data()) {
				d_ptr->mItem = nullptr;
				updateData();
			}
		}
	}
}

void DANodeItemSettingWidget::onButtonGroupClicked(int id)
{
	if (nullptr == d_ptr->mItem) {
		return;
	}
	DAAbstractNodeGraphicsItem* nodeItem = qobject_cast< DAAbstractNodeGraphicsItem* >(d_ptr->mItem);
	if (nullptr == nodeItem) {
		return;
	}
	switch (id) {
	case 0:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Input, DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide);
		break;
	case 1:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Input, DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide);
		break;
	case 2:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Input, DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide);
		break;
	case 3:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Input, DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide);
		break;
	case 4:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Output, DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide);
		break;
	case 5:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Output, DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide);
		break;
	case 6:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Output, DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide);
		break;
	case 7:
		nodeItem->setLinkPointLocation(DANodeLinkPoint::Output, DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide);
		break;
	default:
		break;
	}
	nodeItem->updateLinkPointPos();
	nodeItem->update();
}

void DANodeItemSettingWidget::onTextEditTooltipTextChanged()
{
	if (nullptr == d_ptr->mItem) {
		return;
	}
	QString t = ui->textEditTooltip->toPlainText();
	d_ptr->mItem->setToolTip(t);
}

void DANodeItemSettingWidget::resetValue()
{
	ui->doubleSpinBoxBodyWidth->setValue(0);
	ui->doubleSpinBoxBodyHeight->setValue(0);
	ui->doubleSpinBoxRotation->setValue(0);
	ui->doubleSpinBoxX->setValue(0);
	ui->doubleSpinBoxY->setValue(0);
	ui->checkBoxMovable->setChecked(false);
	ui->checkBoxResizable->setChecked(false);
	ui->textEditTooltip->clear();
}

}  // end of DA
