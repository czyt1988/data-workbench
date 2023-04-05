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
class DAChartScrollZoomerPrivate
{
    DA_IMPL_PUBLIC(DAChartScrollZoomer)
public:
    DAChartScrollZoomerPrivate(DAChartScrollZoomer* p);
    QWidget* _cornerWidget;

    _DAChartScrollZoomerScrollData* _hScrollData;
    _DAChartScrollZoomerScrollData* _vScrollData;

    bool _inZoom;
    bool _alignCanvasToScales[ QwtPlot::axisCnt ];
    bool _isEnable;  ///< 标定是否显示滚动条
};

DAChartScrollZoomerPrivate::DAChartScrollZoomerPrivate(DAChartScrollZoomer* p)
    : q_ptr(p), _cornerWidget(nullptr), _hScrollData(nullptr), _vScrollData(nullptr), _inZoom(false), _isEnable(true)
{
}
//===================================================
// DAChartScrollZoomer
//===================================================

DAChartScrollZoomer::DAChartScrollZoomer(int xAxis, int yAxis, QWidget* canvas)
    : QwtPlotZoomer(xAxis, yAxis, canvas), d_ptr(new DAChartScrollZoomerPrivate(this))

{
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        d_ptr->_alignCanvasToScales[ axis ] = false;
    }

    if (!canvas) {
        return;
    }

    d_ptr->_hScrollData = new _DAChartScrollZoomerScrollData;
    d_ptr->_vScrollData = new _DAChartScrollZoomerScrollData;
}

DAChartScrollZoomer::DAChartScrollZoomer(QWidget* canvas)
    : QwtPlotZoomer(canvas), d_ptr(new DAChartScrollZoomerPrivate(this))
{
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        d_ptr->_alignCanvasToScales[ axis ] = false;
    }

    if (!canvas) {
        return;
    }

    d_ptr->_hScrollData = new _DAChartScrollZoomerScrollData;
    d_ptr->_vScrollData = new _DAChartScrollZoomerScrollData;
}

DAChartScrollZoomer::~DAChartScrollZoomer()
{
    delete d_ptr->_cornerWidget;
    delete d_ptr->_vScrollData;
    delete d_ptr->_hScrollData;
}

void DAChartScrollZoomer::rescale()
{
    QwtScaleWidget* xScale = plot()->axisWidget(xAxis());
    QwtScaleWidget* yScale = plot()->axisWidget(yAxis());

    if (zoomRectIndex() <= 0) {
        if (d_ptr->_inZoom) {
            xScale->setMinBorderDist(0, 0);
            yScale->setMinBorderDist(0, 0);

            QwtPlotLayout* layout = plot()->plotLayout();

            for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
                layout->setAlignCanvasToScale(axis, d_ptr->_alignCanvasToScales);
            }

            d_ptr->_inZoom = false;
        }
    } else {
        if (!d_ptr->_inZoom) {
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
                d_ptr->_alignCanvasToScales[ axis ] = layout->alignCanvasToScale(axis);
            }

            layout->setAlignCanvasToScales(false);

            d_ptr->_inZoom = true;
        }
    }

    QwtPlotZoomer::rescale();
    updateScrollBars();
}

bool DAChartScrollZoomer::isEnableScrollBar() const
{
    return (d_ptr->_isEnable);
}

void DAChartScrollZoomer::on_enable_scrollBar(bool enable)
{
    d_ptr->_isEnable = enable;
    updateScrollBars();
}

DAChartScrollBar* DAChartScrollZoomer::scrollBar(Qt::Orientation orientation)
{
    DAChartScrollBar*& sb = (orientation == Qt::Vertical) ? d_ptr->_vScrollData->scrollBar : d_ptr->_hScrollData->scrollBar;

    if (sb == NULL) {
        sb = new DAChartScrollBar(orientation, canvas());
        sb->hide();
        connect(sb, SIGNAL(valueChanged(Qt::Orientation, double, double)), SLOT(scrollBarMoved(Qt::Orientation, double, double)));
    }
    return (sb);
}

