#include "DAChartScrollZoomer.h"
#include <QEvent>
#include <QResizeEvent>
// qwt
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
namespace DA
{

//===================================================
// _DAChartScrollZoomerScrollData
//===================================================
class _DAChartScrollZoomerScrollData
{
public:
    _DAChartScrollZoomerScrollData()
        : scrollBar(NULL), position(DAChartScrollZoomer::OppositeToScale), mode(Qt::ScrollBarAsNeeded)
    {
    }

    ~_DAChartScrollZoomerScrollData()
    {
        delete scrollBar;
    }

    DAChartScrollBar* scrollBar;
    DAChartScrollZoomer::ScrollBarPosition position;
    Qt::ScrollBarPolicy mode;
};

//===================================================
// name
//===================================================
class DAChartScrollZoomer::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartScrollZoomer)
public:
    PrivateData(DAChartScrollZoomer* p);

public:
    bool mInZoom { false };
    bool mIsEnable { true };  ///< 标定是否显示滚动条
    bool mAlignCanvasToScales[ QwtPlot::axisCnt ];
    QWidget* mCornerWidget { nullptr };
    _DAChartScrollZoomerScrollData* mHScrollData { nullptr };
    _DAChartScrollZoomerScrollData* mVScrollData { nullptr };
};

DAChartScrollZoomer::PrivateData::PrivateData(DAChartScrollZoomer* p) : q_ptr(p)
{
}
//===================================================
// DAChartScrollZoomer
//===================================================

DAChartScrollZoomer::DAChartScrollZoomer(int xAxis, int yAxis, QWidget* canvas)
    : QwtPlotZoomer(xAxis, yAxis, canvas), DA_PIMPL_CONSTRUCT

{
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        d_ptr->mAlignCanvasToScales[ axis ] = false;
    }

    if (!canvas) {
        return;
    }

    d_ptr->mHScrollData = new _DAChartScrollZoomerScrollData;
    d_ptr->mVScrollData = new _DAChartScrollZoomerScrollData;
}

DAChartScrollZoomer::DAChartScrollZoomer(QWidget* canvas) : QwtPlotZoomer(canvas), d_ptr(new PrivateData(this))
{
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        d_ptr->mAlignCanvasToScales[ axis ] = false;
    }

    if (!canvas) {
        return;
    }

    d_ptr->mHScrollData = new _DAChartScrollZoomerScrollData;
    d_ptr->mVScrollData = new _DAChartScrollZoomerScrollData;
}

DAChartScrollZoomer::~DAChartScrollZoomer()
{
    delete d_ptr->mCornerWidget;
    delete d_ptr->mVScrollData;
    delete d_ptr->mHScrollData;
}

void DAChartScrollZoomer::rescale()
{
    QwtScaleWidget* xScale = plot()->axisWidget(xAxis());
    QwtScaleWidget* yScale = plot()->axisWidget(yAxis());

    if (zoomRectIndex() <= 0) {
        if (d_ptr->mInZoom) {
            xScale->setMinBorderDist(0, 0);
            yScale->setMinBorderDist(0, 0);

            QwtPlotLayout* layout = plot()->plotLayout();

            for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
                layout->setAlignCanvasToScale(axis, d_ptr->mAlignCanvasToScales);
            }

            d_ptr->mInZoom = false;
        }
    } else {
        if (!d_ptr->mInZoom) {
            /*
             * We set a minimum border distance.
             * Otherwise the canvas size changes when scrolling,
             * between situations where the major ticks are at
             * the canvas borders (requiring extra space for the label)
             * and situations where all labels can be painted below/top
             * or left/right of the canvas.
             */
            int start, end;

            xScale->getBorderDistHint(start, end);
            xScale->setMinBorderDist(start, end);

            yScale->getBorderDistHint(start, end);
            yScale->setMinBorderDist(start, end);

            QwtPlotLayout* layout = plot()->plotLayout();
            for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
                d_ptr->mAlignCanvasToScales[ axis ] = layout->alignCanvasToScale(axis);
            }

            layout->setAlignCanvasToScales(false);

            d_ptr->mInZoom = true;
        }
    }

    QwtPlotZoomer::rescale();
    updateScrollBars();
}

