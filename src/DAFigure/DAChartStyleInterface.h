#ifndef DACHARTSTYLEINTERFACE_H
#define DACHARTSTYLEINTERFACE_H
#include "DAFigureAPI.h"
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QString>

class QwtPlotGrid;
class QwtPlotLegendItem;

namespace DA
{

/**
 * @brief 图表样式设置接口 - 负责颜色、字体、背景、显示元素等样式设置
 */
class DAFIGURE_API DAChartStyleInterface
{
public:
    virtual ~DAChartStyleInterface() = default;

    // ==================== 图表整体样式 ====================
    virtual void setChartTitle(const QString& title) = 0;
    virtual QString getChartTitle() const            = 0;

    virtual void setBackgroundBrush(const QBrush& brush) = 0;
    virtual QBrush getBackgroundBrush() const            = 0;

    virtual void setBorderColor(const QColor& color) = 0;
    virtual QColor getBorderColor() const            = 0;

    // ==================== 坐标轴样式 ====================
    virtual void setAxisLabel(int axisId, const QString& label) = 0;
    virtual QString getAxisLabel(int axisId) const              = 0;

    virtual void setAxisColor(int axisId, const QColor& color) = 0;
    virtual QColor getAxisColor(int axisId) const              = 0;

    // ==================== 网格样式 ====================
    virtual void enableGrid(bool enable = true) = 0;
    virtual bool isGridEnabled() const          = 0;
    virtual void setGridStyle(const QColor& color, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine, bool isMajor = true) = 0;
    virtual void setGridMajorStyle(const QColor& color, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine) = 0;
    virtual void setGridMinorStyle(const QColor& color, qreal width = 0.5, Qt::PenStyle style = Qt::DotLine) = 0;

    // ==================== 图例样式 ====================
    virtual void enableLegend(bool enable = true) = 0;
    virtual bool isLegendEnabled() const          = 0;

    virtual void setLegendPosition(Qt::Alignment alignment) = 0;
    virtual Qt::Alignment getLegendPosition() const         = 0;

    virtual void setLegendBackground(const QBrush& brush) = 0;
    virtual QBrush getLegendBackground() const            = 0;

    virtual void setLegendTextColor(const QColor& color) = 0;
    virtual QColor getLegendTextColor() const            = 0;

    // ==================== 时间坐标轴 ====================
    virtual void setupDateTimeAxis(int axisId, const QString& format = "yyyy-MM-dd hh:mm:ss") = 0;
    virtual bool isDateTimeAxis(int axisId) const                                             = 0;
};

}  // namespace DA
#endif  // DACHARTSTYLEINTERFACE_H
