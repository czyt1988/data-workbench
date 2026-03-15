#ifndef DACHARTLINEEDITOR_H
#define DACHARTLINEEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractTwoPointEditor.h"
class QKeyEvent;
namespace DA
{
/**
 * @brief 线段编辑器
 * 
 * 用于在图表上绘制线段的编辑器，通过两次点击确定线段的起点和终点
 */
class DAFIGURE_API DAChartLineEditor : public DAAbstractTwoPointEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartLineEditor)
public:
    explicit DAChartLineEditor(QwtPlot* parent);
    virtual ~DAChartLineEditor();
    
    // rtti
    virtual int rtti() const override;
    
    // 设置线段颜色
    void setLineColor(const QColor& color);
    QColor getLineColor() const;
    
    // 设置线段宽度
    void setLineWidth(qreal width);
    qreal getLineWidth() const;
    
    // 设置线段样式
    void setLineStyle(Qt::PenStyle style);
    Qt::PenStyle getLineStyle() const;

protected:
    // 创建图表项
    virtual QwtPlotItem* createPlotItem(const QPointF& startPoint, const QPointF& endPoint) override;
    
    // 更新预览项
    virtual void updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint) override;
    
    // 清理预览项
    virtual void clearPreviewItem() override;
};
}  // End Of Namespace DA
#endif  // DACHARTLINEEDITOR_H