#include "DANodeItemSettingWidget.h"
#include <QDebug>
#include <QSignalBlocker>
#include <QActionGroup>
#include <QPointer>
#include "ui_DANodeItemSettingWidget.h"
#include "DANodeGraphicsScene.h"
#include "DAGraphicsResizeableItem.h"
#include "DACommandsForGraphics.h"
#include "DAAbstractNodeGraphicsItem.h"

namespace DA
{
class DANodeItemSettingWidgetPrivate
{
    DA_IMPL_PUBLIC(DANodeItemSettingWidget)
public:
    DANodeItemSettingWidgetPrivate(DANodeItemSettingWidget* p);
    QPointer< DAGraphicsResizeableItem > _item;
    QPointer< DANodeGraphicsScene > _scene;
    QActionGroup* _actionGroupInputLocation;
    QActionGroup* _actionGroupOutputLocation;
    QAction* _actionInLeft;
    QAction* _actionInTop;
    QAction* _actionInRight;
    QAction* _actionInBottom;
    QAction* _actionOutLeft;
    QAction* _actionOutTop;
    QAction* _actionOutRight;
    QAction* _actionOutBottom;
};

DANodeItemSettingWidgetPrivate::DANodeItemSettingWidgetPrivate(DANodeItemSettingWidget* p)
    : q_ptr(p), _item(nullptr), _scene(nullptr), _actionGroupInputLocation(nullptr), _actionGroupOutputLocation(nullptr)
{
}

////////////////////////////////////////////////
/// DANodeItemSettingWidget
////////////////////////////////////////////////
DANodeItemSettingWidget::DANodeItemSettingWidget(QWidget* parent)
    : QWidget(parent), d_ptr(new DANodeItemSettingWidgetPrivate(this)), ui(new Ui::DANodeItemSettingWidget)
{
    ui->setupUi(this);
    d_ptr->_actionGroupInputLocation  = new QActionGroup(this);
    d_ptr->_actionGroupOutputLocation = new QActionGroup(this);
    d_ptr->_actionGroupInputLocation->setExclusive(true);
    d_ptr->_actionGroupOutputLocation->setExclusive(true);
    //创建action
    auto createAction = [](const char* iconPath, QActionGroup* p, int dataId) -> QAction* {
        QAction* a = new QAction(p);
        a->setIcon(QIcon(iconPath));
        a->setCheckable(true);
        a->setData(dataId);
        return a;
    };
    d_ptr->_actionInLeft    = createAction(":/icon/icon/leftSideIn.svg", d_ptr->_actionGroupInputLocation, 0);
    d_ptr->_actionInTop     = createAction(":/icon/icon/topSideIn.svg", d_ptr->_actionGroupInputLocation, 1);
    d_ptr->_actionInRight   = createAction(":/icon/icon/rightSideIn.svg", d_ptr->_actionGroupInputLocation, 2);
    d_ptr->_actionInBottom  = createAction(":/icon/icon/bottomSideIn.svg", d_ptr->_actionGroupInputLocation, 3);
    d_ptr->_actionOutLeft   = createAction(":/icon/icon/leftSideOut.svg", d_ptr->_actionGroupOutputLocation, 4);
    d_ptr->_actionOutTop    = createAction(":/icon/icon/topSideOut.svg", d_ptr->_actionGroupOutputLocation, 5);
    d_ptr->_actionOutRight  = createAction(":/icon/icon/rightSideOut.svg", d_ptr->_actionGroupOutputLocation, 6);
    d_ptr->_actionOutBottom = createAction(":/icon/icon/bottomSideOut.svg", d_ptr->_actionGroupOutputLocation, 7);
    ui->toolButtonInLeft->setDefaultAction(d_ptr->_actionInLeft);
    ui->toolButtonInTop->setDefaultAction(d_ptr->_actionInTop);
    ui->toolButtonInRight->setDefaultAction(d_ptr->_actionInRight);
    ui->toolButtonInBottom->setDefaultAction(d_ptr->_actionInBottom);
    ui->toolButtonOutLeft->setDefaultAction(d_ptr->_actionOutLeft);
    ui->toolButtonOutTop->setDefaultAction(d_ptr->_actionOutTop);
    ui->toolButtonOutRight->setDefaultAction(d_ptr->_actionOutRight);
    ui->toolButtonOutBottom->setDefaultAction(d_ptr->_actionOutBottom);
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
    connect(ui->doubleSpinBoxX, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, &DANodeItemSettingWidget::onDoubleSpinBoxXValueChanged);
    connect(ui->doubleSpinBoxY, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, &DANodeItemSettingWidget::onDoubleSpinBoxYValueChanged);
    connect(ui->checkBoxMovable, &QCheckBox::stateChanged, this, &DANodeItemSettingWidget::onCheckBoxMovableStateChanged);
    connect(ui->checkBoxResizable, &QCheckBox::stateChanged, this, &DANodeItemSettingWidget::onCheckBoxResizableStateChanged);
    connect(d_ptr->_actionGroupInputLocation, &QActionGroup::triggered, this, &DANodeItemSettingWidget::onActionGroupTriggered);
    connect(d_ptr->_actionGroupOutputLocation, &QActionGroup::triggered, this, &DANodeItemSettingWidget::onActionGroupTriggered);
}

DANodeItemSettingWidget::~DANodeItemSettingWidget()
{
    delete ui;
}

void DANodeItemSettingWidget::setItem(DAGraphicsResizeableItem* item)
{
    d_ptr->_item = item;
    updateData();
}
/**
 * @brief 获取维护的item
 * @return
 */
DAGraphicsResizeableItem* DANodeItemSettingWidget::getItem() const
{
    return d_ptr->_item.data();
}

/**
 * @brief 设置了scene能实现redo/undo
 *
 * 如果设置nullptr，将不使用redo/undo
 * @param sc
 */
void DANodeItemSettingWidget::setScene(DANodeGraphicsScene* sc)
{
    if (d_ptr->_scene == sc) {
        return;
    }
    if (d_ptr->_scene) {
        disconnect(d_ptr->_scene.data(), &DANodeGraphicsScene::nodeItemsRemoved, this, &DANodeItemSettingWidget::onNodeItemsRemoved);
    }
    d_ptr->_scene = sc;
    if (d_ptr->_scene) {
        connect(d_ptr->_scene.data(), &DANodeGraphicsScene::nodeItemsRemoved, this, &DANodeItemSettingWidget::onNodeItemsRemoved);
    }
}

void DANodeItemSettingWidget::updateData()
{
    if (nullptr == d_ptr->_item) {
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
    ui->doubleSpinBoxX->setValue(d_ptr->_item->x());
    ui->doubleSpinBoxY->setValue(d_ptr->_item->y());
}

void DANodeItemSettingWidget::updateRotation()
{
    QSignalBlocker b1(ui->doubleSpinBoxRotation);
    Q_UNUSED(b1);
    ui->doubleSpinBoxRotation->setValue(d_ptr->_item->rotation());
}

void DANodeItemSettingWidget::updateBodySize()
{
    QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    QSizeF originSize = d_ptr->_item->getBodySize();
    QSizeF minSize    = d_ptr->_item->getBodyMinimumSize();
    QSizeF maxSize    = d_ptr->_item->getBodyMaximumSize();
    ui->doubleSpinBoxBodyWidth->setValue(originSize.width());
    ui->doubleSpinBoxBodyWidth->setMinimum(minSize.width());
    ui->doubleSpinBoxBodyWidth->setMaximum(maxSize.width());
    ui->doubleSpinBoxBodyHeight->setValue(originSize.height());
    ui->doubleSpinBoxBodyHeight->setMinimum(minSize.height());
    ui->doubleSpinBoxBodyHeight->setMaximum(maxSize.height());
}

void DANodeItemSettingWidget::updateItemState()
{
    QSignalBlocker b1(ui->checkBoxMovable), b2(ui->checkBoxResizable);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    //网格的尺寸作为x,y的步长
    if (d_ptr->_scene) {
        QSize gs = d_ptr->_scene->getGridSize();
        ui->doubleSpinBoxX->setSingleStep(gs.width());
        ui->doubleSpinBoxY->setSingleStep(gs.height());
    }
    ui->checkBoxMovable->setChecked(d_ptr->_item->isMovable());
    ui->checkBoxResizable->setChecked(d_ptr->_item->isEnableResize());
    ui->doubleSpinBoxBodyWidth->setEnabled(d_ptr->_item->isEnableResize());
    ui->doubleSpinBoxBodyHeight->setEnabled(d_ptr->_item->isEnableResize());
    ui->doubleSpinBoxX->setEnabled(d_ptr->_item->isMovable());
    ui->doubleSpinBoxY->setEnabled(d_ptr->_item->isMovable());
    ui->textEdit->setText(d_ptr->_item->toolTip());
}

/**
 * @brief 更新连接点的位置
 */
void DANodeItemSettingWidget::updateLinkPointLocation()
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    DAAbstractNodeGraphicsItem* nodeItem = qobject_cast< DAAbstractNodeGraphicsItem* >(d_ptr->_item);
    if (nullptr == nodeItem) {
        d_ptr->_actionGroupInputLocation->setDisabled(true);
        d_ptr->_actionGroupOutputLocation->setDisabled(true);
        return;
    }
    if (!d_ptr->_actionGroupInputLocation->isEnabled()) {
        d_ptr->_actionGroupInputLocation->setEnabled(true);
    }
    if (!d_ptr->_actionGroupOutputLocation->isEnabled()) {
        d_ptr->_actionGroupOutputLocation->setEnabled(true);
    }

    QSignalBlocker b1(d_ptr->_actionGroupInputLocation), b2(d_ptr->_actionGroupOutputLocation);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    DAAbstractNodeGraphicsItem::LinkPointLocation ll = nodeItem->getLinkPointLocation(DANodeLinkPoint::Input);
    switch (ll) {
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
        d_ptr->_actionInLeft->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
        d_ptr->_actionInTop->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
        d_ptr->_actionInRight->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
        d_ptr->_actionInBottom->setChecked(true);
        break;
    default:
        break;
    }
    ll = nodeItem->getLinkPointLocation(DANodeLinkPoint::Output);
    switch (ll) {
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
        d_ptr->_actionOutLeft->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
        d_ptr->_actionOutTop->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
        d_ptr->_actionOutRight->setChecked(true);
        break;
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
        d_ptr->_actionOutBottom->setChecked(true);
        break;
    default:
        break;
    }
}

void DANodeItemSettingWidget::onDoubleSpinBoxBodyWidthValueChanged(double v)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    QSizeF originSize = d_ptr->_item->getBodySize();
    if (ui->checkBoxLockAspectRatio->isChecked()) {
        d_ptr->_item->setBodySize(originSize.scaled(v, originSize.height(), Qt::KeepAspectRatio));
    } else {
        d_ptr->_item->setBodySize(QSizeF(v, originSize.height()));
    }
    QSizeF settedSize = d_ptr->_item->getBodySize();

    QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
    ui->doubleSpinBoxBodyWidth->setValue(settedSize.width());
    ui->doubleSpinBoxBodyHeight->setValue(settedSize.height());
    if (d_ptr->_scene) {
        //如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
        if (originSize != settedSize) {
            auto cmd = new DA::DACommandsForGraphicsItemResized(d_ptr->_item, originSize, settedSize, true);
            d_ptr->_scene->push(cmd);
        }
    }
}

void DANodeItemSettingWidget::onDoubleSpinBoxBodyHeightValueChanged(double v)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    QSizeF originSize = d_ptr->_item->getBodySize();
    if (ui->checkBoxLockAspectRatio->isChecked()) {
        d_ptr->_item->setBodySize(originSize.scaled(originSize.width(), v, Qt::KeepAspectRatio));
    } else {
        d_ptr->_item->setBodySize(QSizeF(originSize.width(), v));
    }
    QSizeF settedSize = d_ptr->_item->getBodySize();
    //设置成功了，设置高度
    QSignalBlocker b1(ui->doubleSpinBoxBodyWidth), b2(ui->doubleSpinBoxBodyHeight);
    ui->doubleSpinBoxBodyWidth->setValue(settedSize.width());
    ui->doubleSpinBoxBodyHeight->setValue(settedSize.height());
    if (d_ptr->_scene) {
        //如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
        if (d_ptr->_item->getBodySize().height() == originSize.height()) {
            //说明没设置成功,跳过
            return;
        }
        auto cmd = new DA::DACommandsForGraphicsItemResized(d_ptr->_item, originSize, settedSize, true);
        d_ptr->_scene->push(cmd);
    }
}