bool DAChartScrollZoomer::isEnableScrollBar() const
{
    return (d_ptr->mIsEnable);
}

void DAChartScrollZoomer::on_enable_scrollBar(bool enable)
{
    d_ptr->mIsEnable = enable;
    updateScrollBars();
}

DAChartScrollBar* DAChartScrollZoomer::scrollBar(Qt::Orientation orientation)
{
    DAChartScrollBar*& sb = (orientation == Qt::Vertical) ? d_ptr->mVScrollData->scrollBar : d_ptr->mHScrollData->scrollBar;

    if (sb == NULL) {
        sb = new DAChartScrollBar(orientation, canvas());
        sb->hide();
        connect(sb, SIGNAL(valueChanged(Qt::Orientation, double, double)), SLOT(scrollBarMoved(Qt::Orientation, double, double)));
    }
    return (sb);
}

DAChartScrollBar* DAChartScrollZoomer::horizontalScrollBar() const
{
    return (d_ptr->mHScrollData->scrollBar);
}

DAChartScrollBar* DAChartScrollZoomer::verticalScrollBar() const
{
    return (d_ptr->mVScrollData->scrollBar);
}

void DAChartScrollZoomer::setHScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if (hScrollBarMode() != mode) {
        d_ptr->mHScrollData->mode = mode;
        updateScrollBars();
    }
}

void DAChartScrollZoomer::setVScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if (vScrollBarMode() != mode) {
        d_ptr->mVScrollData->mode = mode;
        updateScrollBars();
    }
}

Qt::ScrollBarPolicy DAChartScrollZoomer::hScrollBarMode() const
{
    return (d_ptr->mHScrollData->mode);
}

Qt::ScrollBarPolicy DAChartScrollZoomer::vScrollBarMode() const
{
    return (d_ptr->mVScrollData->mode);
}

void DAChartScrollZoomer::setHScrollBarPosition(ScrollBarPosition pos)
{
    if (d_ptr->mHScrollData->position != pos) {
        d_ptr->mHScrollData->position = pos;
        updateScrollBars();
    }
}

void DAChartScrollZoomer::setVScrollBarPosition(ScrollBarPosition pos)
{
    if (d_ptr->mVScrollData->position != pos) {
        d_ptr->mVScrollData->position = pos;
        updateScrollBars();
    }
}

DAChartScrollZoomer::ScrollBarPosition DAChartScrollZoomer::hScrollBarPosition() const
{
    return (d_ptr->mHScrollData->position);
}

DAChartScrollZoomer::ScrollBarPosition DAChartScrollZoomer::vScrollBarPosition() const
{
    return (d_ptr->mVScrollData->position);
}

void DAChartScrollZoomer::setCornerWidget(QWidget* w)
{
    if (w != d_ptr->mCornerWidget) {
        if (canvas()) {
            delete d_ptr->mCornerWidget;
            d_ptr->mCornerWidget = w;
            if (d_ptr->mCornerWidget->parent() != canvas()) {
                d_ptr->mCornerWidget->setParent(canvas());
            }

            updateScrollBars();
        }
    }
}

QWidget* DAChartScrollZoomer::cornerWidget() const
{
    return (d_ptr->mCornerWidget);
}

bool DAChartScrollZoomer::eventFilter(QObject* object, QEvent* event)
{
    if (object == canvas()) {
        switch (event->type()) {
        case QEvent::Resize: {
            int left, top, right, bottom;
            canvas()->getContentsMargins(&left, &top, &right, &bottom);

            QRect rect;
            rect.setSize(static_cast< QResizeEvent* >(event)->size());
            rect.adjust(left, top, -right, -bottom);

            layoutScrollBars(rect);
            break;
        }

        case QEvent::ChildRemoved: {
            const QObject* child = static_cast< QChildEvent* >(event)->child();

            if (child == d_ptr->mCornerWidget) {
                d_ptr->mCornerWidget = nullptr;
            } else if (child == d_ptr->mHScrollData->scrollBar) {
                d_ptr->mHScrollData->scrollBar = nullptr;
            } else if (child == d_ptr->mVScrollData->scrollBar) {
                d_ptr->mVScrollData->scrollBar = nullptr;
            }
            break;
        }

        default:
            break;
        }
    }
    return (QwtPlotZoomer::eventFilter(object, event));
}

