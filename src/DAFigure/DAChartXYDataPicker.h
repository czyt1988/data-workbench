#ifndef DACHARTXYDATAPICKER_H
#define DACHARTXYDATAPICKER_H
#include "DAFigureAPI.h"
#include <QColor>
#include <QRect>
#include <QPen>
#include <QPointF>
#include "qwt_plot_picker.h"
#include "qwt_text.h"
class QwtPlotCurve;
class QwtPlotItem;
namespace DA
{
/**
 * @brief 点数据拾取
 */
class DAFIGURE_API DAChartXYDataPicker : public QwtPlotPicker
{
    DA_DECLARE_PRIVATE(DAChartXYDataPicker)
public:
    DAChartXYDataPicker(QWidget* canvas);
    ~DAChartXYDataPicker();

protected:
    virtual QwtText trackerTextF(const QPointF& pos) const;
    virtual QRect trackerRect(const QFont& font) const;
    virtual void drawRubberBand(QPainter* painter) const;
    // item最近点，如果有新的item，继承此类并重写此函数即可
    virtual int itemClosedPoint(const QwtPlotItem* item, const QPoint& pos, QPointF* itemPoint, double* dist);
    void calcClosestPoint(const QPoint& pos);
    static double distancePower(const QPointF& p1, const QPointF& p2);
    //获取item对应的颜色，对应自定义的item，需要重载此函数
    virtual QColor getItemColor(const QwtPlotItem* item) const;
private slots:
    //捕获鼠标移动的槽
    void mouseMove(const QPoint& pos);
public slots:
    void itemAttached(QwtPlotItem* plotItem, bool on);
};
}  // End Of Namespace DA
#endif  // DACHARTXYDATAPICKER_H
