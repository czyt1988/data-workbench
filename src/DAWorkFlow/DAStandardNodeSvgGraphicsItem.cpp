#include "DAStandardNodeSvgGraphicsItem.h"
#include <QPainter>
#include <QFontMetrics>
#include "DANodePalette.h"
#include "DAAbstractNode.h"
#include <QGraphicsScene>
#include <QApplication>
#include <QGraphicsSimpleTextItem>
#include <QSvgRenderer>
#include <QStyleOptionGraphicsItem>

namespace DA
{
class DAStandardNodeSvgGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAStandardNodeSvgGraphicsItem)
public:
    DAStandardNodeSvgGraphicsItemPrivate(DAStandardNodeSvgGraphicsItem* p);
    DAStandardNodeSvgGraphicsItemPrivate(DAStandardNodeSvgGraphicsItem* p, QSvgRenderer* sharedrenderer);
    void updateDefaultSize();
    QSizeF getTextSize() const;
    void updateSvgPaintRect();
    //按照AspectRatioMode得到的尺寸
    QSizeF getAspectRatioSize(const QSizeF& s) const;

public:
    bool _shared;
    QSvgRenderer* _renderer;
    QSize _defaultSize;
    QString _elemId;
    QRectF _nodeNameRect;
    QRectF _svgPaintRect;
    QMarginsF _margins;
    QGraphicsSimpleTextItem* _itemText;
    Qt::AspectRatioMode _aspectRatioMode;
};
}  // end of namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAStandardNodeSvgGraphicsItemPrivate
//===================================================

DAStandardNodeSvgGraphicsItemPrivate::DAStandardNodeSvgGraphicsItemPrivate(DAStandardNodeSvgGraphicsItem* p)
    : q_ptr(p), _shared(false), _aspectRatioMode(Qt::KeepAspectRatio)
{
    _itemText = new QGraphicsSimpleTextItem(p);
    _renderer = new QSvgRenderer(p);
    QObject::connect(_renderer, &QSvgRenderer::repaintNeeded, p, &DAStandardNodeSvgGraphicsItem::repaintItem);
    p->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

DAStandardNodeSvgGraphicsItemPrivate::DAStandardNodeSvgGraphicsItemPrivate(DAStandardNodeSvgGraphicsItem* p, QSvgRenderer* sharedrenderer)
    : q_ptr(p), _shared(true)
{
    _itemText = new QGraphicsSimpleTextItem(p);
    p->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    _renderer = sharedrenderer;
}
/**
 * @brief _defaultSize可以记录最初的比例
 */
void DAStandardNodeSvgGraphicsItemPrivate::updateDefaultSize()
{
    if (_elemId.isEmpty()) {
        _defaultSize = _renderer->defaultSize();
    } else {
        _defaultSize = _renderer->boundsOnElement(_elemId).size().toSize();
    }
}

QSizeF DAStandardNodeSvgGraphicsItemPrivate::getTextSize() const
{
    return _itemText->boundingRect().size();
}

void DAStandardNodeSvgGraphicsItemPrivate::updateSvgPaintRect()
{
    QRectF r      = q_ptr->getBodyRect();
    _svgPaintRect = r.adjusted(_margins.left(), _margins.top(), -_margins.right(), -_margins.bottom());
}

QSizeF DAStandardNodeSvgGraphicsItemPrivate::getAspectRatioSize(const QSizeF& s) const
{
    QSizeF newSize = _defaultSize;
    newSize.scale(s, _aspectRatioMode);
    return newSize;
}

//==============================================================
// DAStandardNodeSvgGraphicsItem
//==============================================================

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), d_ptr(new DAStandardNodeSvgGraphicsItemPrivate(this))
{
    setEnableResize(true);
    setEnableMoveText(true);
    setText(n->getNodeName());
}

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, const QString& svgfile, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), d_ptr(new DAStandardNodeSvgGraphicsItemPrivate(this))
{
    setEnableResize(true);
    setEnableMoveText(true);
    setSvg(svgfile);
    d_ptr->updateSvgPaintRect();
    setText(n->getNodeName());
}

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QSvgRenderer* sharedrender, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), d_ptr(new DAStandardNodeSvgGraphicsItemPrivate(this, sharedrender))
{
    setEnableResize(true);
    setEnableMoveText(true);
    d_ptr->updateSvgPaintRect();
    setText(n->getNodeName());
}

DAStandardNodeSvgGraphicsItem::~DAStandardNodeSvgGraphicsItem()
{
}

