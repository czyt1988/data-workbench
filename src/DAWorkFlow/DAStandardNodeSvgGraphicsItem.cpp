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
class DAStandardNodeSvgGraphicsItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAStandardNodeSvgGraphicsItem)
public:
	PrivateData(DAStandardNodeSvgGraphicsItem* p);
	PrivateData(DAStandardNodeSvgGraphicsItem* p, QSvgRenderer* sharedrenderer);
	void updateDefaultSize();
	QSizeF getTextSize() const;
	void updateSvgPaintRect();
	// 按照AspectRatioMode得到的尺寸
	QSizeF getAspectRatioSize(const QSizeF& s) const;

public:
	bool mShared { false };
	QSvgRenderer* mRenderer { nullptr };
	QSize mDefaultSize;
	QString mElemId;
	QRectF mNodeNameRect;
	QRectF mSvgPaintRect;
	QMarginsF mMargins;
	QGraphicsSimpleTextItem* mItemText { nullptr };
	Qt::AspectRatioMode mAspectRatioMode { Qt::KeepAspectRatio };
};

//===================================================
// DAStandardNodeSvgGraphicsItemPrivate
//===================================================

DAStandardNodeSvgGraphicsItem::PrivateData::PrivateData(DAStandardNodeSvgGraphicsItem* p) : q_ptr(p)
{
	mItemText = new QGraphicsSimpleTextItem(p);
	mRenderer = new QSvgRenderer(p);
	QObject::connect(mRenderer, &QSvgRenderer::repaintNeeded, p, &DAStandardNodeSvgGraphicsItem::repaintItem);
	p->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

DAStandardNodeSvgGraphicsItem::PrivateData::PrivateData(DAStandardNodeSvgGraphicsItem* p, QSvgRenderer* sharedrenderer)
    : q_ptr(p)
{
	mItemText = new QGraphicsSimpleTextItem(p);
	p->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	mShared   = true;
	mRenderer = sharedrenderer;
}
/**
 * @brief _defaultSize可以记录最初的比例
 */
void DAStandardNodeSvgGraphicsItem::PrivateData::updateDefaultSize()
{
	if (mElemId.isEmpty()) {
		mDefaultSize = mRenderer->defaultSize();
	} else {
		mDefaultSize = mRenderer->boundsOnElement(mElemId).size().toSize();
	}
}

QSizeF DAStandardNodeSvgGraphicsItem::PrivateData::getTextSize() const
{
	return mItemText->boundingRect().size();
}

void DAStandardNodeSvgGraphicsItem::PrivateData::updateSvgPaintRect()
{
	QRectF r      = q_ptr->getBodyRect();
	mSvgPaintRect = r.adjusted(mMargins.left(), mMargins.top(), -mMargins.right(), -mMargins.bottom());
}

QSizeF DAStandardNodeSvgGraphicsItem::PrivateData::getAspectRatioSize(const QSizeF& s) const
{
	QSizeF newSize = mDefaultSize;
	newSize.scale(s, mAspectRatioMode);
	return newSize;
}

//==============================================================
// DAStandardNodeSvgGraphicsItem
//==============================================================

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
	setEnableResize(true);
	setEnableMoveText(true);
	setText(n->getNodeName());
}

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, const QString& svgfile, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
	setEnableResize(true);
	setEnableMoveText(true);
	setSvg(svgfile);
	d_ptr->updateSvgPaintRect();
	setText(n->getNodeName());
}

DAStandardNodeSvgGraphicsItem::DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QSvgRenderer* sharedrender, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p)
    , d_ptr(std::make_unique< DAStandardNodeSvgGraphicsItem::PrivateData >(this, sharedrender))
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
	// 设置尺寸
	QSizeF ss = testBodySize(s);
	ss        = d_ptr->getAspectRatioSize(ss);
	// 计算比例

	DAAbstractNodeGraphicsItem::setBodySize(ss);
	// 刷新文字位置
	QSizeF ts = getTextSize();
	ss        = getBodySize();
	d_ptr->updateSvgPaintRect();
	d_ptr->mItemText->setPos((ss.width() - ts.width()) / 2, ss.height() + 5);
}

