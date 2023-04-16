#include "DAChartXYDataPicker.h"
#include <algorithm>
#include <numeric>
#include <math.h>
//
#include <QPainter>
#include <QFont>
// qwt
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_picker_machine.h"
#include "qwt_plot_dict.h"
#include "qwt_plot.h"
#include "qwt_painter.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
//
#include "DAChartUtil.h"
namespace DA
{

//===================================================
// DAChartXYDataPickerClosePointInfo
//===================================================
///
/// \brief 记录最近点的信息
///
class DAChartXYDataPickerClosePointInfo
{
public:
    DAChartXYDataPickerClosePointInfo();
    QwtPlotItem* item() const
    {
        return this->mItem;
    }
    void setItem(QwtPlotItem* item);
    bool isValid() const;
    QPointF getClosePoint() const;
    void setClosePoint(const QPointF& p);
    int index() const
    {
        return this->mIndex;
    }
    void setIndex(int i)
    {
        this->mIndex = i;
    }
    double distace() const
    {
        return this->mDistace;
    }
    void setDistace(double d)
    {
        this->mDistace = d;
    }
    void setInvalid();

private:
    QwtPlotItem* mItem { nullptr };
    int mIndex { -1 };
    double mDistace { std::numeric_limits< double >::max() };
    QPointF mPoint;
};

DAChartXYDataPickerClosePointInfo::DAChartXYDataPickerClosePointInfo()
{
}

void DAChartXYDataPickerClosePointInfo::setItem(QwtPlotItem* item)
{
    this->mItem = item;
}

bool DAChartXYDataPickerClosePointInfo::isValid() const
{
    return ((this->item() != nullptr) && (this->index() >= 0));
}

QPointF DAChartXYDataPickerClosePointInfo::getClosePoint() const
{
    if (isValid())
        return mPoint;
    return QPointF();
}

void DAChartXYDataPickerClosePointInfo::setClosePoint(const QPointF& p)
{
    mPoint = p;
}

void DAChartXYDataPickerClosePointInfo::setInvalid()
{
    setItem(nullptr);
    setIndex(-1);
    setDistace(std::numeric_limits< double >::max());
    mPoint = QPointF();
}

//===================================================
// DAChartXYDataPickerPrivate
//===================================================
class DAChartXYDataPicker::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartXYDataPicker)
public:
    PrivateData(DAChartXYDataPicker* p);
    DAChartXYDataPickerClosePointInfo mClosePointInfo;
    QPen mPen;
};
DAChartXYDataPicker::PrivateData::PrivateData(DAChartXYDataPicker* p) : q_ptr(p)
{
}
//===================================================
// DAChartXYDataPicker
//===================================================
DAChartXYDataPicker::DAChartXYDataPicker(QWidget* canvas) : QwtPlotPicker(canvas), DA_PIMPL_CONSTRUCT
{
    setTrackerMode(QwtPlotPicker::ActiveOnly);
    setRubberBand(UserRubberBand);
    setStateMachine(new QwtPickerTrackerMachine());
    connect(this, &QwtPicker::moved, this, &DAChartXYDataPicker::mouseMove);
    d_ptr->mPen.setWidth(1);
    if (plot())
        connect(plot(), &QwtPlot::itemAttached, this, &DAChartXYDataPicker::itemAttached);
}

DAChartXYDataPicker::~DAChartXYDataPicker()
{
}

QwtText DAChartXYDataPicker::trackerTextF(const QPointF& pos) const
{
    Q_UNUSED(pos);
    QwtText trackerText;
    if (!d_ptr->mClosePointInfo.isValid())
        return trackerText;
    trackerText.setColor(Qt::black);
    QColor lineColor = getItemColor(d_ptr->mClosePointInfo.item());
    QColor bkColor(lineColor);
    bkColor.setAlpha(30);
    // trackerText.setBorderPen( d_ptr->_closePointInfo.item()->pen () );
    trackerText.setBackgroundBrush(bkColor);
    QPointF point = d_ptr->mClosePointInfo.getClosePoint();
    QString info  = QString("<font color=\"%1\">y:%2</font><br>").arg(lineColor.name()).arg(point.y())
                   + QString("<font color=\"%1\">x:%2</font>").arg(lineColor.name()).arg(point.x());
    trackerText.setText(info);
    trackerText.setBorderRadius(5);
    return trackerText;
}

QRect DAChartXYDataPicker::trackerRect(const QFont& font) const
{
    QRect r = QwtPlotPicker::trackerRect(font);
    r += QMargins(5, 5, 5, 5);
    return r;
}