void DAStandardNodeSvgGraphicsItem::setBodySize(const QSizeF& s)
{
    //设置尺寸
    QSizeF ss = testBodySize(s);
    ss        = d_ptr->getAspectRatioSize(ss);
    //计算比例

    DAAbstractNodeGraphicsItem::setBodySize(ss);
    //最后重新计算连接点必须在setBodySize之后
    updateLinkPointPos();
    //刷新文字位置
    QSizeF ts = getTextSize();
    ss        = getBodySize();
    d_ptr->updateSvgPaintRect();
    d_ptr->_itemText->setPos((ss.width() - ts.width()) / 2, ss.height() + 5);
}

void DAStandardNodeSvgGraphicsItem::setEnableMoveText(bool on)
{
    d_ptr->_itemText->setFlag(ItemIsMovable, on);
}

bool DAStandardNodeSvgGraphicsItem::isEnableMoveText() const
{
    return d_ptr->_itemText->flags().testFlag(ItemIsMovable);
}

/**
 * @brief 获取svg渲染器
 * @return
 */
QSvgRenderer* DAStandardNodeSvgGraphicsItem::renderer() const
{
    return d_ptr->_renderer;
}

void DAStandardNodeSvgGraphicsItem::setSharedRenderer(QSvgRenderer* renderer)
{
    if (!d_ptr->_shared)
        delete d_ptr->_renderer;

    d_ptr->_renderer = renderer;
    d_ptr->_shared   = true;

    d_ptr->updateDefaultSize();

    update();
}

/**
 * @brief DAStandardNodeSvgGraphicsItem::paintBody
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAStandardNodeSvgGraphicsItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(widget);
    Q_UNUSED(bodyRect);
    if (!d_ptr->_renderer->isValid())
        return;

    if (d_ptr->_elemId.isEmpty())
        d_ptr->_renderer->render(painter, d_ptr->_svgPaintRect);
    else
        d_ptr->_renderer->render(painter, d_ptr->_elemId, d_ptr->_svgPaintRect);
    //绘制连接点
    paintLinkPoints(painter, option, widget);
}

/**
 * @brief 等价 QGraphicsSvgItem::setElementId
 * @param id
 */
void DAStandardNodeSvgGraphicsItem::setElementId(const QString& id)
{
    d_ptr->_elemId = id;
    d_ptr->updateDefaultSize();
    update();
}

QString DAStandardNodeSvgGraphicsItem::elementId() const
{
    return d_ptr->_elemId;
}

void DAStandardNodeSvgGraphicsItem::setCachingEnabled(bool caching)
{
    setCacheMode(caching ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
}

bool DAStandardNodeSvgGraphicsItem::isCachingEnabled() const
{
    return cacheMode() != QGraphicsItem::NoCache;
}

QSizeF DAStandardNodeSvgGraphicsItem::getTextSize() const
{
    return d_ptr->getTextSize();
}

/**
 * @brief 设置文本
 * @param t
 */
void DAStandardNodeSvgGraphicsItem::setText(const QString& t)
{
    d_ptr->_itemText->setText(t);
}

/**
 * @brief 设置svg
 * @param svgfile
 * @return 成功返回true
 */
bool DAStandardNodeSvgGraphicsItem::setSvg(const QString& svgfile)
{
    if (renderer()->load(svgfile)) {
        d_ptr->updateDefaultSize();
        repaintItem();
        return true;
    }
    return false;
}

/**
 * @brief 把bodysize设置为svg的default size
 */
void DAStandardNodeSvgGraphicsItem::resetBodySize()
{
    setBodySize(d_ptr->_defaultSize);
}
/**
 * @brief 设置图片变换时的比例
 * @param m
 */
void DAStandardNodeSvgGraphicsItem::setAspectRatioMode(Qt::AspectRatioMode m)
{
    d_ptr->_aspectRatioMode = m;
    setBodySize(getBodySize());
}
/**
 * @brief 获取图片变换时的比例
 * @return
 */
Qt::AspectRatioMode DAStandardNodeSvgGraphicsItem::getAspectRatioMode() const
{
    return d_ptr->_aspectRatioMode;
}

/**
 * @brief 获取文本图元
 * @return
 */
QGraphicsSimpleTextItem* DAStandardNodeSvgGraphicsItem::getTextItem() const
{
    return d_ptr->_itemText;
}

void DAStandardNodeSvgGraphicsItem::prepareNodeNameChanged(const QString& name)
{
    if (d_ptr->_itemText) {
        d_ptr->_itemText->setText(name);
    }
}

void DAStandardNodeSvgGraphicsItem::repaintItem()
{
    update();
}

QVariant DAStandardNodeSvgGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (QGraphicsItem::ItemSceneHasChanged == change) {
        if (!(d_ptr->_svgPaintRect.isValid())) {
            if (d_ptr->_defaultSize.isValid()) {
                resetBodySize();
            }
        }
    }
    return (DAAbstractNodeGraphicsItem::itemChange(change, value));
}
