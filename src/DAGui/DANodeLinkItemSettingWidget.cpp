#include "DANodeLinkItemSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAPropertyItemWidget.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include <QSignalBlocker>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include <QComboBox>
////////////////////////////////////////////////////
///
////////////////////////////////////////////////////

using namespace DA;

////////////////////////////////////////////////////
/// DANodeLinkItemSettingWidget
////////////////////////////////////////////////////
DANodeLinkItemSettingWidget::DANodeLinkItemSettingWidget(QWidget* parent)
    : QWidget(parent), mPanel(nullptr), _comboBoxFrontStyle(nullptr), _comboBoxEndStyle(nullptr), _linkItem(nullptr), _scene(nullptr), _endpointIconSize(40, 20)
{
    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(6);

    // 创建属性面板容器
    mPanel = new DAPropertyPanelContainerWidget(this);
    layout->addWidget(mPanel);

    // 添加画笔属性
    mPanel->addPenProperty(PropertyPen, tr("pen"));

    // 添加连线样式枚举属性
    mPanel->addEnumProperty(PropertyLinkStyle,
                            tr("link style"),
                            QStringList{ tr("Knuckle"), tr("Straight"), tr("Bezier") },
                            QList<int>{ int(DAGraphicsLinkItem::LinkLineKnuckle),
                                        int(DAGraphicsLinkItem::LinkLineStraight),
                                        int(DAGraphicsLinkItem::LinkLineBezier) });

    // 添加端点大小整数属性
    mPanel->addIntProperty(PropertyEndpointSize, tr("end point size"), 0, 0, 999);

    // 创建自定义端点样式下拉框并初始化
    _comboBoxFrontStyle = new QComboBox();
    _comboBoxEndStyle   = new QComboBox();
    initEndpointComboxBox();

    // 添加前端点样式属性(自定义编辑器)
    mPanel->addProperty(PropertyFrontStyle, tr("front style"), _comboBoxFrontStyle);

    // 添加后端点样式属性(自定义编辑器)
    mPanel->addProperty(PropertyEndStyle, tr("end style"), _comboBoxEndStyle);

    // 连接属性面板信号
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DANodeLinkItemSettingWidget::onPropertyValueChanged);

    // 连接自定义下拉框信号
    connect(_comboBoxFrontStyle, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DANodeLinkItemSettingWidget::onComboBoxFrontStyleCurrentIndexChanged);
    connect(_comboBoxEndStyle, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DANodeLinkItemSettingWidget::onComboBoxEndStyleCurrentIndexChanged);
}

DANodeLinkItemSettingWidget::~DANodeLinkItemSettingWidget()
{
}

/**
 * @brief 设置当前显示的连线样式，此函数不会触发@sa currentLinkLineStyleChanged 信号
 * @param s 连线样式
 * @param updateLinkItem 如果为false，此函数只负责界面操作，并不会对持有的linkitem改变
 */
void DANodeLinkItemSettingWidget::setCurrentLinkLineStyle(DAGraphicsLinkItem::LinkLineStyle s, bool updateLinkItem)
{
    QSignalBlocker b(mPanel);
    Q_UNUSED(b);
    mPanel->setEnumValue(PropertyLinkStyle, int(s));
    if (updateLinkItem && _linkItem) {
        _linkItem->setLinkLineStyle(s);
        _linkItem->update();
    }
}
/**
 * @brief 设置连线画笔，此函数不会触发@sa linkLinePenChanged 信号
 * @param p 画笔
 * @param updateLinkItem 如果为true，同时更新linkitem的画笔
 */
void DANodeLinkItemSettingWidget::setLinkLinePen(const QPen& p, bool updateLinkItem)
{
    QSignalBlocker b(mPanel);
    Q_UNUSED(b);
    mPanel->setPenValue(PropertyPen, p);
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
    QSignalBlocker bPanel(mPanel);
    QSignalBlocker bFront(_comboBoxFrontStyle);
    QSignalBlocker bEnd(_comboBoxEndStyle);
    Q_UNUSED(bPanel);
    Q_UNUSED(bFront);
    Q_UNUSED(bEnd);
    if (nullptr == _linkItem) {
        // 清空连线样式选择
        DAPropertyItemWidget* item = mPanel->getPropertyItem(PropertyLinkStyle);
        if (item) {
            QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
            if (combo) {
                combo->setCurrentIndex(-1);
            }
        }
        return;
    }
    setCurrentLinkLineStyle(_linkItem->getLinkLineStyle(), false);
    setLinkLinePen(_linkItem->getLinePen(), false);
    updateLinkEndpointInfo(_linkItem);
}

/**
 * @brief 设置item
 * @param link 连线item
 */
void DANodeLinkItemSettingWidget::setLinkItem(DAPyLinkGraphicsItem* link)
{
    _linkItem = link;
    updateData();
}
/**
 * @brief 获取item
 * @return 连线item指针
 */
DAPyLinkGraphicsItem* DANodeLinkItemSettingWidget::getLinkItem() const
{
    return _linkItem;
}

