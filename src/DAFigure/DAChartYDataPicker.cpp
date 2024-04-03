#include "DAChartYDataPicker.h"
// stl
#include <numeric>
// qt
#include <QPalette>
#include <QPen>
#include <QPainter>
#include <QMap>
#include <QDebug>
// qwt
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_picker_machine.h"
#include "qwt_plot_dict.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot.h"
#include "qwt_column_symbol.h"
#include "qwt_painter.h"
//
#include "DAChartUtil.h"
namespace DA
{

class DAChartYDataPicker::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAChartYDataPicker)
    PrivateData(DAChartYDataPicker* p);
    //把ypoints的最高点找到（相对于绘图坐标）,res(top,bottom)
    bool getTopBottomPoint(QPair< QPointF, QPointF >& res) const;
    //刷新点信息
    void resetPointsInfo(const QPointF& pos);
    //
    void resetCurveInfo(const QPointF& pos);
    //
    QString curveInfoAt(QwtPlotCurve*, const QPointF&) const;
    QLineF curveLineAt(QwtPlotCurve*, double x) const;
    QString barInfoAt(QwtPlotBarChart*, const QPointF&) const;
    double barValueAt(QwtPlotBarChart* bar, double x) const;
    //判断是否是nan point
    static bool isNanPoint(const QPointF& p);
    static QPointF makeNanPoint();

public:
    QMap< QwtPlotItem*, QPointF > mYPoints;  ///< 记录所有曲线对应的y点
    // QList< QPair< QwtPlotItem*, QPointF > > _ypoints;  ///< 记录所有曲线对应的y点
    QPen mPen;
    QPoint mMousePoint;
    QString mText;
};

DAChartYDataPicker::PrivateData::PrivateData(DAChartYDataPicker* p) : q_ptr(p)
{
    mYPoints.clear();
    mPen.setColor(Qt::black);
    mPen.setStyle(Qt::DashLine);
}

/**
 * @brief 把ypoints的最高点找到（相对于绘图坐标）
 * @return
 */
bool DAChartYDataPicker::PrivateData::getTopBottomPoint(QPair< QPointF, QPointF >& res) const
{
    if (mYPoints.isEmpty()) {
        return false;
    }
    res.first  = mYPoints.first();
    res.second = mYPoints.first();
    for (auto i = mYPoints.begin() + 1; i != mYPoints.end(); ++i) {
        if (i.value().y() > res.first.y()) {
            res.first = i.value();
        } else if (i.value().y() < res.second.y()) {
            res.second = i.value();
        }
    }
    return true;
}

void DAChartYDataPicker::PrivateData::resetPointsInfo(const QPointF& pos)
{
    mText.clear();
    mYPoints.clear();
    const QwtPlotItemList& items = q_ptr->plot()->itemList();
    QList< QPair< QwtPlotItem*, QPointF > > ordered;
    for (int i = 0; i < items.size(); i++) {
        QPointF p = q_ptr->keyPoint(items[ i ], pos);
        if (isNanPoint(p)) {
            continue;
        }
        ordered.append(qMakePair(items[ i ], p));
        mYPoints[ items[ i ] ] = p;
    }
    //把ordered排序
    std::sort(ordered.begin(), ordered.end(), [](const QPair< QwtPlotItem*, QPointF >& a, const QPair< QwtPlotItem*, QPointF >& b) -> bool {
        return (a.second.y() > b.second.y());  //大于，使得排序是大到小
    });
    //形成文本
    for (int i = 0; i < ordered.size(); ++i) {
        QColor c = DAChartUtil::getPlotItemColor(ordered[ i ].first);
        QString str;
        if (c.isValid()) {
            str = QString("<font color=\"%1\">%2</font>").arg(c.name()).arg(ordered[ i ].second.y());
        } else {
            str = QString("<font color=\"black\">%1</font>").arg(ordered[ i ].second.y());
        }
        if (0 != i) {
            str = "<br/>" + str;
        }
        mText += str;
    }
}

