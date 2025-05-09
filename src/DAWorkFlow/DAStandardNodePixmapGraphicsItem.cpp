#include "DAStandardNodePixmapGraphicsItem.h"
#include <QPainter>
#include <QFontMetrics>
#include "DANodePalette.h"
#include "DAAbstractNode.h"
#include <QGraphicsScene>
#include <QApplication>
#include <QGraphicsSimpleTextItem>
#include <QDomDocument>
#include <QDomElement>
#include <QBuffer>
#include <QByteArray>
#include "DAQtEnumTypeStringUtils.h"
namespace DA
{
class DAStandardNodePixmapGraphicsItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAStandardNodePixmapGraphicsItem)
public:
	PrivateData(DAStandardNodePixmapGraphicsItem* p);
	// 获取图的尺寸
	QSize getPixmapSize() const;
	// 获取字体的尺寸
	QSizeF getTextSize() const;
	// 计算推荐尺寸
	QSizeF sizeHint() const;

public:
	QRectF mNodeNameRect { 2, 2, 46, 46 };
	QRectF mPixmapPaintRect { 6, 4, 6, 4 };
	QMarginsF mMargins;
	QPixmap mPixmap;
	QPixmap mPixmapOrigin;
	qreal mWidthHeightRadio { 1 };  ///< 宽高比
	int mSpace { 4 };               ///< 文字和图片的间隔
	Qt::TransformationMode mTransformationMode { Qt::FastTransformation };
	Qt::AspectRatioMode mAspectRatioMode { Qt::KeepAspectRatio };
	QGraphicsSimpleTextItem* mItemText { nullptr };
};

//===================================================
// DAStandardNodePixmapGraphicsItemPrivate
//===================================================

DAStandardNodePixmapGraphicsItem::PrivateData::PrivateData(DAStandardNodePixmapGraphicsItem* p) : q_ptr(p)
{
	mItemText = new QGraphicsSimpleTextItem(p);
}

QSize DAStandardNodePixmapGraphicsItem::PrivateData::getPixmapSize() const
{
	if (mPixmap.isNull()) {
		return QSize(0, 0);
	}
	return mPixmap.size();
}

QSizeF DAStandardNodePixmapGraphicsItem::PrivateData::getTextSize() const
{
	return mItemText->boundingRect().size();
}

QSizeF DAStandardNodePixmapGraphicsItem::PrivateData::sizeHint() const
{
	// 图片尺寸
	QSizeF pixmapSize = getPixmapSize();
	return pixmapSize.grownBy(mMargins);
}

//===================================================
// DAStandardNodePixmapGraphicsItem
//===================================================

DAStandardNodePixmapGraphicsItem::DAStandardNodePixmapGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
	setEnableResize(true);
	setEnableMoveText(true);
	setText(n->getNodeName());
}

DAStandardNodePixmapGraphicsItem::~DAStandardNodePixmapGraphicsItem()
{
}

void DAStandardNodePixmapGraphicsItem::setBodySize(const QSizeF& s)
{
	// 设置尺寸
	QSizeF ss               = testBodySize(s);
	d_ptr->mPixmapPaintRect = QRectF(d_ptr->mMargins.left(),
									 d_ptr->mMargins.top(),
									 ss.width() - d_ptr->mMargins.left() - d_ptr->mMargins.right(),
									 ss.height() - d_ptr->mMargins.top() - d_ptr->mMargins.bottom());
	d_ptr->mPixmap          = d_ptr->mPixmapOrigin.scaled(
		d_ptr->mPixmapPaintRect.size().toSize(), d_ptr->mAspectRatioMode, d_ptr->mTransformationMode);
	// 真正的bodysize是变换后的图片大小
	DAAbstractNodeGraphicsItem::setBodySize(
		QSizeF(d_ptr->mPixmap.width() + d_ptr->mMargins.left() + d_ptr->mMargins.right(),
			   d_ptr->mPixmap.height() + d_ptr->mMargins.top() + d_ptr->mMargins.bottom()));
	// 调整text item的位置
	QSizeF ts = getTextSize();
	ss        = getBodySize();
	//
	d_ptr->mItemText->setPos((ss.width() - ts.width()) / 2, ss.height() + 5);
}

