#include "DAChartUtil.h"
#include <numeric>
#include <QSet>
#include <QtGlobal>
#include <QtMath>
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "qwt_date_scale_draw.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_rasteritem.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_scale_map.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_math.h"
namespace DA
{

///
/// \brief 更加强制的replot，就算设置为不实时刷新也能实现重绘
/// \param chart
///
void DAChartUtil::replot(QwtPlot* chart)
{
	QwtPlotCanvas* plotCanvas = qobject_cast< QwtPlotCanvas* >(chart->canvas());
	if (plotCanvas) {
		if (!plotCanvas->testPaintAttribute(QwtPlotCanvas::ImmediatePaint)) {
			plotCanvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
			chart->replot();
			plotCanvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, false);
		} else {
			chart->replot();
		}
	} else {
		chart->replot();
	}
}

/**
 * @brief 根据筛选set获取item list
 * @param chart
 * @param enableRtti
 * @return
 */
QwtPlotItemList DAChartUtil::filterPlotItem(const QwtPlot* chart, const QSet< int >& enableRtti)
{
	const QwtPlotItemList& items = chart->itemList();
	QwtPlotItemList res;
	for (int i = 0; i < items.size(); ++i) {
		if (enableRtti.contains(items[ i ]->rtti())) {
			res.append(items[ i ]);
		}
	}
	return res;
}
///
/// \brief 把当前坐标点转换为对应的坐标系的坐标点
/// \param chart
/// \param p
/// \param orgXAxis
/// \param orgYAxis
/// \param otherXAxis
/// \param otherYAxis
/// \return
///
QPointF DAChartUtil::transformValue(QwtPlot* chart, const QPointF& p, int orgXAxis, int orgYAxis, int otherXAxis, int otherYAxis)
{
	double x = p.x(), y = p.y();
	if (orgXAxis == otherXAxis && orgYAxis == otherYAxis) {
		return p;
	}
	QwtScaleDraw* sdx1 = chart->axisScaleDraw(orgXAxis);
	QwtScaleDraw* sdy1 = chart->axisScaleDraw(orgYAxis);
	QwtScaleDraw* sdx2 = chart->axisScaleDraw(otherXAxis);
	QwtScaleDraw* sdy2 = chart->axisScaleDraw(otherYAxis);
	if (sdx1 && sdx2) {
		// 转换
		// 先转到屏幕坐标
		x = sdx1->scaleMap().transform(x);
		x = sdx2->scaleMap().invTransform(x);
	}
	if (sdy1 && sdy2) {
		y = sdy1->scaleMap().transform(y);
		y = sdy2->scaleMap().invTransform(y);
	}
	return QPointF(x, y);
}
///
/// \brief 坐标轴数据互转（把坐标轴转换为另外一个坐标轴数据而保持屏幕位置不变）
/// \param chart
/// \param p
/// \param orgXAxis
/// \param orgYAxis
/// \param otherXAxis
/// \param otherYAxis
/// \return
///
QPainterPath
DAChartUtil::transformPath(QwtPlot* chart, const QPainterPath& p, int orgXAxis, int orgYAxis, int otherXAxis, int otherYAxis)
{
	QPainterPath shape = p;
	const int eleCount = p.elementCount();
	for (int i = 0; i < eleCount; ++i) {
		const QPainterPath::Element& el = p.elementAt(i);
		QPointF tmp = transformValue(chart, QPointF(el.x, el.y), orgXAxis, orgYAxis, otherXAxis, otherYAxis);
		shape.setElementPositionAt(i, tmp.x(), tmp.y());
	}
	return shape;
}
///
/// \brief 计算在屏幕上移动一个像素，在实际数据需要平移的距离
/// \param chart 绘图指针
/// \param xaxis x轴
/// \param yaxis y轴
/// \return
///
QPointF DAChartUtil::calcOnePixelOffset(QwtPlot* chart, int xaxis, int yaxis)
{
	QPoint cen = chart->rect().center();
	double x1  = chart->invTransform(xaxis, cen.x());
	double y1  = chart->invTransform(yaxis, cen.y());
	double x2  = chart->invTransform(xaxis, cen.x() + 1);
	double y2  = chart->invTransform(yaxis, cen.y() + 1);
	return QPointF(x2 - x1, y2 - y1);
}
///
/// \brief 屏幕坐标转到对应的绘图坐标
/// \param chart
/// \param screen 屏幕坐标
/// \param xAxis x轴
/// \param yAxis y轴
/// \return
///
QPointF DAChartUtil::screenPointToPlotPoint(QwtPlot* chart, const QPointF& screen, int xAxis, int yAxis)
{
	QwtScaleDraw* sdx = chart->axisScaleDraw(xAxis);
	QwtScaleDraw* sdy = chart->axisScaleDraw(yAxis);
	if (sdx && sdy) {
		// 转换
		// 先转到屏幕坐标
		return QPointF(sdx->scaleMap().invTransform(screen.x()), sdy->scaleMap().invTransform(screen.y()));
	}
	return QPointF();
}

///
/// \brief 是否允许显示坐标轴
/// \param chart
/// \param axisID
/// \param b
///
void DAChartUtil::setAxisEnable(QwtPlot* chart, int axisID, bool b)
{
	if (chart) {
		chart->enableAxis(axisID, b);
		if (!chart->axisAutoScale(axisID)) {
			chart->setAxisAutoScale(axisID);
		}
	}
}
///
/// \brief 设置坐标轴的标题
/// \param chart
/// \param axisID
/// \param text
///
void DAChartUtil::setAxisTitle(QwtPlot* chart, int axisID, const QString& text)
{
	if (chart) {
		chart->setAxisTitle(axisID, text);
	}
}

