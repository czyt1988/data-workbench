#include "DAChartAxisRangeBinder.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
namespace DA
{

DAChartAxisRangeBinder::DAChartAxisRangeBinder(QObject* par) : QObject(par)
{
}

DAChartAxisRangeBinder::DAChartAxisRangeBinder(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid, QObject* par)
    : QObject(par)
{
    setSourceChart(source, sourceAxisid);
    setFollowerChart(follower, followerAxisid);
    bind();
}

void DAChartAxisRangeBinder::setSourceChart(QwtPlot* source, int axisid)
{
    m_sourcePlot   = source;
    m_sourceAxisid = axisid;
}

QwtPlot* DAChartAxisRangeBinder::getSourcePlot() const
{
    return m_sourcePlot.data();
}

int DAChartAxisRangeBinder::getSourceAxis() const
{
    return m_sourceAxisid;
}

void DAChartAxisRangeBinder::setFollowerChart(QwtPlot* follower, int axisid)
{
    m_followerPlot   = follower;
    m_followerAxisid = axisid;
}

QwtPlot* DAChartAxisRangeBinder::getFollowerPlot() const
{
    return m_followerPlot.data();
}

int DAChartAxisRangeBinder::getFollowerAxis() const
{
    return m_followerAxisid;
}

bool DAChartAxisRangeBinder::bind()
{
    if (!isValid()) {
        return false;
    }
    QwtScaleWidget* axisSource = m_sourcePlot->axisWidget(m_sourceAxisid);

    m_con = QObject::connect(
        axisSource, &QwtScaleWidget::scaleDivChanged, this, &DAChartAxisRangeBinder::onSourcePlotScaleDivChanged);
    return m_con;
}

bool DAChartAxisRangeBinder::unbind()
{
    if (!isBinded()) {
        return false;
    }
    return QObject::disconnect(m_con);
}

bool DAChartAxisRangeBinder::isValid() const
{
    return (m_sourcePlot && m_followerPlot && QwtAxis::isValid(m_sourceAxisid) && QwtAxis::isValid(m_followerAxisid));
}

bool DAChartAxisRangeBinder::isSame(const DAChartAxisRangeBinder& other) const
{
    return (m_sourcePlot == other.m_sourcePlot) && (m_sourceAxisid == other.m_sourceAxisid)
           && (m_followerPlot == other.m_followerPlot) && (m_followerAxisid == other.m_followerAxisid);
}

bool DAChartAxisRangeBinder::isSame(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid) const
{
    return (m_sourcePlot == source) && (m_sourceAxisid == sourceAxisid) && (m_followerPlot == follower)
           && (m_followerAxisid == followerAxisid);
}

bool DAChartAxisRangeBinder::operator==(const DAChartAxisRangeBinder& other) const
{
    return isSame(other);
}

void DAChartAxisRangeBinder::onSourcePlotScaleDivChanged()
{
    if (!m_followerPlot || !m_sourcePlot || !QwtAxis::isValid(m_sourceAxisid) || !QwtAxis::isValid(m_followerAxisid)) {
        return;
    }
    m_followerPlot->setAxisScaleDiv(m_followerAxisid, m_sourcePlot->axisScaleDiv(m_sourceAxisid));
    if (m_replotImmediately) {
        m_followerPlot->replotAll();
    }
}

bool DAChartAxisRangeBinder::isRplotImmediately() const
{
    return m_replotImmediately;
}

void DAChartAxisRangeBinder::setReplotImmediately(bool v)
{
    m_replotImmediately = v;
}

bool DAChartAxisRangeBinder::isBinded() const
{
    return m_con;
}
}
