#include "DANodeLinkItemSettingWidget.h"
#include "ui_DANodeLinkItemSettingWidget.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DANodeGraphicsScene.h"
#include <QSignalBlocker>
#include <QPainter>
#include <QPixmap>
////////////////////////////////////////////////
///
////////////////////////////////////////////////

using namespace DA;

////////////////////////////////////////////////
/// DANodeLinkItemSettingWidget
////////////////////////////////////////////////
DANodeLinkItemSettingWidget::DANodeLinkItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DANodeLinkItemSettingWidget), _linkItem(nullptr), _scene(nullptr), _endpointIconSize(40, 20)
{
    ui->setupUi(this);
    ui->comboBoxLinkStyle->addItem(tr("Knuckle"), int(DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle));
    ui->comboBoxLinkStyle->addItem(tr("Straight"), int(DAAbstractNodeLinkGraphicsItem::LinkLineStraight));
    ui->comboBoxLinkStyle->addItem(tr("Bezier"), int(DAAbstractNodeLinkGraphicsItem::LinkLineBezier));
    initEndpointComboxBox();
    //信号透传
    connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DANodeLinkItemSettingWidget::onLinkLinePenChanged);
    connect(ui->comboBoxLinkStyle,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DANodeLinkItemSettingWidget::onComboBoxLinkStyleCurrentIndexChanged);
    connect(ui->spinBoxEndpointSize, QOverload< int >::of(&QSpinBox::valueChanged), this, &DANodeLinkItemSettingWidget::onSpinBoxEndpointSizeValueChanged);
    connect(ui->comboBoxFrontStyle,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DANodeLinkItemSettingWidget::onComboBoxFrontStyleCurrentIndexChanged);
    connect(ui->comboBoxEndStyle, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DANodeLinkItemSettingWidget::onComboBoxEndStyleCurrentIndexChanged);
}

DANodeLinkItemSettingWidget::~DANodeLinkItemSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置当前显示的连线样式，此函数不会触发@sa currentLinkLineStyleChanged 信号
 * @param s
 * @param updateLinkItem 如果为false，此函数只负责界面操作，并不会对持有的linkitem改变
 */
void DANodeLinkItemSettingWidget::setCurrentLinkLineStyle(DAAbstractNodeLinkGraphicsItem::LinkLineStyle s, bool updateLinkItem)
{
    QSignalBlocker b(ui->comboBoxLinkStyle);

    Q_UNUSED(b);
    switch (s) {
    case DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle:
        ui->comboBoxLinkStyle->setCurrentIndex(0);
        break;
    case DAAbstractNodeLinkGraphicsItem::LinkLineStraight:
        ui->comboBoxLinkStyle->setCurrentIndex(1);
        break;
    case DAAbstractNodeLinkGraphicsItem::LinkLineBezier:
        ui->comboBoxLinkStyle->setCurrentIndex(2);
        break;
    default:
        ui->comboBoxLinkStyle->setCurrentIndex(-1);
        break;
    }
    if (updateLinkItem && _linkItem) {
        _linkItem->setLinkLineStyle(s);
        _linkItem->update();
    }
}
/**
 * @brief 设置连线画笔，此函数不会触发@sa linkLinePenChanged 信号
 * @param p
 * @param updateLinkItem
 */
void DANodeLinkItemSettingWidget::setLinkLinePen(const QPen& p, bool updateLinkItem)
{
    QSignalBlocker b(ui->penEditWidget);
    Q_UNUSED(b);
    ui->penEditWidget->setCurrentPen(p);
    if (updateLinkItem && _linkItem) {
        _linkItem->setLinePen(p);
        _linkItem->update();
    }
}
/**
 * @brief 刷新数据
 */
void DANodeLinkItemSettingWidget::updateData()
{
    QSignalBlocker b(ui->comboBoxLinkStyle);
    if (nullptr == _linkItem) {
        ui->comboBoxLinkStyle->setCurrentIndex(-1);
        return;
    }
    setCurrentLinkLineStyle(_linkItem->getLinkLineStyle(), false);
    setLinkLinePen(_linkItem->getLinePen(), false);
    updateLinkEndpointInfo(_linkItem);
}

/**
 * @brief 设置item
 * @param link
 */
void DANodeLinkItemSettingWidget::setLinkItem(DAAbstractNodeLinkGraphicsItem* link)
{
    _linkItem = link;
    updateData();
}
/**
 * @brief 获取item
 * @return
 */
DAAbstractNodeLinkGraphicsItem* DANodeLinkItemSettingWidget::getLinkItem() const
{
    return _linkItem;
}

/**
 * @brief 设置端点的信息
 * @param link
 */