void DAChartXYDataPicker::drawRubberBand(QPainter* painter) const
{
    if (!isActive() || rubberBand() == NoRubberBand || rubberBandPen().style() == Qt::NoPen) {
        return;
    }
    if (!d_ptr->mClosePointInfo.isValid())
        return;
    //获取鼠标的客户坐标位置
    const QPoint pos = trackerPosition();
    if (pos.isNull())
        return;
    painter->setPen(d_ptr->mPen);
    const QPointF closePoint = d_ptr->mClosePointInfo.getClosePoint();
    const QPoint cvp         = transform(closePoint);
    QwtPainter::drawLine(painter, pos, cvp);
    QRect r(0, 0, 10, 10);
    r.moveCenter(cvp);
    QwtPainter::drawRect(painter, r);
}

int DAChartXYDataPicker::itemClosedPoint(const QwtPlotItem* item, const QPoint& pos, QPointF* itemPoint, double* dist)
{
    int index = -1;
    QPointF point;
#if 1
    if (const QwtPlotCurve* pc = dynamic_cast< const QwtPlotCurve* >(item)) {
        index = pc->closestPoint(pos, dist);
        if (-1 != index) {
            point = pc->sample(index);
        }
    } else if (const QwtPlotBarChart* pb = dynamic_cast< const QwtPlotBarChart* >(item)) {
        index = DAChartUtil::closestPoint(pb, pos, dist);
        if (-1 != index) {
            point = pb->sample(index);
        }
    }
#else
    switch (item->rtti()) {
    case QwtPlotItem::Rtti_PlotCurve: {
        const QwtPlotCurve* cur = static_cast< const QwtPlotCurve* >(item);
        index                   = cur->closestPoint(pos, dist);
        if (-1 != index) {
            point = cur->sample(index);
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotBarChart: {
        const QwtPlotBarChart* bar = static_cast< const QwtPlotBarChart* >(item);
        index                      = SAChart::closestPoint(bar, pos, dist);
        if (-1 != index) {
            point = bar->sample(index);
        }
        break;
    }
    default:
        break;
    }
#endif
    if (itemPoint) {
        *itemPoint = point;
    }
    return index;
}
///
/// \brief 遍历所有数据找到最近点
/// \param pos 绘图坐标
/// \note 此算法是遍历所有数据，在数据量大时请谨慎
///
void DAChartXYDataPicker::calcClosestPoint(const QPoint& pos)
{
    const QwtPlotItemList curveItems = plot()->itemList();
    if (curveItems.size() <= 0)
        return;
    //把屏幕坐标转换为图形的数值坐标
    // QPointF mousePoint = invTransform(pos);
    //记录最短的距离，默认初始化为double的最大值
    double distance = std::numeric_limits< double >::max();
    //记录前一次最近点的曲线指针

    QPointF point;
    QwtPlotItem* oldItem = d_ptr->mClosePointInfo.item();
    for (int i = 0; i < curveItems.size(); ++i) {
        double dp;
        int index         = -1;
        QwtPlotItem* item = curveItems[ i ];
        index             = itemClosedPoint(item, pos, &point, &dp);
        if (-1 == index)
            continue;
        // QPointF p = cur->sample (index);
        if (dp < distance) {
            d_ptr->mClosePointInfo.setDistace(dp);  //实际距离需要开方
            d_ptr->mClosePointInfo.setIndex(index);
            d_ptr->mClosePointInfo.setItem(item);
            d_ptr->mClosePointInfo.setClosePoint(point);
            distance = dp;
        }
    }
    //说明最近点的曲线更换了，标记线的颜色换为当前曲线的颜色
    if (d_ptr->mClosePointInfo.isValid() && oldItem != d_ptr->mClosePointInfo.item()) {
        d_ptr->mPen.setColor(getItemColor(d_ptr->mClosePointInfo.item()));
        setRubberBandPen(d_ptr->mPen);
    }
}

double DAChartXYDataPicker::distancePower(const QPointF& p1, const QPointF& p2)
{
    return pow(p1.x() - p2.x(), 2.0) + pow(p1.y() - p2.y(), 2.0);
}

QColor DAChartXYDataPicker::getItemColor(const QwtPlotItem* item) const
{
    QColor c = DAChartUtil::getPlotItemColor(item);
    if (c.isValid()) {
        return c;
    }
    return Qt::black;
}

void DAChartXYDataPicker::mouseMove(const QPoint& pos)
{
    calcClosestPoint(pos);
}

void DAChartXYDataPicker::itemAttached(QwtPlotItem* plotItem, bool on)
{
    if (!on) {
        if (plotItem == d_ptr->mClosePointInfo.item())
            d_ptr->mClosePointInfo.setInvalid();
    }
}

}  // End Of Namespace DA