///
/// \brief 设置坐标轴文字的字体
/// \param chart
/// \param axisID
/// \param font
///
void DAChartUtil::setAxisFont(QwtPlot* chart, int axisID, const QFont& font)
{
	if (chart) {
		chart->setAxisFont(axisID, font);
	}
}
///
/// \brief 设置坐标轴文字的旋转
/// \param chart
/// \param axisID
/// \param v
///
void DAChartUtil::setAxisLabelRotation(QwtPlot* chart, int axisID, double v)
{
	if (chart) {
		chart->setAxisLabelRotation(axisID, v);
	}
}
///
/// \brief 设置坐标轴最小刻度
/// \param chart
/// \param axisID
/// \param v
///
void DAChartUtil::setAxisScaleMin(QwtPlot* chart, int axisID, double v)
{
	if (chart) {
		QwtInterval inv = chart->axisInterval(axisID);
		chart->setAxisScale(axisID, v, inv.maxValue());
	}
}
///
/// \brief 设置坐标轴最大刻度
/// \param chart
/// \param axisID
/// \param v
///
void DAChartUtil::setAxisScaleMax(QwtPlot* chart, int axisID, double v)
{
	if (chart) {
		QwtInterval inv = chart->axisInterval(axisID);
		chart->setAxisScale(axisID, inv.minValue(), v);
	}
}
///
/// \brief 指定坐标轴端点到窗体的距离-影响坐标轴标题的显示
/// \param chart
/// \param axisID
/// \param v
///
void DAChartUtil::setAxisBorderDistStart(QwtPlot* chart, int axisID, int v)
{
	if (nullptr == chart) {
		return;
	}
	QwtScaleWidget* ax = chart->axisWidget(axisID);
	if (ax) {
		ax->setBorderDist(v, ax->endBorderDist());
	}
}
///
/// \brief 指定坐标轴端点到窗体的距离-影响坐标轴标题的显示
/// \param chart
/// \param axisID
/// \param v
///
void DAChartUtil::setAxisBorderDistEnd(QwtPlot* chart, int axisID, int v)
{
	if (nullptr == chart) {
		return;
	}
	QwtScaleWidget* ax = chart->axisWidget(axisID);
	if (ax) {
		ax->setBorderDist(ax->startBorderDist(), v);
	}
}
///
/// \brief 设置坐标轴和画板的偏移距离
/// \param chart
/// \param axisID 坐标轴
/// \param v 偏移距离
///
void DAChartUtil::setAxisMargin(QwtPlot* chart, int axisID, int v)
{
	if (nullptr == chart) {
		return;
	}
	QwtScaleWidget* ax = chart->axisWidget(axisID);
	if (ax) {
		ax->setMargin(v);
	}
}
///
/// \brief 设置坐标轴的间隔
/// \param chart
/// \param axisID 坐标轴
/// \param v 间隔
///
void DAChartUtil::setAxisSpacing(QwtPlot* chart, int axisID, int v)
{
	if (nullptr == chart) {
		return;
	}
	QwtScaleWidget* ax = chart->axisWidget(axisID);
	if (ax) {
		ax->setSpacing(v);
	}
}
///
/// \brief 设置坐标轴文字的对齐方式
/// \param chart
/// \param axisID 坐标轴
/// \param v 对齐方式
///
void DAChartUtil::setAxisLabelAlignment(QwtPlot* chart, int axisID, Qt::Alignment v)
{
	if (nullptr == chart) {
		return;
	}
	QwtScaleWidget* ax = chart->axisWidget(axisID);
	if (ax) {
		ax->setLabelAlignment(v);
	}
}
///
/// \brief 设置坐标轴为时间坐标
/// \param chart
/// \param axisID 坐标轴id
/// \param format 时间格式
/// \param type 间隔类型
/// \return
///
QwtDateScaleDraw* DAChartUtil::setAxisDateTimeScale(QwtPlot* chart, int axisID, const QString& format, QwtDate::IntervalType type)
{
	if (nullptr == chart) {
		return nullptr;
	}
	QwtDateScaleDraw* dateScale;
	dateScale = new QwtDateScaleDraw;  // 原来的scaleDraw会再qwt自动delete
	dateScale->setDateFormat(type, format);
	chart->setAxisScaleDraw(axisID, dateScale);

	QwtDateScaleEngine* scaleEngine = dynamic_cast< QwtDateScaleEngine* >(chart->axisScaleEngine(axisID));
	if (nullptr == scaleEngine) {
		scaleEngine = new QwtDateScaleEngine;
		chart->setAxisScaleEngine(axisID, scaleEngine);
	}
	return dateScale;
}
///
/// \brief 获取时间坐标轴，若当前不是时间坐标轴，返回nullptr,可以用来判断是否为时间坐标轴
/// \param chart
/// \param axisID
/// \return 若当前不是时间坐标轴，返回nullptr
///
QwtDateScaleDraw* DAChartUtil::getAxisDateTimeScale(QwtPlot* chart, int axisID)
{
	if (nullptr == chart) {
		return nullptr;
	}
	QwtScaleDraw* scale = chart->axisScaleDraw(axisID);
	return dynamic_cast< QwtDateScaleDraw* >(scale);
}
///
/// \brief 设置为普通线性坐标轴
/// \param chart
/// \param axisID
/// \return
///
QwtScaleDraw* DAChartUtil::setAxisNormalScale(QwtPlot* chart, int axisID)
{
	if (nullptr == chart) {
		return nullptr;
	}
	QwtScaleDraw* scale = nullptr;
	scale               = new QwtScaleDraw;

	chart->setAxisScaleDraw(axisID, scale);
	QwtLinearScaleEngine* scaleEngine = dynamic_cast< QwtLinearScaleEngine* >(chart->axisScaleEngine(axisID));
	if (nullptr == scaleEngine) {
		scaleEngine = new QwtLinearScaleEngine;
		chart->setAxisScaleEngine(axisID, scaleEngine);
	}
	return scale;
}

///
/// \brief 获取对应坐标轴的id
/// 如 xTop会返回xBottom
/// \param axisID
/// \return
///
///
int DAChartUtil::otherAxis(int axisID)
{
	switch (axisID) {
	case QwtPlot::xBottom:
		return QwtPlot::xTop;
	case QwtPlot::xTop:
		return QwtPlot::xBottom;
	case QwtPlot::yLeft:
		return QwtPlot::yRight;
	case QwtPlot::yRight:
		return QwtPlot::yLeft;
	default:
		return QwtPlot::xBottom;
	}
	return QwtPlot::xBottom;
}
///
/// \brief 判断是否是x坐标
/// \param axisID
/// \return
///
bool DAChartUtil::isXAxis(int axisID)
{
	return ((QwtPlot::xBottom == axisID) || (QwtPlot::xTop == axisID));
}
///
/// \brief 判断是否是y坐标
/// \param axisID
/// \return
///
bool DAChartUtil::isYAxis(int axisID)
{
    return ((QwtPlot::yLeft == axisID) || (QwtPlot::yRight == axisID));
}

/**
 * @brief 获取item的数据个数
 * @param item
 * @return -1代表没有数据
 */