void DANodeLinkItemSettingWidget::updateLinkEndpointInfo(DAAbstractNodeLinkGraphicsItem* link)
{
    QSignalBlocker b(ui->spinBoxEndpointSize);
    QSignalBlocker b1(ui->comboBoxFrontStyle);
    QSignalBlocker b2(ui->comboBoxEndStyle);
    Q_UNUSED(b);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    ui->spinBoxEndpointSize->setValue(link->getEndPointSize());
    ui->comboBoxFrontStyle->setCurrentIndex(
        ui->comboBoxFrontStyle->findData((int)link->getEndPointType(DAAbstractNodeLinkGraphicsItem::OrientationStart)));
    ui->comboBoxEndStyle->setCurrentIndex(
        ui->comboBoxEndStyle->findData((int)link->getEndPointType(DAAbstractNodeLinkGraphicsItem::OrientationEnd)));
}

/**
 * @brief 设置scene
 * @param sc
 */
void DANodeLinkItemSettingWidget::setScene(DANodeGraphicsScene* sc)
{
    if (_scene) {
        disconnect(_scene, &DANodeGraphicsScene::nodeLinksRemoved, this, &DANodeLinkItemSettingWidget::onNodeLinksRemoved);
    }
    _scene = sc;
    if (_scene) {
        connect(_scene, &DANodeGraphicsScene::nodeLinksRemoved, this, &DANodeLinkItemSettingWidget::onNodeLinksRemoved);
    }
}

/**
 * @brief 初始化
 */
void DANodeLinkItemSettingWidget::initEndpointComboxBox()
{
    DAAbstractNodeLinkGraphicsItem item;
    ui->comboBoxFrontStyle->setIconSize(_endpointIconSize);
    ui->comboBoxEndStyle->setIconSize(_endpointIconSize);
    auto fn = [ this, &item ](DAAbstractNodeLinkGraphicsItem::EndPointType et, const QString& str = "") {
        this->ui->comboBoxFrontStyle->addItem(QIcon(this->generateEndPointPixmap(&item, et)), str, (int)et);
        this->ui->comboBoxEndStyle->addItem(QIcon(this->generateEndPointPixmap(&item, et)), str, (int)et);
    };
    fn(DAGraphicsLinkItem::EndPointNone, tr("None"));
    fn(DAGraphicsLinkItem::EndPointTriangType);
}

QPixmap DANodeLinkItemSettingWidget::generateEndPointPixmap(DAAbstractNodeLinkGraphicsItem* link, DAGraphicsLinkItem::EndPointType epType)
{
    QPixmap px(_endpointIconSize);
    px.fill(Qt::transparent);  //设置为透明底
    QPainter painter(&px);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);
    QPainterPath p = link->generateEndPointPainterPath(epType, px.height() * 0.8);
    QTransform tf;
    tf.translate(0, px.height() / 2);
    painter.setTransform(tf);
    painter.drawPath(p);
    painter.drawLine(0, 0, px.width(), 0);
    return px;
}

void DANodeLinkItemSettingWidget::onComboBoxLinkStyleCurrentIndexChanged(int index)
{
    DAGraphicsLinkItem::LinkLineStyle s = DAGraphicsLinkItem::LinkLineKnuckle;
    switch (index) {
    case 0: {
        s = DAGraphicsLinkItem::LinkLineKnuckle;
    } break;
    case 1: {
        s = DAGraphicsLinkItem::LinkLineStraight;
    } break;
    case 2: {
        s = DAGraphicsLinkItem::LinkLineBezier;
    } break;
    default:
        break;
    }
    if (_linkItem) {
        _linkItem->setLinkLineStyle(s);
        _linkItem->update();
    }
    emit currentLinkLineStyleChanged(s);
}

void DANodeLinkItemSettingWidget::onLinkLinePenChanged(const QPen& p)
{
    if (_linkItem) {
        _linkItem->setLinePen(p);
        _linkItem->update();
    }
    emit linkLinePenChanged(p);
}

void DANodeLinkItemSettingWidget::onNodeLinksRemoved(const QList< DAAbstractNodeLinkGraphicsItem* >& items)
{
    if (items.contains(_linkItem)) {
        _linkItem = nullptr;
        updateData();
    }
}

void DANodeLinkItemSettingWidget::onSpinBoxEndpointSizeValueChanged(int arg1)
{
    if (_linkItem) {
        _linkItem->setEndPointSize(arg1);
        _linkItem->update();
    }
}

void DANodeLinkItemSettingWidget::onComboBoxFrontStyleCurrentIndexChanged(int index)
{
    DAAbstractNodeLinkGraphicsItem::EndPointType et = static_cast< DAAbstractNodeLinkGraphicsItem::EndPointType >(index);
    if (_linkItem) {
        _linkItem->setEndPointType(DAGraphicsLinkItem::OrientationStart, et);
        _linkItem->update();
    }
}

void DANodeLinkItemSettingWidget::onComboBoxEndStyleCurrentIndexChanged(int index)
{
    DAAbstractNodeLinkGraphicsItem::EndPointType et = static_cast< DAAbstractNodeLinkGraphicsItem::EndPointType >(index);
    if (_linkItem) {
        _linkItem->setEndPointType(DAGraphicsLinkItem::OrientationEnd, et);
        _linkItem->update();
    }
}