/**
 * @brief 判断是否是nan
 * @param p
 * @return
 */
bool DAChartYDataPicker::PrivateData::isNanPoint(const QPointF& p)
{
    return (qIsNaN(p.x()) || qIsNaN(p.y()));
}

/**
 * @brief 构建一个nan point
 * @return
 */
QPointF DAChartYDataPicker::PrivateData::makeNanPoint()
{
    return QPointF(qQNaN(), qQNaN());
}
//===================================================
// DAChartYDataPicker
//===================================================

DAChartYDataPicker::DAChartYDataPicker(QWidget* canvas) : QwtPlotPicker(canvas), DA_PIMPL_CONSTRUCT
{
    setTrackerMode(QwtPlotPicker::ActiveOnly);
    //    setRubberBand(HLineRubberBand);
    setRubberBand(UserRubberBand);
    setStateMachine(new QwtPickerTrackerMachine());
}

DAChartYDataPicker::~DAChartYDataPicker()
{
}

QRect DAChartYDataPicker::trackerRect(const QFont& font) const
{
    qDebug() << "trackerRect";
    //! 多个点无法提供一个Rect，因此，把rect放置在鼠标那里
    QRect r            = QwtPlotPicker::trackerRect(font);
    const QRect axRect = transform(scaleRect());
    //判断是显示到鼠标上面还是鼠标下面
    if (d_ptr->mMousePoint.y() - r.height() < axRect.top()) {
        //显示到鼠标上面会被鼠标遮挡，默认显示到鼠标下面
        r.moveTop(d_ptr->mMousePoint.y());
    } else {
        //否则都显示到鼠标下面
        r.moveBottom(d_ptr->mMousePoint.y());
    }
    //判断是显示到左边还是右边

    if (d_ptr->mMousePoint.x() + r.width() > axRect.right()) {
        r.moveRight(d_ptr->mMousePoint.x());
    } else {
        r.moveLeft(d_ptr->mMousePoint.x());
    }
    return r;
}

QPointF DAChartYDataPicker::keyPoint(QwtPlotItem* item, const QPointF& pos) const
{
    if (QwtPlotCurve* c = dynamic_cast< QwtPlotCurve* >(item)) {
        return getKeyPoint(c, pos);
    } else if (QwtPlotBarChart* c = dynamic_cast< QwtPlotBarChart* >(item)) {
        return getKeyPoint(c, pos);
    }
    return PrivateData::makeNanPoint();
}

void DAChartYDataPicker::drawRubberBand(QPainter* painter) const
{
    qDebug() << "drawRubberBand";
    if (!isActive() || rubberBand() == NoRubberBand || rubberBandPen().style() == Qt::NoPen) {
        return;
    }
    if (d_ptr->mYPoints.size() == 0) {
        return;
    }
    painter->setPen(d_ptr->mPen);
    QRect axisrect = transform(scaleRect());
    QPoint topPoint, bottomPoint;
    for (auto i = d_ptr->mYPoints.begin(); i != d_ptr->mYPoints.end(); ++i) {
        const QPoint cvp = transform(i.value());
        if (d_ptr->mYPoints.begin() == i) {
            topPoint    = cvp;
            bottomPoint = cvp;
        }
        QwtPainter::drawLine(painter, QPointF(axisrect.left(), cvp.y()), QPointF(axisrect.right(), cvp.y()));
        if (cvp.y() < topPoint.y()) {
            topPoint = cvp;
        } else if (cvp.y() > bottomPoint.y()) {
            bottomPoint = cvp;
        }
        QRect r(0, 0, 10, 10);
        r.moveCenter(cvp);
        QwtPainter::drawRect(painter, r);
    }
    //绘制鼠标到toppoint和bottompoint
    if (d_ptr->mMousePoint.y() > bottomPoint.y()) {
        //说明鼠标在所有点之下
        QwtPainter::drawLine(painter, QPoint(bottomPoint.x(), d_ptr->mMousePoint.y()), bottomPoint);
        QwtPainter::drawLine(painter, bottomPoint, topPoint);
    } else if (d_ptr->mMousePoint.y() < topPoint.y()) {
        //说明鼠标在所有点之上
        QwtPainter::drawLine(painter, QPoint(topPoint.x(), d_ptr->mMousePoint.y()), topPoint);
        QwtPainter::drawLine(painter, topPoint, bottomPoint);
    } else {
        //说明鼠标在上下中间
        QwtPainter::drawLine(painter, QPoint(topPoint.x(), d_ptr->mMousePoint.y()), topPoint);
        QwtPainter::drawLine(painter, QPoint(bottomPoint.x(), d_ptr->mMousePoint.y()), bottomPoint);
    }
}

