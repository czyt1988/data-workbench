#include "DAChartCrossTracker.h"
#include <QPen>
#include "qwt_picker_machine.h"
#include "qwt_plot.h"
#include "qwt_date_scale_draw.h"
namespace DA
{
/**
 * @brief 构造函数
 * @param w 关联的控件
 */
DAChartCrossTracker::DAChartCrossTracker(QWidget* w) : QwtPlotPicker(w)
{
    init();
}

/**
 * @brief 构造函数，指定坐标轴
 * @param xAxis X轴ID
 * @param yAxis Y轴ID
 * @param w 关联的控件
 */
DAChartCrossTracker::DAChartCrossTracker(int xAxis, int yAxis, QWidget* w) : QwtPlotPicker(xAxis, yAxis, w)
{
    init();
}

/**
 * @brief 初始化十字光标追踪器
 */
void DAChartCrossTracker::init()
{
    setTrackerMode(QwtPlotPicker::AlwaysOn);  //这是指定文字的显示，AlwaysOn值，光标不激活，也显示文字提示
    setRubberBand(QwtPlotPicker::CrossRubberBand);
    setStateMachine(new QwtPickerTrackerMachine());  // QwtPickerTrackerMachine是不用鼠标激活
    setRubberBandPen(QPen(QColor(186, 85, 211)));
}

/**
 * @brief 返回指定位置处的追踪文本
 * @param pos 鼠标位置（绘图坐标）
 * @return 格式化后的追踪文本
 */
QwtText DAChartCrossTracker::trackerTextF(const QPointF& pos) const
{
    QString s("");
    const QwtScaleDraw* sd = plot()->axisScaleDraw(QwtPlot::xBottom);
    if (sd != nullptr) {
        const QwtDateScaleDraw* dsd = dynamic_cast< const QwtDateScaleDraw* >(sd);
        if (dsd != nullptr) {
            //说明坐标轴是时间轴
            s += QStringLiteral("(%1,").arg(dsd->label(pos.x()).text());
        } else {
            s += QString("(%1,").arg(pos.x());
        }
    }
    sd = plot()->axisScaleDraw(QwtPlot::yLeft);
    if (sd != nullptr) {
        const QwtDateScaleDraw* dsd = dynamic_cast< const QwtDateScaleDraw* >(sd);
        if (dsd != nullptr) {
            //说明坐标轴是时间轴
            s += QStringLiteral("%1)").arg(dsd->label(pos.y()).text());
        } else {
            s += QString("%2)").arg(pos.y());
        }
    }
    // axisScaleDraw

    QwtText text(s);
    text.setColor(Qt::white);
    QColor c = rubberBandPen().color();
    text.setBorderPen(QPen(c));
    text.setBorderRadius(6);
    c.setAlpha(200);
    text.setBackgroundBrush(c);

    return text;
}
}  // End Of Namespace DA
