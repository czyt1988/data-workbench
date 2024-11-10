#include "DAAbstractWidgetOverlay.h"
#include <QPainter>
#include <QPaintEngine>
#include <QPainterPath>
#include <QImage>
#include <QEvent>
#include <QPaintEvent>
namespace DA
{

class DAAbstractWidgetOverlay::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractWidgetOverlay)
public:
	PrivateData(DAAbstractWidgetOverlay* p);
	static bool isX11GraphicsSystem();
	static QImage::Format maskImageFormat();
	static QRegion alphaMask(const QImage& image, const QRegion& region);
	void resetRgbaBuffer();
	MaskMode maskMode { DAAbstractWidgetOverlay::MaskHint };
	RenderMode renderMode { DAAbstractWidgetOverlay::AutoRenderMode };
	uchar* rgbaBuffer { nullptr };
};

DAAbstractWidgetOverlay::PrivateData::PrivateData(DAAbstractWidgetOverlay* p) : q_ptr(p)
{
}

bool DAAbstractWidgetOverlay::PrivateData::isX11GraphicsSystem()
{
	static int onX11 = -1;
	if (onX11 < 0) {
		QPixmap pm(1, 1);
		QPainter painter(&pm);

		onX11 = (painter.paintEngine()->type() == QPaintEngine::X11) ? 1 : 0;
	}

	return onX11 == 1;
}

QImage::Format DAAbstractWidgetOverlay::PrivateData::maskImageFormat()
{
	if (isX11GraphicsSystem())
		return QImage::Format_ARGB32;

	return QImage::Format_ARGB32_Premultiplied;
}

QRegion DAAbstractWidgetOverlay::PrivateData::alphaMask(const QImage& image, const QRegion& region)
{
	const int w = image.width();
	const int h = image.height();

	QRegion mask;
	QRect rect;

#if QT_VERSION >= 0x050800
	for (QRegion::const_iterator it = region.cbegin(); it != region.cend(); ++it) {
		const QRect& r = *it;
#else
	const QVector< QRect > rects = region.rects();
	for (int i = 0; i < rects.size(); i++) {
		const QRect& r = rects[ i ];
#endif
		int x1, x2, y1, y2;
		r.getCoords(&x1, &y1, &x2, &y2);

		x1 = qMax(x1, 0);
		x2 = qMin(x2, w - 1);
		y1 = qMax(y1, 0);
		y2 = qMin(y2, h - 1);

		for (int y = y1; y <= y2; ++y) {
			bool inRect = false;
			int rx0     = -1;

			const uint* line = reinterpret_cast< const uint* >(image.scanLine(y)) + x1;
			for (int x = x1; x <= x2; x++) {
				const bool on = ((*line++ >> 24) != 0);
				if (on != inRect) {
					if (inRect) {
						rect.setCoords(rx0, y, x - 1, y);
						mask += rect;
					} else {
						rx0 = x;
					}

					inRect = on;
				}
			}

			if (inRect) {
				rect.setCoords(rx0, y, x2, y);
				mask = mask.united(rect);
			}
		}
	}

	return mask;
}

void DAAbstractWidgetOverlay::PrivateData::resetRgbaBuffer()
{
	if (rgbaBuffer) {
		std::free(rgbaBuffer);
		rgbaBuffer = NULL;
	}
}

//===============================================================
// DAWidgetOverlay
//===============================================================

DAAbstractWidgetOverlay::DAAbstractWidgetOverlay(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_NoSystemBackground);
	setFocusPolicy(Qt::NoFocus);

	if (parent) {
		resize(parent->size());
		parent->installEventFilter(this);
	}
}

DAAbstractWidgetOverlay::~DAAbstractWidgetOverlay()
{
}

void DAAbstractWidgetOverlay::setMaskMode(MaskMode mode)
{
	if (mode != d_ptr->maskMode) {
		d_ptr->maskMode = mode;
		d_ptr->resetRgbaBuffer();
	}
}

DAAbstractWidgetOverlay::MaskMode DAAbstractWidgetOverlay::getMaskMode() const
{
	return d_ptr->maskMode;
}

void DAAbstractWidgetOverlay::setRenderMode(RenderMode mode)
{
	d_ptr->renderMode = mode;
}

DAAbstractWidgetOverlay::RenderMode DAAbstractWidgetOverlay::getRenderMode() const
{
	return d_ptr->renderMode;
}

bool DAAbstractWidgetOverlay::eventFilter(QObject* object, QEvent* event)
{
	if (object == parent() && event->type() == QEvent::Resize) {
		QResizeEvent* resizeEvent = static_cast< QResizeEvent* >(event);
		resize(resizeEvent->size());
	}

	return QObject::eventFilter(object, event);
}