void DANodeItemSettingWidget::onDoubleSpinBoxRotationValueChanged(double v)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    qreal oldRotation = d_ptr->_item->rotation();
    if (qFuzzyCompare(oldRotation, v)) {
        qDebug() << "same rotation value , skip";
        return;
    }
    d_ptr->_item->setRotation(v);
    if (d_ptr->_scene) {
        //如果有scene，就把结果推入cmd,注意因为已经设置了bodysize，因此skipFirst一定要为true
        if (!qFuzzyCompare(d_ptr->_item->rotation(), v)) {
            //说明没设置成功,跳过
            qDebug() << "_item rotation value not equal setting value";
            QSignalBlocker b(ui->doubleSpinBoxRotation);
            Q_UNUSED(b);
            ui->doubleSpinBoxRotation->setValue(d_ptr->_item->rotation());
            return;
        }
        auto cmd = new DA::DACommandsForGraphicsItemRotation(d_ptr->_item, oldRotation, v, true);
        d_ptr->_scene->push(cmd);
    }
}

void DANodeItemSettingWidget::onDoubleSpinBoxXValueChanged(double v)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    QPointF oldpos = d_ptr->_item->pos();
    d_ptr->_item->setPos(v, oldpos.y());
    QPointF newPos = d_ptr->_item->pos();
    //在存在网格的情况下，可能设置的位置不是显示的位置，
    QSignalBlocker b1(ui->doubleSpinBoxX), b2(ui->doubleSpinBoxY);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    ui->doubleSpinBoxX->setValue(newPos.x());
    ui->doubleSpinBoxY->setValue(newPos.y());
    if (d_ptr->_scene) {
        auto cmd = new DA::DACommandsForGraphicsItemMoved(d_ptr->_item, oldpos, newPos, true);
        d_ptr->_scene->push(cmd);
    }
}