/**
 * @brief 设置端点的信息
 * @param link 连线item
 */
void DANodeLinkItemSettingWidget::updateLinkEndpointInfo(DAPyLinkGraphicsItem* link)
{
    QSignalBlocker bPanel(mPanel);
    QSignalBlocker bFront(_comboBoxFrontStyle);
    QSignalBlocker bEnd(_comboBoxEndStyle);
    Q_UNUSED(bPanel);
    Q_UNUSED(bFront);
    Q_UNUSED(bEnd);
    mPanel->setIntValue(PropertyEndpointSize, link->getEndPointSize());
    _comboBoxFrontStyle->setCurrentIndex(
        _comboBoxFrontStyle->findData((int)link->getEndPointType(DAGraphicsLinkItem::OrientationStart)));
    _comboBoxEndStyle->setCurrentIndex(
        _comboBoxEndStyle->findData((int)link->getEndPointType(DAGraphicsLinkItem::OrientationEnd)));
}

/**
 * @brief 设置scene
 * @param sc 工作流场景
 */
void DANodeLinkItemSettingWidget::setScene(DAPyWorkFlowGraphicsScene* sc)
{
    if (_scene) {
        disconnect(_scene, &DAPyWorkFlowScene::pyNodeLinksRemoved, this, &DANodeLinkItemSettingWidget::onNodeLinksRemoved);
    }
    _scene = sc;
    if (_scene) {
        connect(_scene, &DAPyWorkFlowScene::pyNodeLinksRemoved, this, &DANodeLinkItemSettingWidget::onNodeLinksRemoved);
    }
}

/**
 * @brief 初始化端点样式下拉框
 */
void DANodeLinkItemSettingWidget::initEndpointComboxBox()
{
    DAPyLinkGraphicsItem item;
    _comboBoxFrontStyle->setIconSize(_endpointIconSize);
    _comboBoxEndStyle->setIconSize(_endpointIconSize);
    auto fn = [ this, &item ](DAGraphicsLinkItem::EndPointType et, const QString& str = "") {
        _comboBoxFrontStyle->addItem(QIcon(generateEndPointPixmap(&item, et)), str, (int)et);
        _comboBoxEndStyle->addItem(QIcon(generateEndPointPixmap(&item, et)), str, (int)et);
    };
    fn(DAGraphicsLinkItem::EndPointNone, tr("None"));
    fn(DAGraphicsLinkItem::EndPointTriangType);
}

/**
 * @brief 生成端点图标
 * @param link 连线item（用于生成路径）
 * @param epType 端点类型
 * @return 端点图标QPixmap
 */
QPixmap DANodeLinkItemSettingWidget::generateEndPointPixmap(DAPyLinkGraphicsItem* link, DAGraphicsLinkItem::EndPointType epType)
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

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DANodeLinkItemSettingWidget::onPropertyValueChanged(int propertyId)
{
    switch (propertyId) {
    case PropertyPen: {
        QPen p = mPanel->getPenValue(PropertyPen);
        if (_linkItem) {
            _linkItem->setLinePen(p);
            _linkItem->update();
        }
        emit linkLinePenChanged(p);
        break;
    }
    case PropertyLinkStyle: {
        int val = mPanel->getEnumValue(PropertyLinkStyle);
        DAGraphicsLinkItem::LinkLineStyle s = static_cast< DAGraphicsLinkItem::LinkLineStyle >(val);
        if (_linkItem) {
            _linkItem->setLinkLineStyle(s);
            _linkItem->update();
        }
        emit currentLinkLineStyleChanged(s);
        break;
    }
    case PropertyEndpointSize: {
        int val = mPanel->getIntValue(PropertyEndpointSize);
        if (_linkItem) {
            _linkItem->setEndPointSize(val);
            _linkItem->update();
        }
        break;
    }
    default:
        break;
    }
}

void DANodeLinkItemSettingWidget::onComboBoxFrontStyleCurrentIndexChanged(int index)
{
    DAGraphicsLinkItem::EndPointType et = static_cast< DAGraphicsLinkItem::EndPointType >(_comboBoxFrontStyle->itemData(index).toInt());
    if (_linkItem) {
        _linkItem->setEndPointType(DAGraphicsLinkItem::OrientationStart, et);
        _linkItem->update();
    }
}

void DANodeLinkItemSettingWidget::onComboBoxEndStyleCurrentIndexChanged(int index)
{
    DAGraphicsLinkItem::EndPointType et = static_cast< DAGraphicsLinkItem::EndPointType >(_comboBoxEndStyle->itemData(index).toInt());
    if (_linkItem) {
        _linkItem->setEndPointType(DAGraphicsLinkItem::OrientationEnd, et);
        _linkItem->update();
    }
}

void DANodeLinkItemSettingWidget::onNodeLinksRemoved(const QList< DAPyLinkGraphicsItem* >& items)
{
    if (items.contains(_linkItem)) {
        _linkItem = nullptr;
        updateData();
    }
}