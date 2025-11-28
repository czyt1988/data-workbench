#ifndef DACHARTWIDGET_H
#define DACHARTWIDGET_H
#if 0
#include "DAFigureAPI.h"
// qt
#include <QMap>
#include <QPointF>
#include <QEvent>
#include <QtGlobal>
#include <QRectF>
#include <QList>
#include <qwt_grid_raster_data.h>
// qwt
#include "qwt_plot.h"
// qwt-items
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_spectrogram.h"

#include "qwt_legend.h"
#include "qwt_point_data.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_magnifier.h"
#include "qwt_text.h"
#include "qwt_symbol.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_canvas_zoomer.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
#include "qwt_symbol.h"
#include "qwt_date.h"
#include "qwt_plot_legenditem.h"
#include "qwt_legend.h"
#include "qwt_date.h"
#include "qwt_plot_histogram.h"
// DAFigure
#include "MarkSymbol/DATriangleMarkSymbol.h"

class QPaintEvent;
class QwtDateScaleDraw;
class QwtPlotPicker;
class QwtPlotSeriesDataPicker;

namespace DA
{
class DAFigureWidget;
class _DAChartScrollZoomerScrollData;

/**
 * @brief 2d绘图
 */
class DAFIGURE_API DAChartWidget : public QwtPlot
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartWidget)
public:
    DAChartWidget(QWidget* parent = nullptr);
    virtual ~DAChartWidget();
    // 获取所有曲线
    QList< QwtPlotCurve* > getCurveList();
    // 获取所有标记
    QList< QwtPlotMarker* > getMakerList();
    // 设置为时间坐标轴
    QwtDateScaleDraw* setAxisDateTimeScale(const QString& format, int axisID, QwtDate::IntervalType intType = QwtDate::Second);
    QwtDateScaleDraw* setAxisDateTimeScale(int axisID);
    // 坐标的极值
    double axisXmin(int axisId = QwtPlot::xBottom) const;
    double axisXmax(int axisId = QwtPlot::xBottom) const;
    double axisYmin(int axisId = QwtPlot::yLeft) const;
    double axisYmax(int axisId = QwtPlot::yLeft) const;
    // 清除所有editor，如zoom，panner，cross等
    virtual void setEnableAllEditor(bool enable);
    // 获取背景
    QBrush getChartBackBrush() const;
    // 获取边框颜色
    QColor getChartBorderColor() const;
    // 添加样条线
    QwtPlotMarker* addVLine(double val, bool representedOnLegend = false);
    QwtPlotMarker* addHLine(double val, bool representedOnLegend = false);
    QwtPlotMarker* addCrossLine(double x, double y, bool representedOnLegend = false);
    // 添加曲线(Line)
    QwtPlotCurve* addCurve(const double* xData, const double* yData, int size);
    QwtPlotCurve* addCurve(const QVector< QPointF >& xyDatas);
    QwtPlotCurve* addCurve(const QVector< double >& xData, const QVector< double >& yData);
    // 绘制散点图(dot)
    QwtPlotCurve* addScatter(const double* xData, const double* yData, int size);
    QwtPlotCurve* addScatter(const QVector< QPointF >& xyDatas);
    QwtPlotCurve* addScatter(const QVector< double >& xData, const QVector< double >& yData);
    // 绘制误差图
    QwtPlotIntervalCurve* addIntervalCurve(const QVector< QwtIntervalSample >& invDatas);
    QwtPlotIntervalCurve*
    addIntervalCurve(const QVector< double >& value, const QVector< double >& min, const QVector< double >& max);
    // 添加bar
    QwtPlotBarChart* addBar(const QVector< QPointF >& xyDatas);
    // 此时x为0~n均匀分布
    QwtPlotBarChart* addBar(const QVector< double >& yDatas);
    // 绘制谱图
    QwtPlotSpectrogram* addSpectroGram(QwtGridRasterData* gridData);
    // 设置所有坐标轴的Margin
    void setAllAxisWidgetMargin(int m);
    // 获取figure
    DAFigureWidget* getFigure() const;
    // title的另外一种方式
    QString getChartTitle() const;

    //==============================================================
    // zoomer
    //==============================================================
    bool isZoomerEnabled() const;
    // 缩放相关
    QwtPlotCanvasZoomer* getZoomer() const;
    // 构建默认的缩放器
    void setupZoomer();

    //==============================================================
    // 网格
    //==============================================================
    // 获取grid指针
    QwtPlotGrid* getGrid() const;
    // 设置或创建网格，并配置其主要线条的样式
    void setGridLine(const QColor& color = Qt::gray, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine, bool isMajor = true);
    // 检查网格是否可见
    bool isGridEnable() const;
    // 检查X轴主网格线是否启用
    bool isGridXEnable() const;
    // 检查Y轴主网格线是否启用
    bool isGridYEnable() const;
    // 检查X轴次要网格线是否启用
    bool isGridXMinEnable() const;
    // 检查Y轴次要网格线是否启用
    bool isGridYMinEnable() const;

    //==============================================================
    // 图例
    //==============================================================
    // 获取legend
    QwtPlotLegendItem* getLegend() const;
    // 检查legend是否可见
    bool isLegendEnable() const;

    //==============================================================
    // Panner
    //==============================================================
    bool isPannerEnable() const;
    // 获取panner
    QwtPlotPanner* getPanner() const;

    //==============================================================
    // LegendPanel
    //==============================================================
    bool isEnableLegendPanel() const;

    //==============================================================
    // Picker
    //==============================================================
    // 是否允许十字光标
    bool isCrossPickerEnable() const;
    bool isYDataPickerEnable() const;
    bool isXYDataPickerEnable() const;

    //==============================================================
    // Magnifier
    //==============================================================
    bool isMagnifierEnable() const;