int DAChartUtil::getItemDataSize(const QwtPlotItem* item)
{
	switch (item->rtti()) {
	case QwtPlotItem::Rtti_PlotCurve: {
		const QwtPlotCurve* p = static_cast< const QwtPlotCurve* >(item);
		if (p) {
			return static_cast< int >(p->data()->size());
		}
		break;
	}
	case QwtPlotItem::Rtti_PlotIntervalCurve: {
		const QwtPlotIntervalCurve* p = static_cast< const QwtPlotIntervalCurve* >(item);
		if (p) {
			return static_cast< int >(p->data()->size());
		}
		break;
	}
	case QwtPlotItem::Rtti_PlotHistogram: {
		const QwtPlotHistogram* p = static_cast< const QwtPlotHistogram* >(item);
		if (p) {
			return static_cast< int >(p->data()->size());
		}
	}
	case QwtPlotItem::Rtti_PlotBarChart: {
		const QwtPlotBarChart* p = static_cast< const QwtPlotBarChart* >(item);
		if (p) {
			return static_cast< int >(p->data()->size());
		}
	}
	case QwtPlotItem::Rtti_PlotMultiBarChart: {
		const QwtPlotMultiBarChart* p = static_cast< const QwtPlotMultiBarChart* >(item);
		if (p) {
			return static_cast< int >(p->data()->size());
		}
	}
	default:
		break;
	}
	return -1;
}
///
/// \brief SAChart::getXYDatas
/// \param xys
/// \param xs
/// \param ys
///
void DAChartUtil::getXYDatas(const QVector< QPointF >& xys, QVector< double >* xs, QVector< double >* ys)
{
	if (nullptr != xs && nullptr == ys) {
		xs->reserve(xys.size());
		std::for_each(xys.begin(), xys.end(), [xs](const QPointF& p) { xs->append(p.x()); });
	} else if (nullptr == xs && nullptr != ys) {
		ys->reserve(xys.size());
		std::for_each(xys.begin(), xys.end(), [ys](const QPointF& p) { ys->append(p.y()); });
	} else {
		xs->reserve(xys.size());
		ys->reserve(xys.size());
		std::for_each(xys.begin(), xys.end(), [xs, ys](const QPointF& p) {
			xs->append(p.x());
			ys->append(p.y());
		});
	}
}

///
/// \brief 获取一个曲线的xy值
/// \param xys
/// \param cur
///
void DAChartUtil::getXYDatas(QVector< QPointF >& xys, const QwtSeriesStore< QPointF >* cur)
{
	getSeriesData< QPointF >(xys, cur);
}

void DAChartUtil::getXYDatas(QVector< double >* xs, QVector< double >* ys, const QwtSeriesStore< QPointF >* cur)
{
	auto size = cur->dataSize();
	for (auto i = 0; i < size; ++i) {
		QPointF p = cur->sample(i);
		if (ys)
			(*ys).push_back(p.y());
		if (xs)
			(*xs).push_back(p.x());
	}
}
///
/// \brief 获取一个曲线的xy值
/// \param xys
/// \param cur
/// \param rang
/// \return
///
size_t DAChartUtil::getXYDatas(QVector< QPointF >& xys,
                               QVector< int >* indexs,
                               const QwtSeriesStore< QPointF >* cur,
                               const QRectF& rang)
{
	const QwtSeriesData< QPointF >* datas = cur->data();
	auto size                             = datas->size();
	size_t realSize                       = 0;
	if (!rang.isNull() && rang.isValid()) {
		for (size_t i = 0; i < size; ++i) {
			if (rang.contains(datas->sample(i))) {
				xys.push_back(datas->sample(i));
				++realSize;
				if (indexs) {
					(*indexs).append(static_cast< int >(i));
				}
			}
		}
	}
	return realSize;
}

size_t DAChartUtil::getXYDatas(QVector< double >* xs,
                               QVector< double >* ys,
                               QVector< int >* indexs,
                               const QwtSeriesStore< QPointF >* cur,
                               const QRectF& rang)
{
	auto size     = cur->dataSize();
	auto realSize = 0;
	if (!rang.isNull() && rang.isValid()) {
		for (auto i = 0; i < size; ++i) {
			QPointF p = cur->sample(i);
			if (rang.contains(p)) {
				if (ys) {
					(*ys).push_back(p.y());
				}
				if (xs) {
					(*xs).push_back(p.x());
				}
				if (indexs) {
					(*indexs).push_back(i);
				}
				++realSize;
			}
		}
	}
	return realSize;
}

///
/// \brief 提取范围里的2d数据点值
/// \param points 值
/// \param indexs 索引，可以设置为nullptr
/// \param series 2d数据点
/// \param rang 范围
/// 如果范围和曲线对应的坐标轴不一致，可以使用\sa transformPath 进行转换
/// \return 提取的点数
///
size_t DAChartUtil::getXYDatas(QVector< QPointF >& xys,
                               QVector< int >* indexs,
                               const QwtSeriesStore< QPointF >* series,
                               const QPainterPath& rang)
{
	auto length = series->data()->size();
	QPointF point;
	size_t resCount = 0;
	for (auto i = 0; i < length; ++i) {
		point = series->data()->sample(i);
		if (rang.contains(point)) {
			++resCount;
			xys.append(point);
			if (indexs) {
				(*indexs).append(i);
			}
		}
	}
	return resCount;
}

///
/// \brief 提取范围里的2d数据点值
/// \param xs x值
/// \param ys y值
/// \param indexs
/// \param series
/// \param rang 范围
/// \return
///
size_t DAChartUtil::getXYDatas(QVector< double >* xs,
                               QVector< double >* ys,
                               QVector< int >* indexs,
                               const QwtSeriesStore< QPointF >* series,
                               const QPainterPath& rang)
{
	size_t length = series->data()->size();
	QPointF point;
	size_t resCount = 0;
	for (size_t i = 0; i < length; ++i) {
		point = series->data()->sample(i);
		if (rang.contains(point)) {
			++resCount;
			if (xs) {
				(*xs).append(point.x());
			}
			if (ys) {
				(*ys).append(point.y());
			}
			if (indexs) {
				(*indexs).append(static_cast< int >(i));
			}
		}
	}
	return resCount;
}
///
/// \brief 对3d数据提取
/// \param xyzs
/// \param cur
///
void DAChartUtil::getXYZDatas(QVector< QwtPoint3D >& xyzs, const QwtSeriesStore< QwtPoint3D >* cur)
{
	getSeriesData< QwtPoint3D >(xyzs, cur);
}
///
/// \brief 获取间隔数据
/// \param xyzs
/// \param cur
///
void DAChartUtil::getIntervalSampleDatas(QVector< QwtIntervalSample >& intv, const QwtSeriesStore< QwtIntervalSample >* cur)
{
	getSeriesData< QwtIntervalSample >(intv, cur);
}
///
/// \brief 判断点是否在选择的范围内
/// \param range
/// \param point
/// \return
///
bool DAChartUtil::isPointInRange(const QPainterPath& range, const QPointF& point)
{
	return range.contains(point);
}

