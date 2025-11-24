#ifndef DACHARTWIDGET_H
#define DACHARTWIDGET_H
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
#include "qwt_plot_zoomer.h"
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
    QwtPlotZoomer* getZoomer() const;
    // 构建默认的缩放器
    void setupZoomer();
    void setupZoomer(QwtPlotZoomer* z, bool issecondZoom = false);
    QwtPlotZoomer* getZoomerSecond();

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
    // 重置放大的基准
    void setZoomReset();
    // 放大1.6 相当于乘以0.625
    void zoomIn();
    // 缩小1.6 相当于乘以1.6
    void zoomOut();
    // 缩放到最适合比例，就是可以把所有图都能看清的比例
    void zoomInCompatible();
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
    void deleteZoomer();
    void setZoomerEnable(QwtPlotZoomer* zoomer, bool enable);

    // 建立一个内置的picker(十字)
    void setupCrossPicker();
    // 建立一个内置的Panner(拖动)，默认使用鼠标中键
    void setupPanner();
    void deletePanner();

    void setuplegendPanel();
    void deletelegendPanel();
    // 创建一个SeriesDataPicker，如果原来已经有，会把原来删除掉，再创建一个
    void setupSeriesDataPicker();

private:
    QwtPlotSeriesDataPicker* getOrCreateSeriesDataPicker();
    QwtPlotGrid* getOrCreateGrid();
    QwtPlotLegendItem* getOrCreateLegend();
};

}  // End Of Namespace DA
#endif  // DACHARTWIDGET_H
