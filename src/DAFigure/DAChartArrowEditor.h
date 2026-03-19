#ifndef DACHARTARROWEDITOR_H
#define DACHARTARROWEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractTwoPointEditor.h"
#include "qwt_plot_arrowmarker.h"
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

    // 设置箭头线笔
    void setArrowLinePen(const QPen& pen);
    QPen getArrowLinePen() const;

    // 设置箭头大小
    void setArrowSize(qreal size);
    qreal getArrowSize() const;

    // 箭头起始箭头类型
    void setStartEndType(QwtPlotArrowMarker::EndpointStyle type);
    QwtPlotArrowMarker::EndpointStyle getStartEndType() const;

    // 箭头结束箭头类型
    void setEndEndType(QwtPlotArrowMarker::EndpointStyle type);
    QwtPlotArrowMarker::EndpointStyle getEndEndType() const;

    QwtPlotItem* takeItem() override;

protected:
    // 创建图表项
    virtual QwtPlotItem* createPlotItem(const QPointF& startPoint, const QPointF& endPoint);

    // 更新预览项
    virtual void updatePreview(const QVector< QPointF >& points) override;

private:
    // 创建箭头标记项
    QwtPlotArrowMarker* createArrowMarker(const QPointF& startPoint, const QPointF& endPoint);
};


}  // End Of Namespace DA
#endif  // DACHARTARROWEDITOR_H