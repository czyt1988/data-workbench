#include "DAStandardNodeInputOutputGraphicsItem.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QDebug>
#include <QApplication>
#include "DAStandardNodeLinkPointDrawDelegate.h"
#include "DANodePalette.h"
#include "DAStandardNodeInputOutput.h"
namespace DA
{

/**
 * @brief 两个输入输出节点的间隔
 */
const int c_arg_line_space = 35;
/**
 * @brief item的默认宽度
 */
const int c_stadardnodeitem_default_width_space = 35;
DAStandardNodeInputOutputGraphicsItem::DAStandardNodeInputOutputGraphicsItem(DA::DAAbstractNode* n, QGraphicsItem* p)
	: DAAbstractNodeGraphicsItem(n, p)
{
	// 禁用缩放
	setEnableResize(false);
	setBodySize(QSizeF(50, 50));
	setBodyMinimumSize(QSize(20, 20));
	if (!acceptHoverEvents()) {
		setAcceptHoverEvents(true);
	}
	// 连接点绘制代理
	setLinkPointDrawDelegate(new DAStandardNodeLinkPointDrawDelegate(this));
	setBodySize(calcSize());  // calcSize要在StandardNodeLinkPointDrawDelegate之后调用
	if (auto bn = dynamic_cast< DAStandardNodeInputOutput* >(n)) {
		mDisplayName = bn->getDisplayName();
	}
}

DAStandardNodeInputOutputGraphicsItem::~DAStandardNodeInputOutputGraphicsItem()
{
}

/**
 * @brief 计算尺寸，根据输入输出进行尺寸计算
 * @return
 */
QSize DAStandardNodeInputOutputGraphicsItem::calcSize() const
{
	auto n = node();
	if (!n) {
		qDebug() << "null node";
		return QSize(20, 20);
	}
	int inCnt  = n->getInputKeysConut();
	int outCnt = n->getOutputKeysConut();
	QFontMetrics fm((QFont()));
	// 找到最长的输入
	const auto inpoints  = n->getInputKeys();
	const auto outpoints = n->getOutputKeys();
	QString maxInText, maxOutText;
	for (const auto& p : inpoints) {
		if (p.size() > maxInText.size()) {
			maxInText = p;
		}
	}
	for (const auto& p : outpoints) {
		if (p.size() > maxOutText.size()) {
			maxOutText = p;
		}
	}
	int minW = fm.horizontalAdvance(maxInText + maxOutText) + getLinkpointSize().width() * 2
			   + c_stadardnodeitem_default_width_space;
	qDebug() << "maxInText=" << maxInText << ",maxOutText=" << maxOutText << ",minW=" << minW;
	// 根据数量，计算高度
	int h = qMax((inCnt + 1) * c_arg_line_space, (outCnt + 1) * c_arg_line_space);
	// qDebug() << "calcSize=" << QSize(c_item_default_width, h) << ",inCnt=" << inCnt << ",outCnt=" << outCnt;
	QSize res(minW, h);
	if (mEnableShowIcon) {
		// 有图标要把图标位置留下
		res.setWidth(res.width() + mIconSize.width());
		res = res.expandedTo(mIconSize);
	}
	return res;
}

/**
 * @brief StandardInputOutputNodeGraphicsItem::paintBody
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAStandardNodeInputOutputGraphicsItem::paintBody(QPainter* painter,
                                                      const QStyleOptionGraphicsItem* option,
                                                      QWidget* widget,
                                                      const QRectF& bodyRect)
{
	painter->setPen(getBorderPen());
	painter->setBrush(getBackgroundBrush());
	painter->drawRect(bodyRect);
	//!绘制图标
	if (mEnableShowIcon) {
		paintIcon(painter, option, widget, bodyRect, mIconPixmap);
	}
	//! 绘制连接点
	paintLinkPoints(painter, option, widget);
	//! 绘制显示名字
	paintDisplayName(painter, option, widget, mDisplayName);
	//!
}

/**
 * @brief 获取连接点尺寸
 * @return
 */
QSize DAStandardNodeInputOutputGraphicsItem::getLinkpointSize() const
{
	DAStandardNodeLinkPointDrawDelegate* delegate = dynamic_cast< DAStandardNodeLinkPointDrawDelegate* >(
		getLinkPointDrawDelegate());
	if (!delegate) {
		return QSize();
	}
	return delegate->getLinkPointSize();
}

/**
 * @brief 节点名字改变的回调
 * @param name
 */
void DAStandardNodeInputOutputGraphicsItem::nodeDisplayNameChanged(const QString& name)
{
    mDisplayName = name;
}

void DAStandardNodeInputOutputGraphicsItem::setEnableMultLink(bool on)
{
	DAStandardNodeLinkPointDrawDelegate* delegate = dynamic_cast< DAStandardNodeLinkPointDrawDelegate* >(
		getLinkPointDrawDelegate());
	if (!delegate) {
		return;
	}
	return delegate->setEnableMultLink(on);
}

bool DAStandardNodeInputOutputGraphicsItem::isEnableMultLink() const
{
	DAStandardNodeLinkPointDrawDelegate* delegate = dynamic_cast< DAStandardNodeLinkPointDrawDelegate* >(
		getLinkPointDrawDelegate());
	if (!delegate) {
		return false;
	}
	return delegate->isEnableMultLink();
}

/**
 * @brief 设置在中间显示icon，icon以node的icon来显示
 * @param on
 */
void DAStandardNodeInputOutputGraphicsItem::setEnableShowIcon(bool on)
{
	mEnableShowIcon = on;
	mIconPixmap     = generateIcon();
	setBodySize(calcSize());
	update();
}

/**
 * @brief 是否显示icon
 * @return
 */
bool DAStandardNodeInputOutputGraphicsItem::isEnableShowIcon() const
{
    return mEnableShowIcon;
}

/**
 * @brief 设置icon的尺寸
 * @param s
 */
void DAStandardNodeInputOutputGraphicsItem::setIconSize(const QSize& s)
{
	mIconSize   = s;
	mIconPixmap = generateIcon();
	setBodySize(calcSize());
}

/**
 * @brief icon的尺寸
 * @return
 */
QSize DAStandardNodeInputOutputGraphicsItem::getIconSize() const
{
    return mIconSize;
}

void DAStandardNodeInputOutputGraphicsItem::paintDisplayName(QPainter* painter,
                                                             const QStyleOptionGraphicsItem* option,
                                                             QWidget* widget,
                                                             const QString& name)
{
	painter->save();
	const DA::DANodePalette& palette = getNodePalette();
	painter->setPen(QPen(palette.getTextColor()));
	auto fm        = painter->fontMetrics();
	QRect bd       = fm.boundingRect(name);
	QRect bodyRect = getBodyRect().toRect();
	int offset     = (bodyRect.width() - bd.width()) / 2;
	bd.moveTopLeft(QPoint(bodyRect.x() + offset, bodyRect.bottom() + 4));
	painter->drawText(bd, Qt::AlignCenter, name);
	painter->restore();
}

void DAStandardNodeInputOutputGraphicsItem::paintIcon(QPainter* painter,
                                                      const QStyleOptionGraphicsItem* option,
                                                      QWidget* widget,
                                                      const QRectF& bodyRect,
                                                      const QPixmap& pixmap)
{
	QRect pixmapRect = pixmap.rect();
	// 把pixmapRect中心移动到bodyRect的中心
	pixmapRect.moveTopLeft(
		QPoint((bodyRect.width() - pixmapRect.width()) / 2, (bodyRect.height() - pixmapRect.height()) / 2));
	painter->drawPixmap(pixmapRect, pixmap);
}

QPixmap DAStandardNodeInputOutputGraphicsItem::generateIcon()
{
	return rawNode()->getIcon().pixmap(mIconSize);
}
}