///
/// \brief 判断HistogramSample是否在区域中
///
/// \param selectRange
/// \param val
/// \return 只要(minValue,value)或者(maxValue,value)在区域中就认为是
///
bool DAChartUtil::isHistogramSampleInRange(const QPainterPath& selectRange, const QwtIntervalSample& val)
{
	QPointF v1(val.interval.minValue(), val.value), v2(val.interval.maxValue(), val.value);
	if (DAChartUtil::isPointInRange(selectRange, v1) || DAChartUtil::isPointInRange(selectRange, v2)) {
		return true;
	}
	return false;
}
///
/// \brief 判断IntervalCurveSample是否在区域中
/// \param selectRange
/// \param val
/// \return 只要(value,minValue)或者(value,maxValue)在区域中就认为是
///
bool DAChartUtil::isIntervalCurveSampleInRange(const QPainterPath& selectRange, const QwtIntervalSample& val)
{
	QPointF v1(val.value, val.interval.minValue()), v2(val.value, val.interval.maxValue());
	if (DAChartUtil::isPointInRange(selectRange, v1) || DAChartUtil::isPointInRange(selectRange, v2)) {
		return true;
	}
	return false;
}
///
/// \brief 判断MultiBarChartSample是否在区域中
/// \param selectRange
/// \param val
/// \return 所有的(value,set[n])在区域中才判定为真
///
bool DAChartUtil::isMultiBarChartSampleInRange(const QPainterPath& selectRange, const QwtSetSample& val)
{
	for (int i = 0; i < val.set.size(); ++i) {
		if (!DAChartUtil::isPointInRange(selectRange, QPointF(val.value, val.set[ i ]))) {
			return false;
		}
	}
	return true;
}
///
/// \brief 判断TradingCurveSample是否在区域中
/// \param selectRange
/// \param val
/// \return 只有(time,high),(time,low)，(time,open)，(time,close)都在区域中才判定为真
///
bool DAChartUtil::isTradingCurveSampleInRange(const QPainterPath& selectRange, const QwtOHLCSample& val)
{
	if (DAChartUtil::isPointInRange(selectRange, QPointF(val.time, val.high))
		&& DAChartUtil::isPointInRange(selectRange, QPointF(val.time, val.low))
		&& DAChartUtil::isPointInRange(selectRange, QPointF(val.time, val.open))
		&& DAChartUtil::isPointInRange(selectRange, QPointF(val.time, val.close))) {
		return true;
	}
	return false;
}
///
/// \brief 判断SpectroCurveSample是否在区域中
/// \param selectRange
/// \param val
/// \return 只要(val.x,val.y)在区域中就判定为真
///
bool DAChartUtil::isSpectroCurveSampleInRange(const QPainterPath& selectRange, const QwtPoint3D& val)
{
	return DAChartUtil::isPointInRange(selectRange, QPointF(val.x(), val.y()));
}
///
/// \brief 对点坐标进行二维偏移
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetPointSample(QPointF& sample, const double& xoffset, const double& yoffset)
{
	sample.rx() += xoffset;
	sample.ry() += yoffset;
}
///
/// \brief 对HistogramSample进行二维偏移
///
/// minValue,maxValue接受xoffset，value接受yoffset
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetHistogramSample(QwtIntervalSample& sample, const double& xoffset, const double& yoffset)
{
	sample.interval.setMinValue(sample.interval.minValue() + xoffset);
	sample.interval.setMaxValue(sample.interval.maxValue() + xoffset);
	sample.value += yoffset;
}
///
/// \brief 对IntervalCurveSample进行二维偏移
///
/// minValue,maxValue接受yoffset，value接受xoffset
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetIntervalCurveSample(QwtIntervalSample& sample, const double& xoffset, const double& yoffset)
{
	sample.interval.setMinValue(sample.interval.minValue() + yoffset);
	sample.interval.setMaxValue(sample.interval.maxValue() + yoffset);
	sample.value += xoffset;
}
///
/// \brief 对MultiBarChartSample进行二维偏移
///
/// value接受xoffset，set接受yoffset
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetMultiBarChartSample(QwtSetSample& sample, const double& xoffset, const double& yoffset)
{
	sample.value += xoffset;
	for (int i = 0; i < sample.set.size(); ++i) {
		sample.set[ i ] += yoffset;
	}
}
///
/// \brief 对TradingCurveSample进行二维偏移
///
/// time接受xoffset，high,low,close,open接受yoffset
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetTradingCurveSample(QwtOHLCSample& sample, const double& xoffset, const double& yoffset)
{
	sample.time += xoffset;
	sample.high += yoffset;
	sample.low += yoffset;
	sample.close += yoffset;
	sample.open += yoffset;
}
///
/// \brief 对SpectroCurveSample进行二维偏移
///
/// \param sample 数据
/// \param xoffset x方向偏移
/// \param yoffset y方向偏移
///
void DAChartUtil::offsetSpectroCurveSample(QwtPoint3D& sample, const double& xoffset, const double& yoffset)
{
	sample.rx() += xoffset;
	sample.ry() += yoffset;
}

///
/// \brief 设置曲线标识符
/// \param cur 曲线
/// \param style 符号类型
/// \param size 符号尺寸
///
void DAChartUtil::setCurveSymbol(QwtPlotCurve* cur, QwtSymbol::Style style, const QSize& size)
{
	QBrush brush      = cur->brush();
	QColor brushColor = brush.color();
	brushColor.setAlpha(80);
	brush.setColor(brushColor);
	QPen pen          = cur->pen();
	QwtSymbol* symbol = new QwtSymbol(style, brush, pen, size);
	cur->setSymbol(symbol);
}
///
/// \brief 设置曲线的线形
/// \param cur
/// \param style
///
void DAChartUtil::setCurveLinePenStyle(QwtPlotCurve* cur, Qt::PenStyle style)
{
	QPen pen = cur->pen();
	pen.setStyle(style);
	cur->setPen(pen);
}
///
/// \brief 设置曲线的线形
/// \param cur 曲线
/// \param style
///
void DAChartUtil::setCurvePenStyle(QwtPlotCurve* cur, Qt::PenStyle style)
{
	QPen pen = cur->pen();
	pen.setStyle(style);
	cur->setPen(pen);
}
///
/// \brief 把范围内的数据移除
/// \param removeRang 需要移除的数据范围
/// \param rawData 输入的原始数据
/// \param newData 输出的新数据
/// \return 移除的个数
///
int DAChartUtil::removeDataInRang(const QRectF& removeRang, const QVector< QPointF >& rawData, QVector< QPointF >& newData)
{
	auto length = rawData.size();
	newData.reserve(length);
	for (auto i = 0; i < length; ++i) {
		const QPointF& point = rawData[ i ];
		if (removeRang.contains(point))
			continue;
		newData.push_back(point);
	}
	return newData.size();
}
///
/// \brief 把范围内的数据移除
/// \param removeRang 需要移除的数据范围
/// \param rawData 输入的原始数据
/// \param newData 输出的新数据
/// \return 移除的个数
///
int DAChartUtil::removeDataInRang(const QPainterPath& removeRang, const QVector< QPointF >& rawData, QVector< QPointF >& newData)
{
	auto length = rawData.size();
	newData.reserve(length);
	for (auto i = 0; i < length; ++i) {
		const QPointF& point = rawData[ i ];
		if (removeRang.contains(point))
			continue;
		newData.push_back(point);
	}
	return newData.size();
}

