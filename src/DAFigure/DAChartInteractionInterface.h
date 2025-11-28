#ifndef DACHARTINTERACTIONINTERFACE_H
#define DACHARTINTERACTIONINTERFACE_H
#include "DAFigureAPI.h"
// stl
#include <functional>
// qt
#include <QRectF>
#include <QWidget>
class QwtPlotCanvasZoomer;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotMagnifier;
class QwtPlotSeriesDataPicker;
class QwtLegend;

namespace DA
{

/**
 * @brief 图表交互控制接口 - 负责缩放、平移、拾取等交互功能
 */
class DAFIGURE_API DAChartInteractionInterface
{
public:
    virtual ~DAChartInteractionInterface() = default;

    // ==================== 缩放控制 ====================
    virtual void enableZoom(bool enable = true) = 0;
    virtual bool isZoomEnabled() const          = 0;

    virtual void zoomToOriginal() = 0;
    virtual void zoomIn()         = 0;
    virtual void zoomOut()        = 0;

    virtual QwtPlotCanvasZoomer* getZoomer() const = 0;

    // ==================== 平移控制 ====================
    virtual void enablePan(bool enable = true) = 0;
    virtual bool isPanEnabled() const          = 0;

    virtual QwtPlotPanner* getPanner() const = 0;

    // ==================== 十字线控制 ====================
    virtual void enableCrosshair(bool enable = true) = 0;
    virtual bool isCrosshairEnabled() const          = 0;

    virtual QwtPlotPicker* getCrosshair() const = 0;

    // ==================== 数据拾取控制 ====================


    virtual void enableYValuePicking(bool enable = true) = 0;
    virtual bool isYValuePickingEnabled() const          = 0;

    virtual void enableXYValuePicking(bool enable = true) = 0;
    virtual bool isXYValuePickingEnabled() const          = 0;

    virtual QwtPlotSeriesDataPicker* getDataPicker() const = 0;

    // ==================== 鼠标滚轮控制 ====================
    virtual void enableMouseWheelZoom(bool enable = true) = 0;
    virtual bool isMouseWheelZoomEnabled() const          = 0;

    virtual QwtPlotMagnifier* getMagnifier() const = 0;

    // ==================== 图例面板控制 ====================
    virtual void enableLegendPanel(bool enable = true) = 0;
    virtual bool isLegendPanelEnabled() const          = 0;

    virtual QwtLegend* getLegendPanel() const = 0;


    // ==================== 工厂函数注册 ====================
    using PannerFactory     = std::function< QwtPlotPanner*(QWidget*) >;
    using PickerFactory     = std::function< QwtPlotPicker*(QWidget*) >;
    using DataPickerFactory = std::function< QwtPlotSeriesDataPicker*(QWidget*) >;

    virtual void registerPannerFactory(const PannerFactory& factory)         = 0;
    virtual void registerPickerFactory(const PickerFactory& factory)         = 0;
    virtual void registerDataPickerFactory(const DataPickerFactory& factory) = 0;
};

}  // namespace DA
#endif  // DACHARTINTERACTIONINTERFACE_H