public Q_SLOTS:
    // 设置边框
    void setChartBorderColor(const QColor& c);
    // 设置背景
    void setChartBackgroundBrush(const QBrush& b);
    //==============================================================
    // zoomer
    //==============================================================
    // 缩放和enablePan是互斥关系，enableZoomer(true)会调用enablePan(false)
    void setZoomerEnable(bool enable = true);
    // 回到放大的最底栈
    void setZoomBase();
    // 放大1.6 相当于乘以0.625
    void zoomIn();
    // 缩小1.6 相当于乘以1.6
    void zoomOut();
    //==============================================================
    // 网格
    //==============================================================
    // 设置grid
    void setGridEnable(bool enabled = true);
    // 启用或禁用X轴主网格线
    void setGridXEnable(bool enabled = true);
    // 启用或禁用Y轴主网格线
    void setGridYEnable(bool enabled = true);
    // 启用或禁用X轴次要网格线
    void setGridXMinEnable(bool enabled = true);
    // 启用或禁用Y轴次要网格线
    void setGridYMinEnable(bool enabled = true);
    //==============================================================
    // Panner
    //==============================================================
    // 拖动,拖动和缩放是互斥关系，enablePan(true)内部会调用enableZoomer(false)
    void setPanEnable(bool enable = true);

    //==============================================================
    // 图例
    //==============================================================
    // 设置legend开启
    void setLegendEnable(bool enable = true);

    //==============================================================
    // Picker
    //==============================================================
    void setYDataPickerEnable(bool enable = true);
    void setXYDataPickerEnable(bool enable = true);
    // 十字线
    void setCrossPickerEnable(bool enable = true);

    //==============================================================
    // LegendPanel
    //==============================================================
    // legend面板
    void setLegendPanelEnable(bool enable = true);

    void showItem(const QVariant& itemInfo, bool on);
    // 设置xbottom-label
    void setXLabelText(const QString& label);
    void setYLabelText(const QString& label);
    // 此函数激活chartPropertyHasChanged信号
    void notifyChartPropertyHasChanged();