///
/// \brief 把范围内的数据移除
/// \param removeRang 需要移除的数据范围
/// \param curve 需要移除数据的曲线
/// \return
///
int DAChartUtil::removeDataInRang(const QRectF& removeRang, QwtSeriesStore< QPointF >* curve)
{
	auto length = curve->data()->size();
	QVector< QPointF > newLine;
	newLine.reserve(static_cast< int >(length));
	QPointF point;
	for (auto i = 0; i < length; ++i) {
		point = curve->data()->sample(i);
		if (removeRang.contains(point))
			continue;
		newLine.push_back(point);
	}
	curve->setData(new QwtPointSeriesData(newLine));
	return newLine.size();
}

int DAChartUtil::removeDataInRang(const QPainterPath& removeRang, QwtSeriesStore< QPointF >* curve)
{
	auto length = curve->data()->size();
	QVector< QPointF > newLine;
	newLine.reserve(static_cast< int >(length));
	QPointF point;
	for (auto i = 0; i < length; ++i) {
		point = curve->data()->sample(i);
		if (removeRang.contains(point))
			continue;
		newLine.push_back(point);
	}
	curve->setData(new QwtPointSeriesData(newLine));
	return newLine.size();
}

void DAChartUtil::setPlotCurveSample(QwtPlotItem* p, const QVector< QPointF >& datas)
{
	setVectorSampleData< QPointF, QwtPlotCurve >(p, datas);
}

void DAChartUtil::setPlotBarChartSample(QwtPlotItem* p, const QVector< QPointF >& datas)
{
	setVectorSampleData< QPointF, QwtPlotBarChart >(p, datas);
}

void DAChartUtil::setPlotHistogramSample(QwtPlotItem* p, const QVector< QwtIntervalSample >& datas)
{
	setVectorSampleData< QwtIntervalSample, QwtPlotHistogram >(p, datas);
}

void DAChartUtil::setPlotIntervalCurveSample(QwtPlotItem* p, const QVector< QwtIntervalSample >& datas)
{
	setVectorSampleData< QwtIntervalSample, QwtPlotIntervalCurve >(p, datas);
}

void DAChartUtil::setPlotMultiBarChartSample(QwtPlotItem* p, const QVector< QwtSetSample >& datas)
{
	setVectorSampleData< QwtSetSample, QwtPlotMultiBarChart >(p, datas);
}

void DAChartUtil::setPlotTradingCurveSample(QwtPlotItem* p, const QVector< QwtOHLCSample >& datas)
{
	setVectorSampleData< QwtOHLCSample, QwtPlotTradingCurve >(p, datas);
}

void DAChartUtil::setPlotSpectroCurveSample(QwtPlotItem* p, const QVector< QwtPoint3D >& datas)
{
	setVectorSampleData< QwtPoint3D, QwtPlotSpectroCurve >(p, datas);
}

void DAChartUtil::getPlotCurveSample(QwtPlotItem* p, QVector< QPointF >& datas)
{
	getVectorSampleData< QPointF, QwtPlotCurve >(p, datas);
}

void DAChartUtil::getPlotBarChartSample(QwtPlotItem* p, QVector< QPointF >& datas)
{
	getVectorSampleData< QPointF, QwtPlotBarChart >(p, datas);
}

void DAChartUtil::getPlotHistogramSample(QwtPlotItem* p, QVector< QwtIntervalSample >& datas)
{
	getVectorSampleData< QwtIntervalSample, QwtPlotHistogram >(p, datas);
}

void DAChartUtil::getPlotIntervalCurveSample(QwtPlotItem* p, QVector< QwtIntervalSample >& datas)
{
	getVectorSampleData< QwtIntervalSample, QwtPlotIntervalCurve >(p, datas);
}

void DAChartUtil::getPlotMultiBarChartSample(QwtPlotItem* p, QVector< QwtSetSample >& datas)
{
	getVectorSampleData< QwtSetSample, QwtPlotMultiBarChart >(p, datas);
}

void DAChartUtil::getPlotTradingCurveSample(QwtPlotItem* p, QVector< QwtOHLCSample >& datas)
{
	getVectorSampleData< QwtOHLCSample, QwtPlotTradingCurve >(p, datas);
}

void DAChartUtil::getPlotSpectroCurveSample(QwtPlotItem* p, QVector< QwtPoint3D >& datas)
{
	getVectorSampleData< QwtPoint3D, QwtPlotSpectroCurve >(p, datas);
}