DAChartScrollBar* DAChartScrollZoomer::horizontalScrollBar() const
{
    return (d_ptr->_hScrollData->scrollBar);
}

DAChartScrollBar* DAChartScrollZoomer::verticalScrollBar() const
{
    return (d_ptr->_vScrollData->scrollBar);
}

void DAChartScrollZoomer::setHScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if (hScrollBarMode() != mode) {
        d_ptr->_hScrollData->mode = mode;
        updateScrollBars();
    }
}

void DAChartScrollZoomer::setVScrollBarMode(Qt::ScrollBarPolicy mode)
{
    if (vScrollBarMode() != mode) {
        d_ptr->_vScrollData->mode = mode;
        updateScrollBars();
    }
}

Qt::ScrollBarPolicy DAChartScrollZoomer::hScrollBarMode() const
{
    return (d_ptr->_hScrollData->mode);
}

Qt::ScrollBarPolicy DAChartScrollZoomer::vScrollBarMode() const
{
    return (d_ptr->_vScrollData->mode);
}

void DAChartScrollZoomer::setHScrollBarPosition(ScrollBarPosition pos)
{
    if (d_ptr->_hScrollData->position != pos) {
        d_ptr->_hScrollData->position = pos;
        updateScrollBars();
    }
}

void DAChartScrollZoomer::setVScrollBarPosition(ScrollBarPosition pos)
{
    if (d_ptr->_vScrollData->position != pos) {
        d_ptr->_vScrollData->position = pos;
        updateScrollBars();
    }
}

DAChartScrollZoomer::ScrollBarPosition DAChartScrollZoomer::hScrollBarPosition() const
{
    return (d_ptr->_hScrollData->position);
}

DAChartScrollZoomer::ScrollBarPosition DAChartScrollZoomer::vScrollBarPosition() const
{
    return (d_ptr->_vScrollData->position);
}

void DAChartScrollZoomer::setCornerWidget(QWidget* w)
{
    if (w != d_ptr->_cornerWidget) {
        if (canvas()) {
            delete d_ptr->_cornerWidget;
            d_ptr->_cornerWidget = w;
            if (d_ptr->_cornerWidget->parent() != canvas()) {
                d_ptr->_cornerWidget->setParent(canvas());
            }

            updateScrollBars();
        }
    }
}

QWidget* DAChartScrollZoomer::cornerWidget() const
{
    return (d_ptr->_cornerWidget);
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

            if (child == d_ptr->_cornerWidget) {
                d_ptr->_cornerWidget = nullptr;
            } else if (child == d_ptr->_hScrollData->scrollBar) {
                d_ptr->_hScrollData->scrollBar = nullptr;
            } else if (child == d_ptr->_vScrollData->scrollBar) {
                d_ptr->_vScrollData->scrollBar = nullptr;
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
        mode    = d_ptr->_hScrollData->mode;
        baseMin = zoomBase().left();
        baseMax = zoomBase().right();
        zoomMin = zoomRect().left();
        zoomMax = zoomRect().right();
    } else {
        mode    = d_ptr->_vScrollData->mode;
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
            if (d_ptr->_isEnable) {
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
            if (d_ptr->_isEnable) {
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
        if (d_ptr->_isEnable) {
            if (d_ptr->_cornerWidget == NULL) {
                d_ptr->_cornerWidget = new QWidget(canvas());
                d_ptr->_cornerWidget->setAutoFillBackground(true);
                d_ptr->_cornerWidget->setPalette(plot()->palette());
            }
            d_ptr->_cornerWidget->show();
        } else {
            if (d_ptr->_cornerWidget) {
                d_ptr->_cornerWidget->hide();
            }
        }
    } else {
        if (d_ptr->_cornerWidget) {
            d_ptr->_cornerWidget->hide();
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
        if (d_ptr->_cornerWidget) {
            QRect cornerRect(vScrollBar->pos().x(), hScrollBar->pos().y(), vdim, hdim);
            d_ptr->_cornerWidget->setGeometry(cornerRect);
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