Q_SIGNALS:
    /**
     * @brief 通过此类设置网格的任何属性（可见性、轴线启用状态等）发生改变时，会发射此信号。
     *
     * 观察者可以连接此信号，然后通过调用isGridVisible()、isGridXEnabled()等函数来获取最新的网格状态。
     *
     * @note 必须通过DAChartWidget设置网格属性才会触发
     */
    void gridSettingsChanged(QwtPlotGrid* grid);

    /**
     * @brief 当legend的任何属性（可见性、轴线启用状态等）发生改变时，会发射此信号。
     * @param legned
     */
    void legendSettingChanged(QwtPlotLegendItem* legned);

    void enableZoomerChanged(bool enable);
    void enableCrossPickerChanged(bool enable);
    void enablePannerChanged(bool enable);
    void enableLegendChanged(bool enable);
    void enableLegendPanelChanged(bool enable);
    void enableYDataPickerChanged(bool enable);
    void enableXYDataPickerChanged(bool enable);
    /**
     * @brief 这是一个由用户激发的信号，此信号由notifyChartPropertyHasChanged槽函数触发
     * 这个，ui界面可以绑定这个信号，在一些没有信号响应的属性设置完成后，通过notifyChartPropertyHasChanged槽函数通知界面
     * 刷新绘图的设置，这个信号在动态显示的绘图设置窗口非常有用
     */
    void chartPropertyHasChanged(DAChartWidget* chart);

public:
    // 把min,max,value的数组转换为QwtIntervalSample数组
    static void makeIntervalSample(const QVector< double >& value,
                                   const QVector< double >& min,
                                   const QVector< double >& max,
                                   QVector< QwtIntervalSample >& invDatas);

public:
    //==============================================================
    // 工厂类相关
    //==============================================================
    using FpCreatePanner           = std::function< QwtPlotPanner*(QWidget*) >;
    using FpCreatePicker           = std::function< QwtPlotPicker*(QWidget*) >;
    using FpCreateSeriesDataPicker = std::function< QwtPlotSeriesDataPicker*(QWidget*) >;
    // 注册panner的工厂
    void registerPannerFactory(const FpCreatePanner& fp);
    // 注册picker工厂
    void registerPickerFactory(const FpCreatePicker& fp);
    // 注册QwtPlotSeriesDataPicker工厂
    void registerSeriesDataPickerFactory(const FpCreateSeriesDataPicker& fp);

protected:
    virtual void resizeEvent(QResizeEvent*) override;

protected:
    void paintEvent(QPaintEvent* e) override;

protected:
    void deleteGrid();
    void setZoomerEnable(QwtPlotCanvasZoomer* zoomer, bool enable);

    // 建立一个内置的picker(十字)
    void setupCrossPicker();
    // 建立一个内置的Panner(拖动)，默认使用鼠标中键
    void setupPanner();
    // 建立Magnifier
    void setupMagnifier();

    void setuplegendPanel();
    // 创建一个SeriesDataPicker，如果原来已经有，会把原来删除掉，再创建一个
    void setupSeriesDataPicker();

private:
    QwtPlotSeriesDataPicker* getOrCreateSeriesDataPicker();
    QwtPlotGrid* getOrCreateGrid();
    QwtPlotLegendItem* getOrCreateLegend();
};

}  // End Of Namespace DA

#else

#include "DAFigureAPI.h"
#include "DAChartDataInterface.h"
#include "DAChartStyleInterface.h"
#include "DAChartInteractionInterface.h"

#include <QWidget>
#include <memory>

// QWT includes
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_spectrogram.h"

class QwtDateScaleDraw;


namespace DA
{
class DAFigureWidget;

/**
 * @brief 增强的Qwt图表控件，提供完整的数据绘图、样式设置和交互控制功能
 *
 * 通过三个接口分离不同方面的功能：
 * - DAChartDataInterface: 数据管理和图表添加
 * - DAChartStyleInterface: 样式设置和显示配置
 * - DAChartInteractionInterface: 用户交互控制
 */
class DAFIGURE_API DAChartWidget : public QwtPlot,
                                   public DAChartDataInterface,
                                   public DAChartStyleInterface,
                                   public DAChartInteractionInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartWidget)
