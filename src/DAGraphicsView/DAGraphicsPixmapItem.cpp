#include "DAGraphicsPixmapItem.h"
#include <QPainter>
#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QBuffer>
#include "DAQtEnumTypeStringUtils.h"
namespace DA
{

//===================================================
// DAGraphicsPixmapItem::PrivateData
//===================================================
class DAGraphicsPixmapItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsPixmapItem)
public:
	PrivateData(DAGraphicsPixmapItem* p);
	// 把pixmap转换为base64的字符串
	QString pixmapToString(const QPixmap& pixmap);
	// 字符串转换为pixmap
	QPixmap stringToPixmap(const QString& base64);
	// 判断是否存在透明度
	bool isValidAlpha() const;
	// 获取0~1的透明度
	qreal getOpacity() const;

public:
	QPixmap mPixmap;        ///< 设置尺寸后的图形
	QPixmap mPixmapOrigin;  ///< 保存原始的图形
	int mAlpha { 255 };     ///< 用于叠加透明度
	Qt::TransformationMode mTransformationMode { Qt::FastTransformation };
	Qt::AspectRatioMode mAspectRatioMode { Qt::IgnoreAspectRatio };
};

/**
 * @brief PrivateData 构造函数
 * @param p 指向DAGraphicsPixmapItem的指针
 */
DAGraphicsPixmapItem::PrivateData::PrivateData(DAGraphicsPixmapItem* p) : q_ptr(p)
{
}

/**
 * @brief 把pixmap转换为base64的字符串
 * @param pixmap 要转换的图片
 * @return base64编码的字符串
 */
QString DAGraphicsPixmapItem::PrivateData::pixmapToString(const QPixmap& pixmap)
{
	QBuffer buff;
	pixmap.save(&buff, "PNG");
	QByteArray dataimg;
	// 图像转换为数据
	dataimg.append(buff.data());
	// 图片保存在字符串中
	return dataimg.toBase64();
}

/**
 * @brief 字符串转换为pixmap
 * @param base64 base64编码的字符串
 * @return 转换后的QPixmap
 */
QPixmap DAGraphicsPixmapItem::PrivateData::stringToPixmap(const QString& base64)
{
	QByteArray imgData = QByteArray::fromBase64(base64.toUtf8());
	QPixmap pixmap;
	// 从数据载入图像
	pixmap.loadFromData(imgData);
	return pixmap;
}

/**
 * @brief 判断透明度是否存在
 *
 * @note 255表示没有透明度
 * @return
 */
bool DAGraphicsPixmapItem::PrivateData::isValidAlpha() const
{
    return (mAlpha >= 0) && (mAlpha < 255);
}

/**
 * @brief 获取0~1的透明度
 * @return
 */
qreal DAGraphicsPixmapItem::PrivateData::getOpacity() const
{
    return mAlpha / 255.0;
}

//===================================================
// DAGraphicsPixmapItem
//===================================================

/**
 * @brief 因为要显示调整尺寸的8个点，因此需要调整boundingRect
 * @return
 */
DAGraphicsPixmapItem::DAGraphicsPixmapItem(QGraphicsItem* parent) : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 构造函数，指定pixmap
 * @param pixmap 图片
 * @param parent 父项
 */