QRectF DAChartUtil::getVisibleRegionRang(QwtPlot* chart)
{
	QwtPlot::Axis xaxis = QwtPlot::xBottom;
	if (!chart->axisEnabled(QwtPlot::xBottom))
		xaxis = QwtPlot::xTop;
	QwtInterval inter   = chart->axisInterval(xaxis);
	double xmin         = inter.minValue();
	double xmax         = inter.maxValue();
	QwtPlot::Axis yaxis = QwtPlot::yLeft;
	if (!chart->axisEnabled(QwtPlot::yLeft))
		yaxis = QwtPlot::yRight;
	inter       = chart->axisInterval(yaxis);
	double ymin = inter.minValue();
	double ymax = inter.maxValue();
	return QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
}
///
/// \brief 获取当前正在显示的区域
/// \param chart
/// \return
///
QRectF DAChartUtil::getVisibleRegionRang(QwtPlot* chart, int xAxis, int yAxis)
{
	QwtInterval inter = chart->axisInterval(xAxis);
	double xmin       = inter.minValue();
	double xmax       = inter.maxValue();
	inter             = chart->axisInterval(yAxis);
	double ymin       = inter.minValue();
	double ymax       = inter.maxValue();
	return QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
}
///
/// \brief 动态获取item的颜色，使用dynamic_cast,需要注意效率问题
/// \param item
/// \return
///
QColor DAChartUtil::dynamicGetItemColor(const QwtPlotItem* item, const QColor& defaultColor)
{
	if (const QwtPlotCurve* p = dynamic_cast< const QwtPlotCurve* >(item)) {
		return p->pen().color();
	} else if (const QwtPlotIntervalCurve* p = dynamic_cast< const QwtPlotIntervalCurve* >(item)) {
		return p->pen().color();
	} else if (const QwtPlotHistogram* p = dynamic_cast< const QwtPlotHistogram* >(item)) {
		return p->brush().color();
	} else if (const QwtPlotBarChart* p = dynamic_cast< const QwtPlotBarChart* >(item)) {
		return p->brush().color();
	} else if (const QwtPlotGrid* grid = dynamic_cast< const QwtPlotGrid* >(item)) {
		return grid->majorPen().color();
	} else if (const QwtPlotMarker* marker = dynamic_cast< const QwtPlotMarker* >(item)) {
		return marker->linePen().color();
	}
	return defaultColor;
}
///
/// \brief 动态判断是否是绘图item，使用dynamic_cast,需要注意效率问题
/// \param item
/// \return
///
bool DAChartUtil::dynamicCheckIsPlotChartItem(const QwtPlotItem* item)
{
	if (dynamic_cast< const QwtPlotSeriesItem* >(item)) {
		return true;
	} else if (dynamic_cast< const QwtPlotRasterItem* >(item)) {
		return true;
	}
	return false;
}
///
/// \brief 动态获取XY series item，使用dynamic_cast,需要注意效率问题
/// \param chart
/// \return
///
QwtPlotItemList DAChartUtil::dynamicGetXYSeriesItemList(const QwtPlot* chart)
{
	QwtPlotItemList itemList = chart->itemList();
	QwtPlotItemList res;
	for (int i = 0; i < itemList.size(); ++i) {
		if (dynamic_cast< QwtSeriesStore< QPointF >* >(itemList[ i ])) {
			res.append(itemList[ i ]);
		}
	}
	return res;
}
///
/// \brief 动态获取plot chart item的数据点数，如果不是plot chart item,返回-1，使用dynamic_cast,需要注意效率问题
/// \param item
/// \return 返回item的点数，如果不是plot chart item,返回-1
///
int DAChartUtil::dynamicGetPlotChartItemDataCount(const QwtPlotItem* item)
{
	if (const QwtSeriesStore< QPointF >* p = dynamic_cast< const QwtSeriesStore< QPointF >* >(item)) {
		return static_cast< int >(p->dataSize());
	} else if (const QwtSeriesStore< QwtIntervalSample >* p =
				   dynamic_cast< const QwtSeriesStore< QwtIntervalSample >* >(item)) {
		return static_cast< int >(p->dataSize());
	} else if (const QwtSeriesStore< QwtSetSample >* p = dynamic_cast< const QwtSeriesStore< QwtSetSample >* >(item)) {
		return static_cast< int >(p->dataSize());
	} else if (const QwtSeriesStore< QwtPoint3D >* p = dynamic_cast< const QwtSeriesStore< QwtPoint3D >* >(item)) {
		return static_cast< int >(p->dataSize());
	}
	return -1;
}

/**
 * @brief 通过设置item的颜色
 * @param item
 * @param color
 * @return 如果设置成功，返回true
 */
bool DAChartUtil::setPlotItemColor(QwtPlotItem* item, const QColor& color)
{
	switch (item->rtti()) {
	case QwtPlotItem::Rtti_PlotCurve:
		if (QwtPlotCurve* p = static_cast< QwtPlotCurve* >(item)) {
			QPen pen = p->pen();
			pen.setColor(color);
			p->setPen(pen);
			return true;
		}
		break;
	case QwtPlotItem::Rtti_PlotIntervalCurve:
		if (QwtPlotIntervalCurve* p = static_cast< QwtPlotIntervalCurve* >(item)) {
			QPen pen = p->pen();
			pen.setColor(color);
			p->setPen(pen);
		}
		return true;
	case QwtPlotItem::Rtti_PlotHistogram:
		if (QwtPlotHistogram* p = static_cast< QwtPlotHistogram* >(item)) {
			QBrush brush = p->brush();
			brush.setColor(color);
			p->setBrush(brush);
			return true;
		}
		break;
	case QwtPlotItem::Rtti_PlotBarChart:
		if (QwtPlotBarChart* bar = static_cast< QwtPlotBarChart* >(item)) {
			QBrush brush = bar->brush();
			brush.setColor(color);
			bar->setBrush(brush);
			return true;
		}
		break;
	case QwtPlotItem::Rtti_PlotTradingCurve:
		if (QwtPlotTradingCurve* p = static_cast< QwtPlotTradingCurve* >(item)) {
			p->setSymbolPen(QPen(color));
			p->setSymbolBrush(QwtPlotTradingCurve::Increasing, QBrush(color));
			p->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QBrush(color));
			return true;
		}
	case QwtPlotItem::Rtti_PlotGrid:
		if (QwtPlotGrid* grid = static_cast< QwtPlotGrid* >(item)) {
			QPen pen = grid->majorPen();
			pen.setColor(color);
			grid->setMajorPen(pen);
			return true;
		}
		break;
	case QwtPlotItem::Rtti_PlotMarker:
		if (QwtPlotMarker* marker = static_cast< QwtPlotMarker* >(item)) {
			QPen pen = marker->linePen();
			pen.setColor(color);
			marker->setLinePen(pen);
			return true;
		}
		break;
		//	case QwtPlotItem::Rtti_PlotSpectrogram:
		//		if (QwtPlotSpectrogram* spectrogram = static_cast< QwtPlotSpectrogram* >(item)) {
		//			QPen pen = spectrogram->contourPen();
		//			pen.setColor(color);
		//			marker->setLinePen(pen);
		//			return true;
		//		}
		//		break;
	default:
		break;
	}
	return false;
}

/**
 * @brief 获取plotitem的color
 * @param item
 * @return
 */