bool DAChartScrollZoomer::needScrollBar(Qt::Orientation orientation) const
{
    Qt::ScrollBarPolicy mode;
    double zoomMin, zoomMax, baseMin, baseMax;

    if (orientation == Qt::Horizontal) {
        mode    = d_ptr->mHScrollData->mode;
        baseMin = zoomBase().left();
        baseMax = zoomBase().right();
        zoomMin = zoomRect().left();
        zoomMax = zoomRect().right();
    } else {
        mode    = d_ptr->mVScrollData->mode;
        baseMin = zoomBase().top();
        baseMax = zoomBase().bottom();
        zoomMin = zoomRect().top();
        zoomMax = zoomRect().bottom();
    }

    bool needed = false;

    switch (mode) {
    case Qt::ScrollBarAlwaysOn:
        needed = true;
        break;

    case Qt::ScrollBarAlwaysOff:
        needed = false;
        break;

    default: {
        if ((baseMin < zoomMin) || (baseMax > zoomMax)) {
            needed = true;
        }
        break;
    }
    }
    return (needed);
}

void DAChartScrollZoomer::updateScrollBars()
{
    if (!canvas()) {
        return;
    }

    const int xAxis = QwtPlotZoomer::xAxis();
    const int yAxis = QwtPlotZoomer::yAxis();

    int xScrollBarAxis = xAxis;

    if (hScrollBarPosition() == OppositeToScale) {
        xScrollBarAxis = oppositeAxis(xScrollBarAxis);
    }

    int yScrollBarAxis = yAxis;

    if (vScrollBarPosition() == OppositeToScale) {
        yScrollBarAxis = oppositeAxis(yScrollBarAxis);
    }

    QwtPlotLayout* layout = plot()->plotLayout();

    bool showHScrollBar = needScrollBar(Qt::Horizontal);

    if (showHScrollBar) {
        DAChartScrollBar* sb = scrollBar(Qt::Horizontal);
        sb->setPalette(plot()->palette());
        sb->setInverted(!plot()->axisScaleDiv(xAxis).isIncreasing());
        sb->setBase(zoomBase().left(), zoomBase().right());
        sb->moveSlider(zoomRect().left(), zoomRect().right());

        if (!sb->isVisibleTo(canvas())) {
            if (d_ptr->mIsEnable) {
                sb->show();
            } else {
                sb->hide();
            }
            layout->setCanvasMargin(layout->canvasMargin(xScrollBarAxis) + sb->extent(), xScrollBarAxis);
        }
    } else {
        if (horizontalScrollBar()) {
            horizontalScrollBar()->hide();
            layout->setCanvasMargin(layout->canvasMargin(xScrollBarAxis) - horizontalScrollBar()->extent(), xScrollBarAxis);
        }
    }

    bool showVScrollBar = needScrollBar(Qt::Vertical);

    if (showVScrollBar) {
        DAChartScrollBar* sb = scrollBar(Qt::Vertical);
        sb->setPalette(plot()->palette());
        sb->setInverted(plot()->axisScaleDiv(yAxis).isIncreasing());
        //如果sb->setInverted(! plot()->axisScaleDiv( yAxis ).isIncreasing() );那么向下拉滑动杆，视图向上滚
        sb->setBase(zoomBase().top(), zoomBase().bottom());
        sb->moveSlider(zoomRect().top(), zoomRect().bottom());

        if (!sb->isVisibleTo(canvas())) {
            if (d_ptr->mIsEnable) {
                sb->show();
            } else {
                sb->hide();
            }
            layout->setCanvasMargin(layout->canvasMargin(yScrollBarAxis) + sb->extent(), yScrollBarAxis);
        }
    } else {
        if (verticalScrollBar()) {
            verticalScrollBar()->hide();
            layout->setCanvasMargin(layout->canvasMargin(yScrollBarAxis) - verticalScrollBar()->extent(), yScrollBarAxis);
        }
    }

    if (showHScrollBar && showVScrollBar) {
        if (d_ptr->mIsEnable) {
            if (d_ptr->mCornerWidget == NULL) {
                d_ptr->mCornerWidget = new QWidget(canvas());
                d_ptr->mCornerWidget->setAutoFillBackground(true);
                d_ptr->mCornerWidget->setPalette(plot()->palette());
            }
            d_ptr->mCornerWidget->show();
        } else {
            if (d_ptr->mCornerWidget) {
                d_ptr->mCornerWidget->hide();
            }
        }
    } else {
        if (d_ptr->mCornerWidget) {
            d_ptr->mCornerWidget->hide();
        }
    }

    layoutScrollBars(canvas()->contentsRect());
    plot()->updateLayout();
}