DAGraphicsPixmapItem::DAGraphicsPixmapItem(const QPixmap& pixmap, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{

	d_ptr->mPixmap       = pixmap;
	d_ptr->mPixmapOrigin = pixmap;
	changeBodySize(pixmap.size());
}

/**
 * @brief 构造函数，移动语义指定pixmap
 * @param pixmap 图片（右值引用）
 * @param parent 父项
 */
DAGraphicsPixmapItem::DAGraphicsPixmapItem(QPixmap&& pixmap, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
	d_ptr->mPixmap       = std::move(pixmap);
	d_ptr->mPixmapOrigin = d_ptr->mPixmap;
	changeBodySize(d_ptr->mPixmap.size());
}

/**
 * @brief 析构函数
 */
DAGraphicsPixmapItem::~DAGraphicsPixmapItem()
{
}

/**
 * @brief 设置是否可移动
 * @param on
 */
void DAGraphicsPixmapItem::setMoveable(bool on)
{
    setFlag(QGraphicsItem::ItemIsMovable, on);
}

/**
 * @brief 是否可移动
 * @return 可移动返回true，否则返回false
 */
bool DAGraphicsPixmapItem::isMoveable() const
{
    return flags().testFlag(QGraphicsItem::ItemIsMovable);
}

/**
 * @brief 设置是否可选择
 * @param on 是否可选择
 */
void DAGraphicsPixmapItem::setSelectable(bool on)
{
    setFlag(QGraphicsItem::ItemIsSelectable, on);
}

/**
 * @brief 是否可选择
 * @return 可选择返回true，否则返回false
 */
bool DAGraphicsPixmapItem::isSelectable() const
{
    return flags().testFlag(QGraphicsItem::ItemIsSelectable);
}

/**
 * @brief 设置图片
 * @param pixmap 要设置的图片
 */
void DAGraphicsPixmapItem::setPixmap(const QPixmap& pixmap)
{
	// 先赋值给原始图片
	d_ptr->mPixmapOrigin = pixmap;
	// 再设置大小
	setBodySize(pixmap.size());
}

/**
 * @brief 获取图片
 * @return 当前显示的图片
 */
const QPixmap& DAGraphicsPixmapItem::getPixmap() const
{
    return d_ptr->mPixmap;
}

/**
 * @brief 获取原来的尺寸的图片
 *
 * @sa DAGraphicsPixmapItem 会保留原来尺寸的图片，以便能进行缩放
 * @return
 */
const QPixmap& DAGraphicsPixmapItem::getOriginPixmap() const
{
    return d_ptr->mPixmapOrigin;
}

/**
 * @brief DAGraphicsResizeablePixmapItem::setTransformationMode
 * @param t
 */
void DAGraphicsPixmapItem::setTransformationMode(Qt::TransformationMode t)
{
    d_ptr->mTransformationMode = t;
}

/**
 * @brief 获取变换模式
 * @return 变换模式
 */
Qt::TransformationMode DAGraphicsPixmapItem::getTransformationMode() const
{
    return d_ptr->mTransformationMode;
}

/**
 * @brief 设置宽高比模式
 * @param t 宽高比模式
 */
void DAGraphicsPixmapItem::setAspectRatioMode(Qt::AspectRatioMode t)
{
    d_ptr->mAspectRatioMode = t;
}

/**
 * @brief 获取宽高比模式
 * @return 宽高比模式
 */
Qt::AspectRatioMode DAGraphicsPixmapItem::getAspectRatioMode() const
{
    return d_ptr->mAspectRatioMode;
}

/**
 * @brief 判断是否存在有效图片
 * @return
 */
bool DAGraphicsPixmapItem::isHaveValidPixmap() const
{
    return (!d_ptr->mPixmapOrigin.isNull());
}

/**
 * @brief 设置透明度
 * @param a
 */
void DAGraphicsPixmapItem::setAlpha(int a)
{
	d_ptr->mAlpha = a;
	update();
}

/**
 * @brief 获取透明度
 * @return 透明度值（0~255）
 */
int DAGraphicsPixmapItem::getAlpha() const
{
	return d_ptr->mAlpha;
}

/**
 * @brief 设置主体尺寸
 * @param s 要设置的尺寸
 */
void DAGraphicsPixmapItem::setBodySize(const QSizeF& s)
{
	// 设置尺寸
	QSizeF ss      = testBodySize(s);
	d_ptr->mPixmap = d_ptr->mPixmapOrigin.scaled(ss.toSize(), getAspectRatioMode(), getTransformationMode());
	DAGraphicsResizeableItem::setBodySize(d_ptr->mPixmap.size());
}

/**
 * @brief 保存到xml中
 * @note 会把pixmap以base64保存
 * @param doc
 * @param parentElement
 * @return
 */
bool DAGraphicsPixmapItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	if (!DAGraphicsResizeableItem::saveToXml(doc, parentElement, ver)) {
		return false;
	}
	QDomElement pixmapEle = doc->createElement("pixmap-info");
	pixmapEle.setAttribute("aspectRatioMode", enumToString(getAspectRatioMode()));
	pixmapEle.setAttribute("transformationMode", enumToString(getTransformationMode()));
	pixmapEle.setAttribute("alpha", getAlpha());
	QDomElement rawEle   = doc->createElement("raw");
	QString pixmapBase64 = d_ptr->pixmapToString(d_ptr->mPixmapOrigin);
	rawEle.appendChild(doc->createTextNode(pixmapBase64));
	pixmapEle.appendChild(rawEle);  // 原数据
	parentElement->appendChild(pixmapEle);
	return true;
}

/**
 * @brief 从xml加载
 * @param itemElement
 * @return
 */
bool DAGraphicsPixmapItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	// 先加载图片
	QDomElement pixmapInfoEle = itemElement->firstChildElement("pixmap-info");
	if (pixmapInfoEle.isNull()) {
		qDebug() << "DAGraphicsPixmapItem::loadFromXml loss <pixmap-info>";
		return false;
	}

	Qt::TransformationMode tm =
		stringToEnum< Qt::TransformationMode >(pixmapInfoEle.attribute("transformationMode"), Qt::FastTransformation);
	Qt::AspectRatioMode ar =
		stringToEnum< Qt::AspectRatioMode >(pixmapInfoEle.attribute("aspectRatioMode"), Qt::IgnoreAspectRatio);
	bool isok = false;
	int alpha = pixmapInfoEle.attribute("alpha", "255").toInt(&isok);
	if (isok) {
		setAlpha(alpha);
	} else {
		setAlpha(255);
	}
	QDomElement rawEle = pixmapInfoEle.firstChildElement("raw");
	if (rawEle.isNull()) {
		qDebug() << "DAGraphicsPixmapItem::loadFromXml,loss <raw>";
		return false;
	}
	QPixmap pix = d_ptr->stringToPixmap(rawEle.text());
	if (pix.isNull()) {
		qDebug() << "DAGraphicsPixmapItem::loadFromXml,pixmap base64 cannot to pixmap,base64string is:\n"
				 << rawEle.text();
		return false;
	}
	setPixmap(pix);
	setTransformationMode(tm);
	setAspectRatioMode(ar);
	return DAGraphicsResizeableItem::loadFromXml(itemElement, ver);
}

/**
 * @brief 绘制主体内容
 * @param painter 画笔
 * @param option 样式选项
 * @param widget 所属widget
 * @param bodyRect 主体矩形区域
 */
void DAGraphicsPixmapItem::paintBody(QPainter* painter,
                                     const QStyleOptionGraphicsItem* option,
                                     QWidget* widget,
                                     const QRectF& bodyRect)
{
	Q_UNUSED(widget);
	Q_UNUSED(option);
	painter->save();
	if (d_ptr->isValidAlpha()) {
		// 说明要有透明度
		painter->setOpacity(d_ptr->getOpacity());
	}
	painter->setRenderHint(QPainter::SmoothPixmapTransform, (d_ptr->mTransformationMode == Qt::SmoothTransformation));
	painter->drawPixmap(bodyRect.topLeft(), d_ptr->mPixmap);
	painter->restore();
}

}