QColor DAChartUtil::getPlotItemColor(const QwtPlotItem* item)
{
	QColor color;
	switch (item->rtti()) {
	//! For QwtPlotGrid
	case QwtPlotItem::Rtti_PlotGrid: {
		const QwtPlotGrid* li = static_cast< const QwtPlotGrid* >(item);
		color                 = li->majorPen().color();
	} break;
	//! For QwtPlotLegendItem
	case QwtPlotItem::Rtti_PlotLegend: {  // legend为文字颜色
		const QwtPlotLegendItem* li = static_cast< const QwtPlotLegendItem* >(item);
		color                       = li->textPen().color();
	} break;
	//! For QwtPlotMarker
	case QwtPlotItem::Rtti_PlotMarker: {  // Marker为linepen颜色
		const QwtPlotMarker* li = static_cast< const QwtPlotMarker* >(item);
		color                   = li->linePen().color();
	} break;
	//! For QwtPlotCurve
	case QwtPlotItem::Rtti_PlotCurve: {  // curve为线条颜色
		const QwtPlotCurve* li = static_cast< const QwtPlotCurve* >(item);
		color                  = li->pen().color();
	} break;
	//! For QwtPlotSpectroCurve
	case QwtPlotItem::Rtti_PlotSpectroCurve: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotIntervalCurve
	case QwtPlotItem::Rtti_PlotIntervalCurve: {  // Interval curve为线条颜色
		const QwtPlotIntervalCurve* li = static_cast< const QwtPlotIntervalCurve* >(item);
		color                          = li->pen().color();
	} break;
	//! For QwtPlotHistogram
	case QwtPlotItem::Rtti_PlotHistogram: {  // Histogram为填充颜色
		const QwtPlotHistogram* li = static_cast< const QwtPlotHistogram* >(item);
		color                      = li->brush().color();
	} break;
	//! For QwtPlotSpectrogram
	case QwtPlotItem::Rtti_PlotSpectrogram: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotGraphicItem, QwtPlotSvgItem
	case QwtPlotItem::Rtti_PlotGraphic:  // display graphic
		break;
	//! For QwtPlotTradingCurve
	case QwtPlotItem::Rtti_PlotTradingCurve: {  // TradingCurve 为symbolPen颜色
		const QwtPlotTradingCurve* li = static_cast< const QwtPlotTradingCurve* >(item);
		color                         = li->symbolPen().color();
	} break;
	//! For QwtPlotBarChart
	case QwtPlotItem::Rtti_PlotBarChart: {  // QwtPlotBarChart 为symbol()->palette().background().color()颜色
		const QwtPlotBarChart* bar = static_cast< const QwtPlotBarChart* >(item);
		color                      = bar->brush().color();
	} break;
	//! For QwtPlotMultiBarChart
	case QwtPlotItem::Rtti_PlotMultiBarChart: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotShapeItem
	case QwtPlotItem::Rtti_PlotShape: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotTextLabel
	case QwtPlotItem::Rtti_PlotTextLabel: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotZoneItem
	case QwtPlotItem::Rtti_PlotZone: {  // QwtPlotZoneItem 为brush().color()颜色
		const QwtPlotZoneItem* li = static_cast< const QwtPlotZoneItem* >(item);
		color                     = li->brush().color();
	} break;
	//! For QwtPlotVectorField
	case QwtPlotItem::Rtti_PlotVectorField: {  // QwtPlotZoneItem 为brush().color()颜色
		const QwtPlotVectorField* li = static_cast< const QwtPlotVectorField* >(item);
		color                        = li->pen().color();
	} break;
	default:
		break;
	}
	return color;
}

/**
 * @brief 获取plotitem的brush
 * @param item
 * @return
 */