/**
 * @brief 重新计算body的尺寸
 * @return
 */
QSizeF DAStandardNodePixmapGraphicsItem::bodySizeHint() const
{
    return d_ptr->sizeHint();
}

void DAStandardNodePixmapGraphicsItem::paintBody(QPainter* painter,
                                                 const QStyleOptionGraphicsItem* option,
                                                 QWidget* widget,
                                                 const QRectF& bodyRect)
{
	painter->save();
	const DANodePalette& palette = getNodePalette();
	// 绘制边框
	QPen pen(palette.getBorderColor());
	QRectF rec = bodyRect;

	pen.setWidth(1);
	if (isSelected()) {
		pen.setWidth(2);
		pen.setColor(pen.color().darker());
		rec.adjust(1, 1, -1, -1);
	}
	painter->setPen(pen);
	painter->fillRect(rec, palette.getBackgroundBrush());
	painter->drawRect(rec);
	// 绘制图片
	painter->setRenderHint(QPainter::SmoothPixmapTransform, (d_ptr->mTransformationMode == Qt::SmoothTransformation));
	painter->drawPixmap(d_ptr->mPixmapPaintRect.topLeft(), d_ptr->mPixmap);
	painter->restore();
	// 绘制连接点
	paintLinkPoints(painter, option, widget);
}
/**
 * @brief 设置图片
 * @param p
 */
void DAStandardNodePixmapGraphicsItem::setPixmap(const QPixmap& p)
{
	d_ptr->mPixmap = p;
	// 保存是为了能无损放大缩小
	d_ptr->mPixmapOrigin = p;
	// 记录宽高比
	d_ptr->mWidthHeightRadio = qreal(p.width()) / qreal(p.height());
	// 最大尺寸就是图片尺寸
	setBodyMaximumSize(p.size());
}

const QPixmap& DAStandardNodePixmapGraphicsItem::getPixmap() const
{
    return d_ptr->mPixmap;
}

/**
 * @brief 设置图片的尺寸
 * @param s
 */
void DAStandardNodePixmapGraphicsItem::setPixmapSize(const QSize& s)
{
	d_ptr->mPixmap = d_ptr->mPixmap.scaled(s);
	setBodySize(bodySizeHint());
}

/**
 * @brief 获取图片尺寸
 * @return
 */
QSize DAStandardNodePixmapGraphicsItem::getPixmapSize() const
{
    return d_ptr->getPixmapSize();
}

/**
 * @brief 获取字体的尺寸
 * @return
 */
QSizeF DAStandardNodePixmapGraphicsItem::getTextSize() const
{
    return d_ptr->getTextSize();
}

/**
 * @brief 设置文本
 * @param t
 */
void DAStandardNodePixmapGraphicsItem::setText(const QString& t)
{
    d_ptr->mItemText->setText(t);
}

/**
 * @brief 获取文本
 * @return
 */
QString DAStandardNodePixmapGraphicsItem::getText() const
{
    return d_ptr->mItemText->text();
}

/**
 * @brief 设置文本可以移动
 * @param on
 */
void DAStandardNodePixmapGraphicsItem::setEnableMoveText(bool on)
{
    d_ptr->mItemText->setFlag(ItemIsMovable, on);
}

/**
 * @brief 判断文本是否可移动
 * @return
 */
bool DAStandardNodePixmapGraphicsItem::isEnableMoveText() const
{
    return d_ptr->mItemText->flags().testFlag(ItemIsMovable);
}

/**
 * @brief 设置图片变换时的比例
 * @param m
 */
