#ifndef DACHARTARROWEDITOR_H
#define DACHARTARROWEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractTwoPointEditor.h"
#include "DAArrowSymbol.h"
class QKeyEvent;
namespace DA
{
/**
 * @brief 箭头编辑器
 * 
 * 用于在图表上绘制箭头的编辑器，通过两次点击确定箭头的起点和终点
 */
class DAFIGURE_API DAChartArrowEditor : public DAAbstractTwoPointEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartArrowEditor)
public:
    explicit DAChartArrowEditor(QwtPlot* parent);
    virtual ~DAChartArrowEditor();
    
    // rtti
    virtual int rtti() const override;
    
    // 设置箭头线宽
    void setArrowLineWidth(qreal width);
    qreal getArrowLineWidth() const;
    
    // 设置箭头颜色
    void setArrowColor(const QColor& color);
    QColor getArrowColor() const;
    
    // 设置箭头大小
    void setArrowSize(qreal size);
    qreal getArrowSize() const;
    
    // 设置箭头样式
    void setArrowStyle(DAArrowSymbol::ArrowStyle style);
    DAArrowSymbol::ArrowStyle getArrowStyle() const;

protected:
    // 创建图表项
    virtual QwtPlotItem* createPlotItem(const QPointF& startPoint, const QPointF& endPoint) override;
    
    // 更新预览项
    virtual void updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint) override;
    
    // 清理预览项
    virtual void clearPreviewItem() override;
    
private:
    // 创建箭头标记项
    QwtPlotItem* createArrowMarker(const QPointF& startPoint, const QPointF& endPoint);
    
    // 计算箭头角度
    qreal calculateArrowAngle(const QPointF& startPoint, const QPointF& endPoint) const;
};


/**
 * @brief 在pos位置创建箭头标记
 *
 * @param plot 图表
 * @param startPoint 起点位置
 * @param endPoint 终点位置
 * @return QwtPlotItem* 图表项
 */
DAFIGURE_API QwtPlotItem* createArrowMarkerPlotItem(QwtPlot* plot, const QPointF& startPoint, const QPointF& endPoint);

/**
 * @brief 在pos位置创建线段
 *
 * @param plot 图表
 * @param startPoint 起点位置
 * @param endPoint 终点位置
 * @return QwtPlotItem* 图表项
 */
DAFIGURE_API QwtPlotItem* createLinePlotItem(QwtPlot* plot, const QPointF& startPoint, const QPointF& endPoint);

}  // End Of Namespace DA
#endif  // DACHARTARROWEDITOR_H