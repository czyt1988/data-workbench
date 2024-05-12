#include "DAStandardNodeTextGraphicsItem.h"
#include "DAGraphicsStandardTextItem.h"
namespace DA
{

class DAStandardNodeTextGraphicsItem::PrivateData
{
public:
	DA_DECLARE_PUBLIC(DAStandardNodeTextGraphicsItem)
	PrivateData(DAStandardNodeTextGraphicsItem* p);

public:
	DAGraphicsStandardTextItem* mTextItem { nullptr };
	bool mAutoAdjustSize { false };
};

DAStandardNodeTextGraphicsItem::PrivateData::PrivateData(DAStandardNodeTextGraphicsItem* p)
{
	mTextItem = new DAGraphicsStandardTextItem(p);
}

//===============================================================
// DAStandardNodeTextGraphicsItem
//===============================================================
DAStandardNodeTextGraphicsItem::DAStandardNodeTextGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
	: DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
	setAcceptDrops(true);
	setAcceptHoverEvents(true);
	setFocusProxy(d_ptr->mTextItem);
	setShowBorder(false);
	setEditable(false);
	setSelectable(false);
	setMovable(false);
	setAutoAdjustSize(true);
	d_ptr->mTextItem->setFlag(ItemIsMovable, false);
}

DAStandardNodeTextGraphicsItem::~DAStandardNodeTextGraphicsItem()
{
}

void DAStandardNodeTextGraphicsItem::setEditable(bool on)
{
	d_ptr->mTextItem->setEditable(on);
}

bool DAStandardNodeTextGraphicsItem::isEditable() const
{
	return d_ptr->mTextItem->isEditable();
}

void DAStandardNodeTextGraphicsItem::setAutoAdjustSize(bool on)
{
	d_ptr->mAutoAdjustSize = on;
}

bool DAStandardNodeTextGraphicsItem::isAutoAdjustSize() const
{
	return d_ptr->mAutoAdjustSize;
}

void DAStandardNodeTextGraphicsItem::setText(const QString& v)
{
	d_ptr->mTextItem->setPlainText(v);
	if (isAutoAdjustSize()) {
		// 自动调整大小
		if (d_ptr->mTextItem->textWidth() != -1) {
			d_ptr->mTextItem->setTextWidth(-1);
		}
		d_ptr->mTextItem->adjustSize();
		setBodySize(d_ptr->mTextItem->boundingRect().size());
	}
}

QString DAStandardNodeTextGraphicsItem::getText() const
{
	return d_ptr->mTextItem->toPlainText();
}

void DAStandardNodeTextGraphicsItem::setFont(const QFont& v)
{
	d_ptr->mTextItem->setFont(v);
}

QFont DAStandardNodeTextGraphicsItem::getFont() const
{
	return d_ptr->mTextItem->font();
}

void DAStandardNodeTextGraphicsItem::setBodySize(const QSizeF& s)
{
	DAAbstractNodeGraphicsItem::setBodySize(s);
	if (!isAutoAdjustSize()) {
		d_ptr->mTextItem->setTextWidth(s.width());
		qDebug() << "setTextWidth:" << s.width();
	}
	QRectF textRect = d_ptr->mTextItem->boundingRect();
	QRectF bdRect   = getBodyRect();
	d_ptr->mTextItem->setPos((bdRect.width() - textRect.width()) / 2, (bdRect.height() - textRect.height()) / 2);
	qDebug() << "text pos=" << d_ptr->mTextItem->pos() << ",text bd=" << textRect;
}

DA::DAGraphicsStandardTextItem* DA::DAStandardNodeTextGraphicsItem::textItem() const
{
	return d_ptr->mTextItem;
}

}