void DAChartScrollZoomer::layoutScrollBars(const QRect& rect)
{
    int hPos = xAxis();

    if (hScrollBarPosition() == OppositeToScale) {
        hPos = oppositeAxis(hPos);
    }

    int vPos = yAxis();

    if (vScrollBarPosition() == OppositeToScale) {
        vPos = oppositeAxis(vPos);
    }

    DAChartScrollBar* hScrollBar = horizontalScrollBar();
    DAChartScrollBar* vScrollBar = verticalScrollBar();

    const int hdim = hScrollBar ? hScrollBar->extent() : 0;
    const int vdim = vScrollBar ? vScrollBar->extent() : 0;

    if (hScrollBar && hScrollBar->isVisible()) {
        int x = rect.x();
        int y = (hPos == QwtPlot::xTop) ? rect.top() : rect.bottom() - hdim + 1;
        int w = rect.width();

        if (vScrollBar && vScrollBar->isVisible()) {
            if (vPos == QwtPlot::yLeft) {
                x += vdim;
            }
            w -= vdim;
        }

        hScrollBar->setGeometry(x, y, w, hdim);
    }
    if (vScrollBar && vScrollBar->isVisible()) {
        int pos = yAxis();
        if (vScrollBarPosition() == OppositeToScale) {
            pos = oppositeAxis(pos);
        }

        int x = (vPos == QwtPlot::yLeft) ? rect.left() : rect.right() - vdim + 1;
        int y = rect.y();

        int h = rect.height();

        if (hScrollBar && hScrollBar->isVisible()) {
            if (hPos == QwtPlot::xTop) {
                y += hdim;
            }

            h -= hdim;
        }

        vScrollBar->setGeometry(x, y, vdim, h);
    }
    if (hScrollBar && hScrollBar->isVisible() && vScrollBar && vScrollBar->isVisible()) {
        if (d_ptr->mCornerWidget) {
            QRect cornerRect(vScrollBar->pos().x(), hScrollBar->pos().y(), vdim, hdim);
            d_ptr->mCornerWidget->setGeometry(cornerRect);
        }
    }
}

void DAChartScrollZoomer::scrollBarMoved(Qt::Orientation o, double min, double max)
{
    Q_UNUSED(max);

    if (o == Qt::Horizontal) {
        moveTo(QPointF(min, zoomRect().top()));
    } else {
        moveTo(QPointF(zoomRect().left(), min));
    }

    Q_EMIT zoomed(zoomRect());
}

int DAChartScrollZoomer::oppositeAxis(int axis) const
{
    switch (axis) {
    case QwtPlot::xBottom:
        return (QwtPlot::xTop);

    case QwtPlot::xTop:
        return (QwtPlot::xBottom);

    case QwtPlot::yLeft:
        return (QwtPlot::yRight);

    case QwtPlot::yRight:
        return (QwtPlot::yLeft);

    default:
        break;
    }

    return (axis);
}

}  // End Of Namespace DA
