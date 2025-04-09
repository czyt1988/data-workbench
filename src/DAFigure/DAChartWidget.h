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
// qwt
#include "qwt_plot.h"
// qwt-items
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_barchart.h"

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
#include "DAChartScrollZoomer.h"
class QPaintEvent;
class QwtDateScaleDraw;

namespace DA
{
class DAFigureWidget;
class _DAChartScrollZoomerScrollData;
class DAChartYDataPicker;
class DAChartXYDataPicker;
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
	///
	/// \brief getCureList 获取所有曲线
	/// \return
	///
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
	QwtPlotIntervalCurve* addIntervalCurve(const QVector< double >& value,
                                           const QVector< double >& min,
                                           const QVector< double >& max);
	// 添加bar
    QwtPlotBarChart* addBar(const QVector< QPointF >& xyDatas);
	// 此时x为0~n均匀分布
    QwtPlotBarChart* addBar(const QVector< double >& yDatas);
	// 设置所有坐标轴的Margin
	void setAllAxisMargin(int m);
	// 获取figure
	DAFigureWidget* getFigure() const;
	// title的另外一种方式
	QString getChartTitle() const;

public:
	// 这里获取绘图窗口里的一些部件
	//==============================================================
	//  缩放相关
	//==============================================================
	// 缩放相关
	QwtPlotZoomer* getZoomer() const;
	// 构建默认的缩放器
	void setupZoomer();
	void setupZoomer(QwtPlotZoomer* z, bool issecondZoom = false);
	QwtPlotZoomer* getZoomerSecond();
	//==============================================================
	// 网格
	//==============================================================
	QwtPlotGrid* getGrid() const;
	QwtPlotGrid* setupGrid(const QColor& color = Qt::gray, qreal width = 1.0, Qt::PenStyle style = Qt::DotLine);
	void setupGrid(QwtPlotGrid* g);
	//==============================================================
	// 图例
	//==============================================================
	// 获取legend
	QwtPlotLegendItem* getLegend() const;
public slots:
	// 设置边框
	void setChartBorderColor(const QColor& c);
	// 设置背景
	void setChartBackgroundBrush(const QBrush& b);

	// 缩放和enablePan是互斥关系，enableZoomer(true)会调用enablePan(false)
	void enableZoomer(bool enable = true);

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

	void enableCrossPicker(bool enable = true);
	void enableGrid(bool enable = true);
	void enableGridX(bool enable = true);
	void enableGridY(bool enable = true);
	void enableGridXMin(bool enable = true);
	void enableGridYMin(bool enable = true);
	// 拖动,拖动和缩放是互斥关系，enablePan(true)内部会调用enableZoomer(false)
	void enablePan(bool enable = true);

	void enableLegend(bool enable = true);
	void enableLegendPanel(bool enable = true);

	void markYValue(double data, const QString& strLabel, QColor clr = Qt::black);

	void showItem(const QVariant& itemInfo, bool on);

	void enableYDataPicker(bool enable = true);

	void enableXYDataPicker(bool enable = true);
	// 设置xbottom-label
	void setXLabel(const QString& label);
	void setYLabel(const QString& label);
	// 此函数激活chartPropertyHasChanged信号
	void notifyChartPropertyHasChanged();
signals:
	void enableZoomerChanged(bool enable);
	void enableCrossPickerChanged(bool enable);
	void enableGridChanged(bool enable);
	void enableGridXChanged(bool enable);
	void enableGridYChanged(bool enable);
	void enableGridXMinChanged(bool enable);
	void enableGridYMinChanged(bool enable);
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
	bool isEnableZoomer() const;

	// 是否允许十字光标
	bool isEnableCrossPicker() const;
	bool isEnableGrid() const;
	bool isEnableGridX() const;
	bool isEnableGridY() const;
	bool isEnableGridXMin() const;
	bool isEnableGridYMin() const;
	bool isEnablePanner() const;
	bool isEnableLegend() const;
	bool isEnableLegendPanel() const;
	bool isEnableYDataPicker() const;
	bool isEnableXYDataPicker() const;

public:
	// 把min,max,value的数组转换为QwtIntervalSample数组
	static void makeIntervalSample(const QVector< double >& value,
                                   const QVector< double >& min,
                                   const QVector< double >& max,
                                   QVector< QwtIntervalSample >& invDatas);

protected:
	virtual void resizeEvent(QResizeEvent*);

protected:
	void paintEvent(QPaintEvent* e) override;

protected:
	void deleteGrid();
	void deleteZoomer();
	void enableZoomer(QwtPlotZoomer* zoomer, bool enable);

	// 建立一个内置的picker(十字)
	void setupCrossPicker();
	// 建立一个内置的Panner(拖动)，默认使用鼠标中键
	void setupPanner();
	void deletePanner();

	///
	/// \brief 建立一个图例r
	///
	void setupLegend();
	void setuplegendPanel();
	void deletelegendPanel();

	void setupYDataPicker();
	void deleteYDataPicker();

	void setupXYDataPicker();
	void deleteXYDataPicker();
};

/*
 *
 *
 * enum QwtPlotItem::RttiValues
 *
 *
 *
 * Runtime type information.
 *
 * RttiValues is used to cast plot items, without having to enable runtime type information of the compiler.
 *
 *
 *
 * Enumerator
 *
 *
 *
 * Rtti_PlotItem
 *
 *
 * Unspecific value, that can be used, when it doesn't matter.
 *
 *
 *
 * Rtti_PlotGrid
 *
 *
 * For QwtPlotGrid.
 *
 *
 *
 * Rtti_PlotScale
 *
 *
 * For QwtPlotScaleItem.
 *
 *
 *
 * Rtti_PlotLegend
 *
 *
 * For QwtPlotLegendItem.
 *
 *
 *
 * Rtti_PlotMarker
 *
 *
 * For QwtPlotMarker.
 *
 *
 *
 * Rtti_PlotCurve
 *
 *
 * For QwtPlotCurve.
 *
 *
 *
 * Rtti_PlotSpectroCurve
 *
 *
 * For QwtPlotSpectroCurve.
 *
 *
 *
 * Rtti_PlotIntervalCurve
 *
 *
 * For QwtPlotIntervalCurve.
 *
 *
 *
 * Rtti_PlotHistogram
 *
 *
 * For QwtPlotHistogram.
 *
 *
 *
 * Rtti_PlotSpectrogram
 *
 *
 * For QwtPlotSpectrogram.
 *
 *
 *
 * Rtti_PlotSVG
 *
 *
 * For QwtPlotSvgItem.
 *
 *
 *
 * Rtti_PlotTradingCurve
 *
 *
 * For QwtPlotTradingCurve.
 *
 *
 *
 * Rtti_PlotBarChart
 *
 *
 * For QwtPlotBarChart.
 *
 *
 *
 * Rtti_PlotMultiBarChart
 *
 *
 * For QwtPlotMultiBarChart.
 *
 *
 *
 * Rtti_PlotShape
 *
 *
 * For QwtPlotShapeItem.
 *
 *
 *
 * Rtti_PlotTextLabel
 *
 *
 * For QwtPlotTextLabel.
 *
 *
 *
 * Rtti_PlotZone
 *
 *
 * For QwtPlotZoneItem.
 *
 *
 *
 * Rtti_PlotUserItem
 *
 *
 * Values >= Rtti_PlotUserItem are reserved for plot items not implemented in the Qwt library.
 *
 *
 */
}  // End Of Namespace DA
#endif  // DACHARTWIDGET_H
