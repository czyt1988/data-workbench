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
 *  用于创建图表项，主要在点击位置创建一个QwtPlotItem对象
 */
class DAFIGURE_API DAChartItemCreatInteractor : public DAAbstractChartEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartItemCreatInteractor)
public:
    using FpCreatePlotItem = std::function< QwtPlotItem*(QwtPlot*, const QPointF&) >;

public:
    explicit DAChartItemCreatInteractor(QwtPlot* parent = nullptr);
    virtual ~DAChartItemCreatInteractor();
    // 设置创建图表项的工厂
    void setPlotItemInteractorFactory(FpCreatePlotItem fun);
    FpCreatePlotItem getPlotItemInteractorFactory() const;

protected:
    virtual bool mousePressEvent(const QMouseEvent* e);
};

}

#endif  // __DACHARTITEMCREATINTERACTOR_H__