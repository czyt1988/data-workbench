#ifndef DACHARTWIDGET_H
#define DACHARTWIDGET_H


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
        GridStyleChanged =
            0x200,  ///< grid的样式发生变化，包括：@ref
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
    Q_ENUM(ChartPropertyChangeFlag)
    Q_DECLARE_FLAGS(ChartPropertyChangeFlags, ChartPropertyChangeFlag)
public:
    explicit DAChartWidget(QWidget* parent = nullptr);
    virtual ~DAChartWidget();

    // ==================== DAChartDataInterface 实现 ====================
    // 获取所有数据相关的rtti
    virtual QList< int > dataRttis() const override;
    // 曲线操作
    QwtPlotCurve* addCurve(const QVector< double >& xData, const QVector< double >& yData, const QString& title = QString()) override;
    QwtPlotCurve* addCurve(const QVector< QPointF >& points, const QString& title = QString()) override;
    QList< QwtPlotCurve* > getCurves() const override;
    void removeCurve(QwtPlotCurve* curve) override;

    // 散点图
    QwtPlotCurve* addScatter(const QVector< QPointF >& points, const QString& title = QString()) override;

    // 柱状图
    QwtPlotBarChart* addBarChart(const QVector< double >& values, const QString& title = QString()) override;
    QwtPlotBarChart* addBarChart(const QVector< QPointF >& points, const QString& title = QString()) override;

    // 误差图
    QwtPlotIntervalCurve* addIntervalCurve(
        const QVector< double >& values,
        const QVector< double >& mins,
        const QVector< double >& maxs,
        const QString& title = QString()
    ) override;

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
    void chartPropertiesChanged(DA::DAChartWidget* chart, DA::DAChartWidget::ChartPropertyChangeFlags flag);

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


#endif  // DACHARTWIDGET_H
