#include "DAChartItemCreatInteractor.h"
#include <QMouseEvent>
#include <QPen>
#include <QDebug>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"

#include "qwt_plot_item.h"
#include "qwt_plot_marker.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
namespace DA
{
QwtPlotMarker* createMarkerPlotItem(const QPointF& pos, const QString& title, const QPen& pen, QwtPlotMarker::LineStyle lineStyle);

class DAChartItemCreatInteractor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartItemCreatInteractor)
public:
    PrivateData(DAChartItemCreatInteractor* p);
    ~PrivateData();
    void releaseTmpItem();
    FpCreatePlotItem m_createPlotItem { nullptr };
    QwtPlotItem* mTmpItem { nullptr };
};

DAChartItemCreatInteractor::PrivateData::PrivateData(DAChartItemCreatInteractor* p)
{
}

DAChartItemCreatInteractor::PrivateData::~PrivateData()
{
    releaseTmpItem();
}

void DAChartItemCreatInteractor::PrivateData::releaseTmpItem()
{
    if (mTmpItem) {
        mTmpItem->detach();
        delete mTmpItem;
        mTmpItem = nullptr;
    }
}
//==============================================
// DAChartItemCreatInteractor
//==============================================
DAChartItemCreatInteractor::DAChartItemCreatInteractor(QwtPlot* parent, FpCreatePlotItem fun)
    : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    setPlotItemInteractorFactory(fun);
}

DAChartItemCreatInteractor::~DAChartItemCreatInteractor()
{
}
void DAChartItemCreatInteractor::setPlotItemInteractorFactory(FpCreatePlotItem fun)
{
    d_ptr->m_createPlotItem = fun;
}
DAChartItemCreatInteractor::FpCreatePlotItem DAChartItemCreatInteractor::getPlotItemInteractorFactory() const
{
    return d_ptr->m_createPlotItem;
}
int DAChartItemCreatInteractor::rtti() const
{
    return DAAbstractChartEditor::RTTICreatInteractor;
}
QwtPlotItem* DA::DAChartItemCreatInteractor::takeItem()
{
    DA_D(d);
    QwtPlotItem* item = d->mTmpItem;
    d->mTmpItem       = nullptr;
    return item;
}

bool DAChartItemCreatInteractor::mousePressEvent(const QMouseEvent* e)
{
    if (d_ptr->m_createPlotItem) {
        QwtPlot* gca = plot();
        if (!gca) {
            return false;
        }
        Q_EMIT beginEdit();
        QPoint canvasPos = compat::eventPos(e);
        QwtScaleMap xMap = gca->canvasMap(gca->visibleXAxisId());
        QwtScaleMap yMap = gca->canvasMap(gca->visibleYAxisId());
        QPointF pos      = QPointF(xMap.invTransform(canvasPos.x()), yMap.invTransform(canvasPos.y()));

        QwtPlotItem* item = d_ptr->m_createPlotItem(gca, pos);
        if (item) {
            d_ptr->mTmpItem = item;
        }
        Q_EMIT finishedEdit(false);
    }
    return false;
}

QwtPlotMarker* createMarkerPlotItem(const QPointF& pos, const QString& title, const QPen& pen, QwtPlotMarker::LineStyle lineStyle)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setLineStyle(lineStyle);
    marker->setLinePen(pen);
    marker->setValue(pos);
    return marker;
}

QwtPlotItem* createHLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Horizontal Vertical Line Marker"),  // cn : 水平垂直线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::HLine
    );
    marker->attach(plot);
    return marker;
}

QwtPlotItem* createVLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Vertical Line Marker"),  // cn : 垂直直线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::VLine
    );
    marker->attach(plot);
    return marker;
}

QwtPlotItem* createCrossLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos)
{
    QwtPlotMarker* marker = createMarkerPlotItem(
        pos,
        QObject::tr("Cross Line Marker"),  // cn : 十字线标记
        QPen(Qt::red, 1, Qt::DashLine),
        QwtPlotMarker::Cross
    );
    marker->attach(plot);
    return marker;
}

}  // namespace DA