QRegion DAAbstractWidgetOverlay::maskRegion(const QRect& r, int penWidth)
{
	const int pw  = qMax(penWidth, 1);
	const int pw2 = penWidth / 2;

	int x1 = r.left() - pw2;
	int x2 = r.right() + 1 + pw2 + (pw % 2);

	int y1 = r.top() - pw2;
	int y2 = r.bottom() + 1 + pw2 + (pw % 2);

	QRegion region;

	region += QRect(x1, y1, x2 - x1, pw);
	region += QRect(x1, y1, pw, y2 - y1);
	region += QRect(x1, y2 - pw, x2 - x1, pw);
	region += QRect(x2 - pw, y1, pw, y2 - y1);

	return region;
}

QRegion DAAbstractWidgetOverlay::maskRegionVOrHLine(const QLine& VOrHLine, int penWidth)
{
	const int pw  = qMax(penWidth, 1);
	const int pw2 = penWidth / 2;

	QRegion region;

	if (VOrHLine.x1() == VOrHLine.x2()) {
		region += QRect(VOrHLine.x1() - pw2, VOrHLine.y1(), pw, VOrHLine.y2()).normalized();
	} else if (VOrHLine.y1() == VOrHLine.y2()) {
		region += QRect(VOrHLine.x1(), VOrHLine.y1() - pw2, VOrHLine.x2(), pw).normalized();
	}

	return region;
}

void DAAbstractWidgetOverlay::paintEvent(QPaintEvent* event)
{
	const QRegion& clipRegion = event->region();

	QPainter painter(this);

	bool useRgbaBuffer = false;
	if (d_ptr->renderMode == DAAbstractWidgetOverlay::CopyAlphaMask) {
		useRgbaBuffer = true;
	} else if (d_ptr->renderMode == DAAbstractWidgetOverlay::AutoRenderMode) {
		if (painter.paintEngine()->type() == QPaintEngine::Raster)
			useRgbaBuffer = true;
	}

	if (d_ptr->rgbaBuffer && useRgbaBuffer) {
		const QImage image(d_ptr->rgbaBuffer, width(), height(), DAAbstractWidgetOverlay::PrivateData::maskImageFormat());

		const int rectCount = clipRegion.rectCount();

		if (rectCount > 2000) {
			// the region is to complex
			painter.setClipRegion(clipRegion);

			const QRect r = clipRegion.boundingRect();
			painter.drawImage(r.topLeft(), image, r);
		} else {
#if QT_VERSION >= 0x050800
			for (QRegion::const_iterator it = clipRegion.cbegin(); it != clipRegion.cend(); ++it) {
				const QRect& r = *it;
				painter.drawImage(r.topLeft(), image, r);
			}
#else
			const QVector< QRect > rects = clipRegion.rects();
			for (int i = 0; i < rects.size(); i++) {
				const QRect& r = rects[ i ];
				painter.drawImage(r.topLeft(), image, r);
			}
#endif
		}
	} else {
		painter.setClipRegion(clipRegion);
		draw(&painter);
	}
}

void DAAbstractWidgetOverlay::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);

	d_ptr->resetRgbaBuffer();
}

QRegion DAAbstractWidgetOverlay::maskHint() const
{
	return QRegion();
}

void DAAbstractWidgetOverlay::updateOverlay()
{
	updateMask();
	update();
}

void DAAbstractWidgetOverlay::updateMask()
{
	d_ptr->resetRgbaBuffer();

	QRegion mask;

	if (d_ptr->maskMode == DAAbstractWidgetOverlay::MaskHint) {
		mask = maskHint();
	} else if (d_ptr->maskMode == DAAbstractWidgetOverlay::AlphaMask) {
		// TODO: the image doesn't need to be larger than
		//       the bounding rectangle of the hint !!

		QRegion hint = maskHint();
		if (hint.isEmpty())
			hint += QRect(0, 0, width(), height());

		// A fresh buffer from calloc() is usually faster
		// than reinitializing an existing one with
		// QImage::fill( 0 ) or memset()

		d_ptr->rgbaBuffer = (uchar*)::calloc(width() * height(), 4);

		QImage image(d_ptr->rgbaBuffer, width(), height(), DAAbstractWidgetOverlay::PrivateData::maskImageFormat());

		QPainter painter(&image);
		draw(&painter);
		painter.end();

		mask = DAAbstractWidgetOverlay::PrivateData::alphaMask(image, hint);

		if (d_ptr->renderMode == DAAbstractWidgetOverlay::DrawOverlay) {
			// we don't need the buffer later
			d_ptr->resetRgbaBuffer();
		}
	}

	// A bug in Qt initiates a full repaint of the widget
	// when we change the mask, while we are visible !

	setVisible(false);

	if (mask.isEmpty())
		clearMask();
	else
		setMask(mask);

	setVisible(true);
}

void DAAbstractWidgetOverlay::draw(QPainter* painter) const
{
	if (QWidget* widget = parentWidget()) {
		painter->setClipRect(widget->contentsRect());
	}
	drawOverlay(painter);
}

}  // end ns da