QBrush DAChartUtil::getPlotItemBrush(const QwtPlotItem* item)
{
	QBrush brush(Qt::NoBrush);
	switch (item->rtti()) {
	//! For QwtPlotGrid
	case QwtPlotItem::Rtti_PlotGrid: {
		const QwtPlotGrid* li = static_cast< const QwtPlotGrid* >(item);
		brush                 = QBrush(li->majorPen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotLegendItem
	case QwtPlotItem::Rtti_PlotLegend: {  // legend为文字颜色
		const QwtPlotLegendItem* li = static_cast< const QwtPlotLegendItem* >(item);
		brush                       = QBrush(li->textPen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotMarker
	case QwtPlotItem::Rtti_PlotMarker: {  // Marker为linepen颜色
		const QwtPlotMarker* li = static_cast< const QwtPlotMarker* >(item);
		brush                   = QBrush(li->linePen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotCurve
	case QwtPlotItem::Rtti_PlotCurve: {  // curve为线条颜色
		const QwtPlotCurve* li = static_cast< const QwtPlotCurve* >(item);
		brush                  = QBrush(li->pen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotSpectroCurve
	case QwtPlotItem::Rtti_PlotSpectroCurve: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotIntervalCurve
	case QwtPlotItem::Rtti_PlotIntervalCurve: {  // Interval curve为线条颜色
		const QwtPlotIntervalCurve* li = static_cast< const QwtPlotIntervalCurve* >(item);
		brush                          = QBrush(li->pen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotHistogram
	case QwtPlotItem::Rtti_PlotHistogram: {  // Histogram为填充颜色
		const QwtPlotHistogram* li = static_cast< const QwtPlotHistogram* >(item);
		brush                      = li->brush();
	} break;
	//! For QwtPlotSpectrogram
	case QwtPlotItem::Rtti_PlotSpectrogram: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotGraphicItem, QwtPlotSvgItem
	case QwtPlotItem::Rtti_PlotGraphic:  // display graphic
		break;
	//! For QwtPlotTradingCurve
	case QwtPlotItem::Rtti_PlotTradingCurve: {  // TradingCurve 为symbolPen颜色
		const QwtPlotTradingCurve* li = static_cast< const QwtPlotTradingCurve* >(item);
		brush                         = QBrush(li->symbolPen().color(), Qt::SolidPattern);
	} break;
	//! For QwtPlotBarChart
	case QwtPlotItem::Rtti_PlotBarChart: {  // QwtPlotBarChart 为symbol()->palette().background().color()颜色
		const QwtPlotBarChart* bar = static_cast< const QwtPlotBarChart* >(item);
		brush                      = bar->brush();
	} break;
	//! For QwtPlotMultiBarChart
	case QwtPlotItem::Rtti_PlotMultiBarChart: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotShapeItem
	case QwtPlotItem::Rtti_PlotShape: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotTextLabel
	case QwtPlotItem::Rtti_PlotTextLabel: {
		// 暂时不出来
		//  TODO:绘制colormap
	} break;
	//! For QwtPlotZoneItem
	case QwtPlotItem::Rtti_PlotZone: {  // QwtPlotZoneItem 为brush().color()颜色
		const QwtPlotZoneItem* li = static_cast< const QwtPlotZoneItem* >(item);
		brush                     = li->brush();
	} break;
	//! For QwtPlotVectorField
	case QwtPlotItem::Rtti_PlotVectorField: {  // QwtPlotZoneItem 为brush().color()颜色
		const QwtPlotVectorField* li = static_cast< const QwtPlotVectorField* >(item);
		brush                        = QBrush(li->pen().color(), Qt::SolidPattern);
	} break;
	default:
		break;
	}
	return brush;
}

///
/// \brief 通过rtti获取可绘图的item，
/// \see dynamicGetPlotChartItemList
/// \param chart
/// \return
///
QwtPlotItemList DAChartUtil::getPlotChartItemList(const QwtPlot* chart)
{
	QwtPlotItemList itemList = chart->itemList();
	QwtPlotItemList res;
	for (int i = 0; i < itemList.size(); ++i) {
		if (checkIsPlotChartItem(itemList[ i ])) {
			res.append(itemList[ i ]);
		}
	}
	return res;
}
///
/// \brief 通过rtti判断是否是绘图item
/// \see dynamicCheckIsPlotChartItem
/// \param item
/// \return
///
bool DAChartUtil::checkIsPlotChartItem(const QwtPlotItem* item)
{
	switch (item->rtti()) {
	case QwtPlotItem::Rtti_PlotCurve:
	case QwtPlotItem::Rtti_PlotIntervalCurve:
	case QwtPlotItem::Rtti_PlotHistogram:
	case QwtPlotItem::Rtti_PlotBarChart:
	case QwtPlotItem::Rtti_PlotSpectroCurve:
	case QwtPlotItem::Rtti_PlotSpectrogram:
	case QwtPlotItem::Rtti_PlotTradingCurve:
	case QwtPlotItem::Rtti_PlotMultiBarChart:
		return true;
	default:
		break;
	}
	return false;
}
///
/// \brief 通过rtti获取XY series item
/// \param chart
/// \return
///
QwtPlotItemList DAChartUtil::getXYSeriesItemList(const QwtPlot* chart)
{
	QwtPlotItemList itemList = chart->itemList();
	QwtPlotItemList res;
	for (int i = 0; i < itemList.size(); ++i) {
		if (checkIsXYSeriesItem(itemList[ i ])) {
			res.append(itemList[ i ]);
		}
	}
	return res;
}
///
/// \brief 通过rtti判断是否是XY series item
/// \param item
/// \return
///
bool DAChartUtil::checkIsXYSeriesItem(const QwtPlotItem* item)
{
	switch (item->rtti()) {
	case QwtPlotItem::Rtti_PlotCurve:
	case QwtPlotItem::Rtti_PlotBarChart:
	case QwtPlotItem::Rtti_PlotScale:
		return true;
	default:
		break;
	}
	return false;
}

///
/// \brief 通过rtti获取所有plot的数据范围，并做并集
/// 也就是最大的数据范围
/// \param chart
/// \return
///
void DAChartUtil::dataRange(const QwtPlot* chart, QwtInterval* yLeft, QwtInterval* yRight, QwtInterval* xBottom, QwtInterval* xTop)
{
	QwtInterval intv[ QwtPlot::axisCnt ];
	const QwtPlotItemList& itmList = DAChartUtil::getPlotChartItemList(chart);
	QwtPlotItemIterator it;
	for (it = itmList.begin(); it != itmList.end(); ++it) {
		const QwtPlotItem* item = *it;
		if (!item->isVisible())
			continue;
		QRectF rect;
		switch (item->rtti()) {
		case QwtPlotItem::Rtti_PlotCurve: {
			const QwtPlotCurve* p = static_cast< const QwtPlotCurve* >(item);
			rect                  = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotHistogram: {
			const QwtPlotHistogram* p = static_cast< const QwtPlotHistogram* >(item);
			rect                      = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotBarChart: {
			const QwtPlotBarChart* p = static_cast< const QwtPlotBarChart* >(item);
			rect                     = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotIntervalCurve: {
			const QwtPlotIntervalCurve* p = static_cast< const QwtPlotIntervalCurve* >(item);
			rect                          = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotMultiBarChart: {
			const QwtPlotMultiBarChart* p = static_cast< const QwtPlotMultiBarChart* >(item);
			rect                          = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotTradingCurve: {
			const QwtPlotTradingCurve* p = static_cast< const QwtPlotTradingCurve* >(item);
			rect                         = p->dataRect();
			break;
		}
		case QwtPlotItem::Rtti_PlotSpectroCurve: {
			const QwtPlotSpectroCurve* p = static_cast< const QwtPlotSpectroCurve* >(item);
			rect                         = p->dataRect();
			break;
		}
		default:
			break;
		}
		if (rect.width() >= 0.0)
			intv[ item->xAxis() ] |= QwtInterval(rect.left(), rect.right());
		if (rect.height() >= 0.0)
			intv[ item->yAxis() ] |= QwtInterval(rect.top(), rect.bottom());
	}
	if (yLeft)
		*yLeft = intv[ QwtPlot::yLeft ];
	if (yRight)
		*yRight = intv[ QwtPlot::yRight ];
	if (xTop)
		*xTop = intv[ QwtPlot::xTop ];
	if (xBottom)
		*xBottom = intv[ QwtPlot::xBottom ];
}

///
/// \brief 动态获取可绘图的item，使用dynamic_cast,需要注意效率问题
/// \param chart
/// \return
///
QwtPlotItemList DAChartUtil::dynamicGetPlotChartItemList(const QwtPlot* chart)
{
	QwtPlotItemList itemList = chart->itemList();
	QwtPlotItemList res;
	for (int i = 0; i < itemList.size(); ++i) {
		if (dynamicCheckIsPlotChartItem(itemList[ i ])) {
			res.append(itemList[ i ]);
		}
	}
	return res;
}

///
/// \brief 获取屏幕位置离bar最近的点，类似于QwtPlotCurve::closestPoint
/// \param bar
/// \param pos pos Position, where to look for the closest curve point
/// \param distdist If dist != NULL, closestPoint() returns the distance between
/// the position and the closest bar point
/// \return Index of the closest bar point, or -1 if none can be found
///
int DAChartUtil::closestPoint(const QwtPlotBarChart* bar, const QPoint& pos, double* dist)
{
	const size_t numSamples = bar->dataSize();

	if (bar->plot() == NULL || numSamples <= 0)
		return -1;
	const QwtSeriesData< QPointF >* series = bar->data();
	const QwtScaleMap xMap                 = bar->plot()->canvasMap(bar->xAxis());
	const QwtScaleMap yMap                 = bar->plot()->canvasMap(bar->yAxis());

	int index   = -1;
	double dmin = 1.0e10;

	for (uint i = 0; i < numSamples; i++) {
		const QPointF sample = series->sample(i);

		const double cx = xMap.transform(sample.x()) - pos.x();
		const double cy = yMap.transform(sample.y()) - pos.y();

		const double f = qwtSqr(cx) + qwtSqr(cy);
		if (f < dmin) {
			index = i;
			dmin  = f;
		}
	}
	if (dist)
		*dist = qSqrt(dmin);

	return index;
}

/**
 * @brief 通过点数量推荐线宽
 * @param pointsNumber
 * @return
 */
qreal DAChartUtil::recommendCurveLinePenWidth(int pointsNumber)
{
	if (pointsNumber < 100) {
		return 2.0;
	} else if (pointsNumber >= 100 && pointsNumber < 2000) {
		return 1.0;
	}
	// 大于2000点
	return 0.5;
}
}  // End Of Namespace DA