public:
    /**
     * @brief The 图表属性
     */
    enum ChartPropertyChangeFlag
    {
        NoChange           = 0x0,
        ChartTitleChanged  = 0x1,
        BackgroundChanged  = 0x2,
        BorderColorChanged = 0x4,
        // Axis
        AxisLabelChanged = 0x10,
        AxisColorChanged = 0x20,
        AxisScaleChanged = 0x40,
        // Grid & Legend
        GridEnabledChanged = 0x100,
        GridStyleChanged   = 0x200,  ///< grid的样式发生变化，包括：@ref
                                   ///< enableGridX(),enableGridY(),enableGridXMin(),enableGridYMin(),setGridStyle(),setGridMajorStyle(),setGridMinorStyle()
                                   ///< 都会触发此信号
        LegendEnabledChanged      = 0x400,
        LegendPositionChanged     = 0x800,
        LegendBackgroundChanged   = 0x1000,
        LegendTextColorChanged    = 0x2000,
        LegendPanelEnabledChanged = 0x8000,  ///< 代表LegendPanel开启或关闭，@ref isLegendPanelEnabled
        // scale
        DateTimeScaleSetup = 0x10000,
        // Interaction
        ZoomStateChanged      = 0x100000,  ///< 代表zoomer开启或关闭，@ref isZoomEnabled
        PanStateChanged       = 0x200000,  ///< 代表pan开启或关闭，@ref isPanEnabled
        CrosshairStateChanged = 0x400000,  ///< 代表Crosshair开启或关闭，@ref isCrosshairEnabled
        DataPickingStateChanged =
            0x800000,  ///< 代表picking开启或关闭，@ref isYValuePickingEnabled 和 @ref isXYValuePickingEnabled
        MouseWheelZoomStateChanged = 0x1000000,  ///< 代表magnifier开启或关闭，@ref isMouseWheelZoomEnabled
    };

    Q_DECLARE_FLAGS(ChartPropertyChangeFlags, ChartPropertyChangeFlag)