void DANodeItemSettingWidget::onDoubleSpinBoxYValueChanged(double v)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    QPointF oldpos = d_ptr->_item->pos();
    d_ptr->_item->setPos(oldpos.x(), v);
    QPointF newPos = d_ptr->_item->pos();
    QSignalBlocker b1(ui->doubleSpinBoxX), b2(ui->doubleSpinBoxY);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    ui->doubleSpinBoxX->setValue(newPos.x());
    ui->doubleSpinBoxY->setValue(newPos.y());
    if (d_ptr->_scene) {
        auto cmd = new DA::DACommandsForGraphicsItemMoved(d_ptr->_item, oldpos, newPos, true);
        d_ptr->_scene->push(cmd);
    }
}

void DANodeItemSettingWidget::onCheckBoxMovableStateChanged(int state)
{
    if (d_ptr->_item) {
        d_ptr->_item->setFlag(QGraphicsItem::ItemIsMovable, state == Qt::Checked);
        ui->doubleSpinBoxX->setEnabled(d_ptr->_item->isMovable());
        ui->doubleSpinBoxY->setEnabled(d_ptr->_item->isMovable());
    }
}

void DANodeItemSettingWidget::onCheckBoxResizableStateChanged(int state)
{
    if (d_ptr->_item) {
        d_ptr->_item->setEnableResize(state == Qt::Checked);
        d_ptr->_item->update();
        ui->doubleSpinBoxBodyWidth->setEnabled(d_ptr->_item->isEnableResize());
        ui->doubleSpinBoxBodyHeight->setEnabled(d_ptr->_item->isEnableResize());
    }
}

void DANodeItemSettingWidget::onNodeItemsRemoved(const QList< DAAbstractNodeGraphicsItem* >& items)
{
    if (d_ptr->_item) {
        for (const DAAbstractNodeGraphicsItem* i : qAsConst(items)) {
            if (i == d_ptr->_item.data()) {
                d_ptr->_item = nullptr;
                updateData();
            }
        }
    }
}

void DANodeItemSettingWidget::onActionGroupTriggered(QAction* a)
{
    if (nullptr == d_ptr->_item) {
        return;
    }
    DAAbstractNodeGraphicsItem* nodeItem = qobject_cast< DAAbstractNodeGraphicsItem* >(d_ptr->_item);
    if (nullptr == nodeItem) {
        return;
    }
    bool isok  = false;
    int dataId = a->data().toInt(&isok);
    if (!isok) {
        return;
    }

    switch (dataId) {
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

void DANodeItemSettingWidget::resetValue()
{
    ui->doubleSpinBoxBodyWidth->setValue(0);
    ui->doubleSpinBoxBodyHeight->setValue(0);
    ui->doubleSpinBoxRotation->setValue(0);
    ui->doubleSpinBoxX->setValue(0);
    ui->doubleSpinBoxY->setValue(0);
    ui->checkBoxMovable->setChecked(false);
    ui->checkBoxResizable->setChecked(false);
    ui->textEdit->setText("");
}

}  // end of DA