void DAStandardNodePixmapGraphicsItem::setAspectRatioMode(Qt::AspectRatioMode m)
{
    d_ptr->mAspectRatioMode = m;
}
/**
 * @brief 获取图片的AspectRatioMode
 * @return
 */
Qt::AspectRatioMode DAStandardNodePixmapGraphicsItem::getAspectRatioMode() const
{
    return d_ptr->mAspectRatioMode;
}
/**
 * @brief 设置图片缩放时TransformationMode
 * @param m
 */
void DAStandardNodePixmapGraphicsItem::setTransformationMode(Qt::TransformationMode m)
{
    d_ptr->mTransformationMode = m;
}
/**
 * @brief 获取图片缩放时TransformationMode
 * @return
 */
Qt::TransformationMode DAStandardNodePixmapGraphicsItem::getTransformationMode() const
{
    return d_ptr->mTransformationMode;
}

bool DAStandardNodePixmapGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	DAAbstractNodeGraphicsItem::saveToXml(doc, parentElement, ver);
	QDomElement pixmapEle = doc->createElement("pixmap-info");
	QSize sz              = getPixmapSize();

	pixmapEle.setAttribute("width", sz.width());
	pixmapEle.setAttribute("height", sz.height());
	pixmapEle.setAttribute("aspectRatioMode", enumToString(getAspectRatioMode()));
	pixmapEle.setAttribute("transformationMode", enumToString(getTransformationMode()));

	QDomElement rawEle = doc->createElement("raw");
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	d_ptr->mPixmapOrigin.save(&buffer, "PNG");
	rawEle.appendChild(doc->createTextNode(bytes.toBase64()));

	QDomElement textEle = doc->createElement("text");
	textEle.setAttribute("move-text", isEnableMoveText());
	textEle.appendChild(doc->createTextNode(getText()));

	pixmapEle.appendChild(rawEle);  // 原数据
	pixmapEle.appendChild(textEle);
	parentElement->appendChild(pixmapEle);
	return true;
}

bool DAStandardNodePixmapGraphicsItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	if (!DAAbstractNodeGraphicsItem::loadFromXml(itemElement, ver)) {
		return false;
	}
	QDomElement pixmapEle = itemElement->firstChildElement("pixmap-info");
	if (pixmapEle.isNull()) {
		return false;
	}
	QSize s;
	if (getStringIntValue(pixmapEle.attribute("width"), s.rwidth())
		&& getStringIntValue(pixmapEle.attribute("height"), s.rheight())) {
		setPixmapSize(s);
	}
	setAspectRatioMode(stringToEnum< Qt::AspectRatioMode >(pixmapEle.attribute("aspectRatioMode"), Qt::KeepAspectRatio));
	setTransformationMode(stringToEnum(pixmapEle.attribute("transformationMode"), Qt::FastTransformation));

	QDomElement rawEle = pixmapEle.firstChildElement("raw");
	if (!rawEle.isNull()) {
		QString imgBase64  = rawEle.text();
		QByteArray imgData = QByteArray::fromBase64(imgBase64.toUtf8());
		// 从数据载入图像
		QPixmap pixmap;
		if (!pixmap.loadFromData(imgData, "PNG")) {
			return false;
		}
		setPixmap(pixmap);
	}

	QDomElement textEle = pixmapEle.firstChildElement("text");
	if (!textEle.isNull()) {
		setEnableMoveText(getStringBoolValue(textEle.attribute("move-text")));
		setText(textEle.text());
	}
	return true;
}

QVariant DAStandardNodePixmapGraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
	if (QGraphicsItem::ItemSceneHasChanged == change) {
		setBodySize(bodySizeHint());
	}
	return (DAAbstractNodeGraphicsItem::itemChange(change, value));
}

void DAStandardNodePixmapGraphicsItem::prepareNodeNameChanged(const QString& name)
{
	if (d_ptr->mItemText) {
		d_ptr->mItemText->setText(name);
	}
}
}  // end of namespace DA