void DAStandardNodeSvgGraphicsItem::setEnableMoveText(bool on)
{
	d_ptr->mItemText->setFlag(ItemIsMovable, on);
}

bool DAStandardNodeSvgGraphicsItem::isEnableMoveText() const
{
    return d_ptr->mItemText->flags().testFlag(ItemIsMovable);
}

/**
 * @brief 获取svg渲染器
 * @return
 */
QSvgRenderer* DAStandardNodeSvgGraphicsItem::renderer() const
{
    return d_ptr->mRenderer;
}

void DAStandardNodeSvgGraphicsItem::setSharedRenderer(QSvgRenderer* renderer)
{
	if (!d_ptr->mShared)
		delete d_ptr->mRenderer;

	d_ptr->mRenderer = renderer;
	d_ptr->mShared   = true;

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
void DAStandardNodeSvgGraphicsItem::paintBody(QPainter* painter,
                                              const QStyleOptionGraphicsItem* option,
                                              QWidget* widget,
                                              const QRectF& bodyRect)
{
	Q_UNUSED(widget);
	Q_UNUSED(bodyRect);
	if (!d_ptr->mRenderer->isValid())
		return;

	if (d_ptr->mElemId.isEmpty()) {
		d_ptr->mRenderer->render(painter, d_ptr->mSvgPaintRect);
		qDebug() << "mSvgPaintRect=" << d_ptr->mSvgPaintRect;
	} else {
		d_ptr->mRenderer->render(painter, d_ptr->mElemId, d_ptr->mSvgPaintRect);
	}
	// 绘制连接点
	paintLinkPoints(painter, option, widget);
}

/**
 * @brief 等价 QGraphicsSvgItem::setElementId
 * @param id
 */
void DAStandardNodeSvgGraphicsItem::setElementId(const QString& id)
{
	d_ptr->mElemId = id;
	d_ptr->updateDefaultSize();
	update();
}

QString DAStandardNodeSvgGraphicsItem::elementId() const
{
	return d_ptr->mElemId;
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
	d_ptr->mItemText->setText(t);
	updateTextItemPos();
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
    setBodySize(d_ptr->mDefaultSize);
}
/**
 * @brief 设置图片变换时的比例
 * @param m
 */
void DAStandardNodeSvgGraphicsItem::setAspectRatioMode(Qt::AspectRatioMode m)
{
	d_ptr->mAspectRatioMode = m;
	setBodySize(getBodySize());
}
/**
 * @brief 获取图片变换时的比例
 * @return
 */
Qt::AspectRatioMode DAStandardNodeSvgGraphicsItem::getAspectRatioMode() const
{
    return d_ptr->mAspectRatioMode;
}

/**
 * @brief 获取文本图元
 * @return
 */
QGraphicsSimpleTextItem* DAStandardNodeSvgGraphicsItem::getTextItem() const
{
    return d_ptr->mItemText;
}

/**
 * @brief 更新text item的位置，使得文本的位置一直处于中间
 */
void DAStandardNodeSvgGraphicsItem::updateTextItemPos()
{
	if (!(d_ptr->mItemText)) {
		return;
	}
	QRectF tc = d_ptr->mItemText->boundingRect();
	QRectF bc = getBodyRect();
	qreal x   = (bc.width() - tc.width()) / 2;
	qreal y   = bc.bottom() + 5;
	d_ptr->mItemText->setPos(x, y);
}

void DAStandardNodeSvgGraphicsItem::prepareNodeNameChanged(const QString& name)
{
	if (d_ptr->mItemText) {
		d_ptr->mItemText->setText(name);
	}
}

void DAStandardNodeSvgGraphicsItem::repaintItem()
{
	update();
}

QVariant DAStandardNodeSvgGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if (QGraphicsItem::ItemSceneHasChanged == change) {
		if (!(d_ptr->mSvgPaintRect.isValid())) {
			if (d_ptr->mDefaultSize.isValid()) {
				resetBodySize();
			}
		}
	}
	return (DAAbstractNodeGraphicsItem::itemChange(change, value));
}
}  // end of namespace DA