public:
    explicit DAChartWidget(QWidget* parent = nullptr);
    virtual ~DAChartWidget();

    // ==================== DAChartDataInterface 实现 ====================
    // 获取所有数据相关的rtti
    virtual QList< int > dataRttis() const override;
    // 曲线操作
    QwtPlotCurve* addCurve(const QVector< double >& xData,
                           const QVector< double >& yData,
                           const QString& title = QString()) override;
    QwtPlotCurve* addCurve(const QVector< QPointF >& points, const QString& title = QString()) override;
    QList< QwtPlotCurve* > getCurves() const override;
    void removeCurve(QwtPlotCurve* curve) override;

    // 散点图
    QwtPlotCurve* addScatter(const QVector< QPointF >& points, const QString& title = QString()) override;

    // 柱状图
    QwtPlotBarChart* addBarChart(const QVector< double >& values, const QString& title = QString()) override;
    QwtPlotBarChart* addBarChart(const QVector< QPointF >& points, const QString& title = QString()) override;

    // 误差图
    QwtPlotIntervalCurve* addIntervalCurve(const QVector< double >& values,
                                           const QVector< double >& mins,
                                           const QVector< double >& maxs,
                                           const QString& title = QString()) override;

    // 标记线
    QwtPlotMarker* addVerticalLine(double x, const QString& title = QString()) override;
    QwtPlotMarker* addHorizontalLine(double y, const QString& title = QString()) override;
    QwtPlotMarker* addCrossLine(double x, double y, const QString& title = QString()) override;

    // 高级图表
    QwtPlotSpectrogram* addSpectrogram(QwtGridRasterData* gridData, const QString& title = QString()) override;

    // 通用数据操作
    void removePlotItem(QwtPlotItem* item) override;
    // 删除所有数据相关的项目（保留网格、图例等显示元素）
    void clearAllData() override;

    // 数据工具函数
    QRectF getDataBounds() const override;
    bool hasData() const override;

    // ==================== DAChartStyleInterface 实现 ====================
    // 图表整体样式
    void setChartTitle(const QString& title) override;
    QString getChartTitle() const override;

    void setBackgroundBrush(const QBrush& brush) override;
    QBrush getBackgroundBrush() const override;

    void setBorderColor(const QColor& color) override;
    QColor getBorderColor() const override;

    // 坐标轴样式
    void setAxisLabel(int axisId, const QString& label) override;
    QString getAxisLabel(int axisId) const override;

    void setAxisColor(int axisId, const QColor& color) override;
    QColor getAxisColor(int axisId) const override;

    // 网格样式
    void enableGrid(bool enable = true) override;
    void enableGridX(bool enable = true);
    void enableGridY(bool enable = true);
    void enableGridXMin(bool enable = true);
    void enableGridYMin(bool enable = true);
    bool isGridEnabled() const override;
    bool isGridXEnabled() const;
    bool isGridYEnabled() const;
    bool isGridXMinEnabled() const;
    bool isGridYMinEnabled() const;

    void setGridStyle(const QColor& color, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine, bool isMajor = true) override;

    void setGridMajorStyle(const QColor& color, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine) override;

    void setGridMinorStyle(const QColor& color, qreal width = 0.5, Qt::PenStyle style = Qt::DotLine) override;

    // 图例样式
    void enableLegend(bool enable = true) override;
    bool isLegendEnabled() const override;

    void setLegendPosition(Qt::Alignment alignment) override;
    Qt::Alignment getLegendPosition() const override;

    void setLegendBackground(const QBrush& brush) override;
    QBrush getLegendBackground() const override;

    void setLegendTextColor(const QColor& color) override;
    QColor getLegendTextColor() const override;

    // 时间坐标轴
    void setupDateTimeAxis(int axisId, const QString& format = "yyyy-MM-dd hh:mm:ss") override;
    bool isDateTimeAxis(int axisId) const override;

    // ==================== DAChartInteractionInterface 实现 ====================
    // 缩放控制
    void enableZoom(bool enable = true) override;
    bool isZoomEnabled() const override;

    void zoomToOriginal() override;
    void zoomIn() override;
    void zoomOut() override;


    QwtPlotCanvasZoomer* getZoomer() const override;

    // 平移控制
    void enablePan(bool enable = true) override;
    bool isPanEnabled() const override;

    QwtPlotPanner* getPanner() const override;

    // 十字线控制
    void enableCrosshair(bool enable = true) override;
    bool isCrosshairEnabled() const override;

    QwtPlotPicker* getCrosshair() const override;

    // 数据拾取控制
    bool isDataPickingEnabled() const;

    void enableYValuePicking(bool enable = true) override;
    bool isYValuePickingEnabled() const override;

    void enableXYValuePicking(bool enable = true) override;
    bool isXYValuePickingEnabled() const override;

    QwtPlotSeriesDataPicker* getDataPicker() const override;

    // 鼠标滚轮控制
    void enableMouseWheelZoom(bool enable = true) override;
    bool isMouseWheelZoomEnabled() const override;

    QwtPlotMagnifier* getMagnifier() const override;

    // 图例面板控制
    void enableLegendPanel(bool enable = true) override;
    bool isLegendPanelEnabled() const override;

    QwtLegend* getLegendPanel() const override;


    // 工厂函数注册
    void registerPannerFactory(const PannerFactory& factory) override;
    void registerPickerFactory(const PickerFactory& factory) override;
    void registerDataPickerFactory(const DataPickerFactory& factory) override;

    // ==================== 工具函数 ====================
    DAFigureWidget* getFigure() const;

    void notifyPropertiesChanged(ChartPropertyChangeFlags flag);

public Q_SLOTS:
    void onLegendItemToggled(const QVariant& itemInfo, bool checked);

Q_SIGNALS:
    // 通用信号
    void chartPropertiesChanged(DAChartWidget* chart, ChartPropertyChangeFlags flag);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // 初始化函数
    void initializeChart();
    void setupCanvas();
    void initializeInteractions();

    // 组件管理
    void setupZoomer();
    void setupPanner();
    void setupMagnifier();
    void setupCrosshair();
    void setupDataPicker();
    void setupLegendPanel();

    // 显示元素管理
    QwtPlotGrid* getOrCreateGrid();
    QwtPlotLegendItem* getOrCreateLegend();
};

}  // namespace DA

#endif

#endif  // DACHARTWIDGET_H
