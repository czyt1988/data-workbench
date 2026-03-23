#ifndef __DACHARTITEMCREATINTERACTOR_H__
#define __DACHARTITEMCREATINTERACTOR_H__
#include "DAAbstractChartEditor.h"
#include <functional>
class QwtPlot;
class QMouseEvent;
namespace DA
{
/**
 *  @brief 创建图表项交互器
 *
 *  用于创建图表项，主要在点击位置创建一个QwtPlotItem对象
 */
class DAFIGURE_API DAChartItemCreatInteractor : public DAAbstractChartEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartItemCreatInteractor)
public:
    /**
     * @brief 创建图表项工厂
     *
     * 用于创建图表项，主要在点击位置创建一个QwtPlotItem对象
     *
     * - 参数1 plot 图表
     * - 参数2 pos 绘图位置，此位置不是屏幕坐标，而是图表坐标
     * - 返回 QwtPlotItem* 图表项
     */
    using FpCreatePlotItem = std::function< QwtPlotItem*(QwtPlot*, const QPointF&) >;

public:
    explicit DAChartItemCreatInteractor(QwtPlot* parent = nullptr, FpCreatePlotItem fun = nullptr);
    virtual ~DAChartItemCreatInteractor();
    // 设置创建图表项的工厂
    void setPlotItemInteractorFactory(FpCreatePlotItem fun);
    FpCreatePlotItem getPlotItemInteractorFactory() const;
    // 重写DAAbstractChartEditor的接口
    virtual int rtti() const override;
    // 获取选框绘制的item
    virtual QwtPlotItem* takeItem() override;

protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
};


// 提供默认的几个创建图表项的工厂

/**
 * @brief 在pos位置创建水平线标记
 *
 * @param plot 图表
 * @param pos 位置
 * @return QwtPlotMarker* 图表项
 */
DAFIGURE_API QwtPlotItem* createHLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos);
/**
 * @brief 在pos位置创建垂直垂直线标记
 *
 * @param plot 图表
 * @param pos 位置
 * @return QwtPlotMarker* 图表项
 */
DAFIGURE_API QwtPlotItem* createVLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos);

/**
 * @brief 在pos位置创建十字线标记
 *
 * @param plot 图表
 * @param pos 位置
 * @return QwtPlotMarker* 图表项
 */
DAFIGURE_API QwtPlotItem* createCrossLineMarkerPlotItem(QwtPlot* plot, const QPointF& pos);


}  // namespace DA

#endif  // __DACHARTITEMCREATINTERACTOR_H__