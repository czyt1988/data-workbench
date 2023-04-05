#include "DAChartScrollBar.h"
#include <QStyleOption>
#include <QStyleOptionSlider>
namespace DA
{
DAChartScrollBar::DAChartScrollBar(QWidget* parent) : QScrollBar(parent)
{
    init();
}

DAChartScrollBar::DAChartScrollBar(Qt::Orientation o, QWidget* parent) : QScrollBar(o, parent)
{
    init();
}

DAChartScrollBar::DAChartScrollBar(double minBase, double maxBase, Qt::Orientation o, QWidget* parent)
    : QScrollBar(o, parent)
{
    init();
    setBase(minBase, maxBase);
    moveSlider(minBase, maxBase);
}

void DAChartScrollBar::init()
{
    d_inverted  = orientation() == Qt::Vertical;
    d_baseTicks = 1000000;
    d_minBase   = 0.0;
    d_maxBase   = 1.0;
    moveSlider(d_minBase, d_maxBase);

    connect(this, SIGNAL(sliderMoved(int)), SLOT(catchSliderMoved(int)));
    connect(this, SIGNAL(valueChanged(int)), SLOT(catchValueChanged(int)));
}

void DAChartScrollBar::setInverted(bool inverted)
{
    if (d_inverted != inverted) {
        d_inverted = inverted;
        moveSlider(minSliderValue(), maxSliderValue());
    }
}

bool DAChartScrollBar::isInverted() const
{
    return (d_inverted);
}

void DAChartScrollBar::setBase(double min, double max)
{
    if ((min != d_minBase) || (max != d_maxBase)) {
        d_minBase = min;
        d_maxBase = max;

        moveSlider(minSliderValue(), maxSliderValue());
    }
}

void DAChartScrollBar::moveSlider(double min, double max)
{
    const int sliderTicks = qRound((max - min) / (d_maxBase - d_minBase) * d_baseTicks);

    // setRange initiates a valueChanged of the scrollbars
    // in some situations. So we block
    // and unblock the signals.

    blockSignals(true);

    setRange(sliderTicks / 2, d_baseTicks - sliderTicks / 2);
    int steps = sliderTicks / 200;

    if (steps <= 0) {
        steps = 1;
    }

    setSingleStep(steps);
    setPageStep(sliderTicks);

    int tick = mapToTick(min + (max - min) / 2);

    if (isInverted()) {
        tick = d_baseTicks - tick;
    }

    setSliderPosition(tick);
    blockSignals(false);
}

double DAChartScrollBar::minBaseValue() const
{
    return (d_minBase);
}

double DAChartScrollBar::maxBaseValue() const
{
    return (d_maxBase);
}

void DAChartScrollBar::sliderRange(int value, double& min, double& max) const
{
    if (isInverted()) {
        value = d_baseTicks - value;
    }

    const int visibleTicks = pageStep();

    min = mapFromTick(value - visibleTicks / 2);
    max = mapFromTick(value + visibleTicks / 2);
}

double DAChartScrollBar::minSliderValue() const
{
    double min, dummy;

    sliderRange(value(), min, dummy);

    return (min);
}

double DAChartScrollBar::maxSliderValue() const
{
    double max, dummy;

    sliderRange(value(), dummy, max);

    return (max);
}

int DAChartScrollBar::mapToTick(double v) const
{
    const double pos = (v - d_minBase) / (d_maxBase - d_minBase) * d_baseTicks;

    return (static_cast< int >(pos));
}

double DAChartScrollBar::mapFromTick(int tick) const
{
    return (d_minBase + (d_maxBase - d_minBase) * tick / d_baseTicks);
}

void DAChartScrollBar::catchValueChanged(int value)
{
    double min, max;

    sliderRange(value, min, max);
    Q_EMIT valueChanged(orientation(), min, max);
}

void DAChartScrollBar::catchSliderMoved(int value)
{
    double min, max;

    sliderRange(value, min, max);
    Q_EMIT sliderMoved(orientation(), min, max);
}

int DAChartScrollBar::extent() const
{
    QStyleOptionSlider opt;

    opt.init(this);
    opt.subControls       = QStyle::SC_None;
    opt.activeSubControls = QStyle::SC_None;
    opt.orientation       = orientation();
    opt.minimum           = minimum();
    opt.maximum           = maximum();
    opt.sliderPosition    = sliderPosition();
    opt.sliderValue       = value();
    opt.singleStep        = singleStep();
    opt.pageStep          = pageStep();
    opt.upsideDown        = invertedAppearance();
    if (orientation() == Qt::Horizontal) {
        opt.state |= QStyle::State_Horizontal;
    }
    return (style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this));
}
}  // End Of Namespace DA