void DAChartYDataPicker::move(const QPoint& pos)
{
    d_ptr->mMousePoint = pos;
    QwtPlotPicker::move(pos);
}

/**
 * @brief 曲线的点
 * @param curve 曲线指针
 * @param pos 鼠标位置
 * @return 返回对应的点
 */
QPointF DAChartYDataPicker::getKeyPoint(QwtPlotCurve* c, const QPointF& pos) const
{
    const QLineF line = getCurveLine(c, pos.x());
    if (line.isNull()) {
        return d_ptr->makeNanPoint();  //一个nanpoint
    }
    const double y = line.pointAt((pos.x() - line.p1().x()) / line.dx()).y();
    return QPointF(pos.x(), y);
}

/**
 * @brief bar的关键点
 * @param c
 * @param pos
 * @return
 */
QPointF DAChartYDataPicker::getKeyPoint(QwtPlotBarChart* c, const QPointF& pos) const
{
    const double y = getBarValue(c, pos.x());
    if (qIsNaN(y)) {
        return PrivateData::makeNanPoint();
    }
    return QPointF(pos.x(), y);
}

QwtText DAChartYDataPicker::trackerTextF(const QPointF& pos) const
{
    qDebug() << "trackerTextF=" << pos;
    d_ptr->resetPointsInfo(pos);
    QwtText trackerText;

    trackerText.setColor(Qt::black);

    QColor c(200, 200, 200, 100);
    trackerText.setBorderPen(QPen(c, 2));
    trackerText.setBackgroundBrush(c);
    trackerText.setText(d_ptr->mText);
    return trackerText;
}

/**
 * @brief 找到x点对应的线段
 * @param curve
 * @param x
 * @return
 */
QLineF DAChartYDataPicker::getCurveLine(QwtPlotCurve* curve, double x) const
{
    QLineF line;

    if (curve->dataSize() >= 2) {
        const QRectF br = curve->boundingRect();
        if (br.isValid() && x >= br.left() && x <= br.right()) {
            int index = qwtUpperSampleIndex< QPointF >(*curve->data(), x, [](const double x, const QPointF& pos) -> bool {
                return (x < pos.x());
            });

            if (index == -1 && x == curve->sample(static_cast<int>(curve->dataSize() - 1)).x()) {
                // the last sample is excluded from qwtUpperSampleIndex
                index = static_cast<int>(curve->dataSize() - 1);
            }

            if (index > 0) {
                line.setP1(curve->sample(index - 1));
                line.setP2(curve->sample(index));
            }
        }
    }

    return line;
}

double DAChartYDataPicker::getBarValue(QwtPlotBarChart* bar, double x) const
{
    if (bar->dataSize() >= 2) {
        const QRectF br = bar->boundingRect();
        if (br.isValid() && x >= br.left() && x <= br.right()) {
            int index = qwtUpperSampleIndex< QPointF >(*bar->data(), x, [](const double& x1, const QPointF& p) -> bool {
                return (x1 < p.x());
            });
            if (index == -1 && x == bar->sample(static_cast<int>(bar->dataSize() - 1)).x()) {
                // the last sample is excluded from qwtUpperSampleIndex
                index = static_cast<int>(bar->dataSize() - 1);
            }
            if (index > 0) {
                return bar->sample(index).y();
            }
        }
    }
    return qQNaN();
}

}  // End Of Namespace DA
