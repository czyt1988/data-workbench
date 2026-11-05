#include "Table/Chart/DAChartQuickPlot.h"
#include "Table/Chart/DAChartSeriesSampler.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "DATableDataSource.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartPlotRenderer.h"
#include "Chart/AddItem/DAChartAddCurveWidget.h"
#include "Chart/AddItem/DAChartAddHistogramWidget.h"
#include "Chart/AddItem/DAChartAddContourWidget.h"
#include "Chart/AddItem/DAChartAddErrorBarWidget.h"
#include "Chart/AddItem/DAChartAddVectorFieldWidget.h"
#include "Chart/AddItem/DAChartAddMultiBarWidget.h"
#include "Chart/AddItem/DAChartAdd3DSurfaceWidget.h"
#include "Chart/AddItem/DAChartAdd3DLineWidget.h"
#include "DAChartOperateWidget.h"
#include "DAChartOperateWidgetCommands.h"
#include "DAFigureWidget.h"
#include "Chart/DAStatsPlotCoordinator.h"
#include "DAPyScripts.h"
#include "DAPybind11QtCaster.hpp"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"
#include "DAPlotDataTypes.h"
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_item.h"
#include "qwt_axis.h"
#include "qwt_scale_draw.h"
#include "qwt_text.h"
// qwt3d
#include "qwt3d_types.h"
// Qt
#include <QVariantMap>
#include <QVariantList>
#include <QUndoStack>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QCoreApplication>
#include <algorithm>

namespace DA
{

namespace
{
// 非 QObject 类的翻译统一用 QCoreApplication::translate("DAChartQuickPlot", ...)：
// lupdate 只认这种带字面量的调用形式，不认自定义包装函数/宏

// 分类基数守卫：唯一值超过该值时只取 TopN（实施方案 D5）
constexpr int kCategoryGuardThreshold = 50;
// 箱线图固定口径：whis=1.5、显示离群点、无 CI（实施方案 D10）
constexpr double kBoxplotWhisker = 1.5;

void setError(QString* error, const QString& text)
{
    if (error) {
        *error = text;
    }
}

QVariantList toVariantList(const QStringList& values)
{
    QVariantList res;
    res.reserve(values.size());
    for (const QString& v : values) {
        res.append(v);
    }
    return res;
}

QVariantList toVariantList(const QVector< double >& values)
{
    QVariantList res;
    res.reserve(values.size());
    for (double v : values) {
        res.append(v);
    }
    return res;
}

QStringList dictToStringList(const pybind11::dict& d, const char* key)
{
    QStringList res;
    pybind11::str k(key);
    if (!d.contains(k)) {
        return res;
    }
    pybind11::object obj = d[k];
    if (obj.is_none() || !pybind11::isinstance< pybind11::list >(obj)) {
        return res;
    }
    try {
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(obj)) {
            res.append(pybind11::cast< QString >(item));
        }
    } catch (const std::exception& e) {
        qWarning() << "DAChartQuickPlot: cannot read string list" << key << e.what();
        res.clear();
    }
    return res;
}

QVector< double > dictToDoubleVector(const pybind11::dict& d, const char* key)
{
    QVector< double > res;
    pybind11::str k(key);
    if (!d.contains(k)) {
        return res;
    }
    pybind11::object obj = d[k];
    if (obj.is_none() || !pybind11::isinstance< pybind11::list >(obj)) {
        return res;
    }
    try {
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(obj)) {
            res.append(pybind11::cast< double >(item));
        }
    } catch (const std::exception& e) {
        qWarning() << "DAChartQuickPlot: cannot read double list" << key << e.what();
        res.clear();
    }
    return res;
}

// 重名列的显示名：name、name (2)、name (3)…
QString displayName(const DATableChartColumn& column)
{
    if (column.occurrence <= 0) {
        return column.name;
    }
    return QStringLiteral("%1 (%2)").arg(column.name).arg(column.occurrence + 1);
}

QString columnDisplayName(const DATableChartRequest& request, int columnIndex)
{
    if (columnIndex < 0 || columnIndex >= request.columns.size()) {
        return QString();
    }
    return displayName(request.columns.at(columnIndex));
}

int dataColumnOf(const DATableChartRequest& request, int columnIndex)
{
    if (columnIndex < 0 || columnIndex >= request.columns.size()) {
        return -1;
    }
    return request.columns.at(columnIndex).dataIndex;
}

DATableChartAggregation aggregationOf(const DATableChartSpec& spec)
{
    return spec.aggregation;
}

QString aggregationDisplayName(DATableChartAggregation aggregation)
{
    switch (aggregation) {
    case DATableChartAggregation::Mean:
        return QCoreApplication::translate("DAChartQuickPlot", "mean");  // cn:平均值
    case DATableChartAggregation::Median:
        return QCoreApplication::translate("DAChartQuickPlot", "median");  // cn:中位数
    case DATableChartAggregation::Sum:
        return QCoreApplication::translate("DAChartQuickPlot", "sum");  // cn:求和
    case DATableChartAggregation::Count:
        return QCoreApplication::translate("DAChartQuickPlot", "count");  // cn:计数
    case DATableChartAggregation::Std:
        return QCoreApplication::translate("DAChartQuickPlot", "std");  // cn:标准差
    case DATableChartAggregation::Var:
        return QCoreApplication::translate("DAChartQuickPlot", "var");  // cn:方差
    case DATableChartAggregation::Min:
        return QCoreApplication::translate("DAChartQuickPlot", "min");  // cn:最小值
    case DATableChartAggregation::Max:
        return QCoreApplication::translate("DAChartQuickPlot", "max");  // cn:最大值
    }
    return QString();
}

// 清空后把坐标轴恢复成普通线性轴（上一次可能是类别轴或时间轴）
void resetAxes(DAChartWidget* chart)
{
    if (!chart) {
        return;
    }
    const int axes[] = { QwtAxis::XBottom, QwtAxis::YLeft };
    for (int axis : axes) {
        chart->setAxisScaleDraw(axis, new QwtScaleDraw());
        chart->setAxisAutoScale(axis, true);
        chart->setAxisTitle(axis, QString());
    }
}

void applyAxisTitles(DAChartWidget* chart, const QString& xTitle, const QString& yTitle)
{
    if (!chart) {
        return;
    }
    if (!xTitle.isEmpty()) {
        chart->setAxisTitle(QwtAxis::XBottom, xTitle);
    }
    if (!yTitle.isEmpty()) {
        chart->setAxisTitle(QwtAxis::YLeft, yTitle);
    }
}

void fillSampleInfo(DATableChartBuildInfo* info, const DAChartSeriesSampler::SampleInfo& sample)
{
    if (!info) {
        return;
    }
    info->sampled        = sample.sampled;
    info->sampledRows    = sample.sampledRows;
    info->totalRows      = sample.totalRows;
    info->droppedMissing = sample.droppedMissing;
    // 同一次构建可能有多列采样，任一列触及预算就整体标注「样本受限」
    info->sampleLimited = info->sampleLimited || sample.limited;
}

// 折线/散点：采样 → 复用 DAChartAddCurveWidget 的图元构建
bool buildLineLike(const DATableChartRequest& request,
                   const DATableChartSpec& spec,
                   DAChartWidget* chart,
                   DATableChartBuildInfo* info,
                   QString* error,
                   DAChartSeriesSampler::SampleBudget* budget)
{
    const int xDataColumn = dataColumnOf(request, spec.xColumn);
    const QString xTitle  = spec.xColumn >= 0 ? columnDisplayName(request, spec.xColumn)
                                              : QCoreApplication::translate("DAChartQuickPlot", "row");  // cn:行号

    if (spec.valueColumns.size() == 1 && spec.type != DATableChartType::MultiLine) {
        const int yIndex = spec.valueColumns.first();
        const int yData  = dataColumnOf(request, yIndex);
        if (yData < 0) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
            return false;
        }
        const QString yTitle = columnDisplayName(request, yIndex);
        const DAChartSeriesSampler::XYSample sample = DAChartSeriesSampler::sampleXY(
            request, xDataColumn, yData, DAChartSeriesSampler::kMaxLinePoints, budget);
        fillSampleInfo(info, sample.info);
        if (sample.points.isEmpty()) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "No valid numeric data in the selected column"));  // cn:所选列没有可绘制的数值
            return false;
        }
        DAChartAddCurveWidget widget;
        widget.setProgrammaticSeries(sample.points, yTitle);
        QwtPlotItem* item = widget.createPlotItem();
        if (!item) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to build the series"));  // cn:构建序列失败
            return false;
        }
        if (spec.type == DATableChartType::Scatter) {
            if (QwtPlotCurve* curve = dynamic_cast< QwtPlotCurve* >(item)) {
                curve->setStyle(QwtPlotCurve::Dots);
            }
        }
        item->attach(chart);
        applyAxisTitles(chart, xTitle, yTitle);
    } else {
        const DAChartSeriesSampler::MultiXYSample sample = DAChartSeriesSampler::sampleMultiXY(
            request, spec.xColumn, spec.valueColumns, DAChartSeriesSampler::kMaxLinePoints, budget);
        fillSampleInfo(info, sample.info);
        if (sample.series.isEmpty()) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "No valid numeric data in the selected columns"));  // cn:所选列没有可绘制的数值
            return false;
        }
        DAChartAddCurveWidget widget;
        widget.setProgrammaticMultiSeries(sample.series, sample.names);
        const QList< QwtPlotItem* > items = widget.createPlotItems();
        if (items.isEmpty()) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to build the series"));  // cn:构建序列失败
            return false;
        }
        for (QwtPlotItem* item : items) {
            item->attach(chart);
        }
        applyAxisTitles(chart, xTitle, QString());
    }

    if (spec.xColumn >= 0 && request.columns.at(spec.xColumn).isDateTime()) {
        chart->setupDateTimeAxis(QwtAxis::XBottom);
    }
    return true;
}

// 直方图：采样 → 复用 DAChartAddHistogramWidget 的分箱与图元构建
bool buildHistogram(const DATableChartRequest& request,
                    const DATableChartSpec& spec,
                    DAChartWidget* chart,
                    DATableChartBuildInfo* info,
                    QString* error,
                    DAChartSeriesSampler::SampleBudget* budget)
{
    const int yIndex = spec.valueColumns.value(0, -1);
    const int yData  = dataColumnOf(request, yIndex);
    if (yData < 0) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    DAChartSeriesSampler::SampleInfo sample;
    const QVector< double > values = DAChartSeriesSampler::sampleValues(
        request, yData, DAChartSeriesSampler::kMaxHistogramValues, &sample, budget);
    fillSampleInfo(info, sample);
    if (values.isEmpty()) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "No valid numeric data in the selected column"));  // cn:所选列没有可绘制的数值
        return false;
    }
    const QString title = columnDisplayName(request, yIndex);
    DAChartAddHistogramWidget widget;
    widget.setProgrammaticValues(values, spec.histogramBins, title);
    QwtPlotItem* item = widget.createPlotItem();
    if (!item) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to build the histogram"));  // cn:构建直方图失败
        return false;
    }
    item->attach(chart);
    applyAxisTitles(chart, title, QCoreApplication::translate("DAChartQuickPlot", "count"));  // cn:计数
    return true;
}

// 箱线图：Python 五数概括 → 统计管线的既有渲染（与统计向导同一份实现）
bool buildBoxplot(const DATableChartRequest& request,
                  const DATableChartSpec& spec,
                  DAChartWidget* chart,
                  DATableChartBuildInfo* info,
                  QString* error,
                  DAChartSeriesSampler::SampleBudget* budget)
{
    if (spec.valueColumns.isEmpty()) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    QVariantMap args;
    args[ QStringLiteral("whis") ] = kBoxplotWhisker;

    QStringList labels;
    for (int idx : std::as_const(spec.valueColumns)) {
        labels.append(columnDisplayName(request, idx));
    }
    const bool hasCategory = (spec.categoryColumn >= 0 && spec.categoryColumn < request.columns.size()
                              && request.columns.at(spec.categoryColumn).isCategorical());
    const bool exact       = DAChartSeriesSampler::canAggregateExactly(request);

    pybind11::dict result;
    if (exact) {
        QVariantList columns;
        for (int idx : std::as_const(spec.valueColumns)) {
            columns.append(dataColumnOf(request, idx));
        }
        args[ QStringLiteral("columns") ] = columns;
        args[ QStringLiteral("labels") ]  = toVariantList(labels);
        if (hasCategory) {
            args[ QStringLiteral("hue") ] = dataColumnOf(request, spec.categoryColumn);
        }
        result = DAPyScripts::getStatistics().computeBoxplotStats(request.data.toDataFrame(), args);
    } else {
        QVariantMap columns;
        if (hasCategory) {
            // 分类 + 一个或多个数值：按行对齐采样，hue 与各值列一一对应
            QList< int > valueDataColumns;
            for (int idx : std::as_const(spec.valueColumns)) {
                valueDataColumns.append(dataColumnOf(request, idx));
            }
            const DAChartSeriesSampler::CategoryMultiSample sample = DAChartSeriesSampler::sampleCategoryMulti(
                request,
                dataColumnOf(request, spec.categoryColumn),
                valueDataColumns,
                DAChartSeriesSampler::kMaxHistogramValues,
                budget);
            fillSampleInfo(info, sample.info);
            columns[ QStringLiteral("__hue") ] = toVariantList(sample.labels);
            QStringList keys;
            for (int i = 0; i < sample.columns.size(); ++i) {
                const QString key = QStringLiteral("__v%1").arg(i);
                keys.append(key);
                columns[ key ] = toVariantList(sample.columns.at(i));
            }
            // 注意：传给 Python 的列表必须是 QVariantList——QStringList 值的 QVariant
            // 在 QtCaster 里没有对应分支，会退化成 toString() 的空串（列名丢失）
            args[ QStringLiteral("columns") ] = toVariantList(keys);
            args[ QStringLiteral("hue") ]     = QStringLiteral("__hue");
        } else {
            QStringList keys;
            DAChartSeriesSampler::SampleInfo merged;
            for (int i = 0; i < spec.valueColumns.size(); ++i) {
                DAChartSeriesSampler::SampleInfo sample;
                const QVector< double > values = DAChartSeriesSampler::sampleValues(
                    request,
                    dataColumnOf(request, spec.valueColumns.at(i)),
                    DAChartSeriesSampler::kMaxHistogramValues,
                    &sample,
                    budget);
                merged.totalRows       = sample.totalRows;
                merged.sampled         = sample.sampled;
                merged.sampledRows += sample.sampledRows;
                merged.droppedMissing += sample.droppedMissing;
                merged.limited = merged.limited || sample.limited;
                const QString key = QStringLiteral("__v%1").arg(i);
                keys.append(key);
                columns[ key ] = toVariantList(values);
            }
            fillSampleInfo(info, merged);
            // 同上：列键必须以 QVariantList 传给 Python（QStringList 会退化为空串）
            args[ QStringLiteral("columns") ] = toVariantList(keys);
        }
        args[ QStringLiteral("labels") ] = toVariantList(labels);
        result = DAPyScripts::getStatistics().computeBoxplotStatsValueLists(columns, args);
    }
    if (info) {
        info->approximate = !exact;
    }

    DAChartPlotRenderer renderer(chart);
    if (!DAStatsPlotCoordinator::renderBoxplotResult(result, renderer,
                                                     QCoreApplication::translate("DAChartQuickPlot", "box plot"))) {  // cn:箱线图
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "No valid numeric data in the selected columns"));  // cn:所选列没有可绘制的数值
        return false;
    }
    return true;
}

// 分类柱状：精确优先，惰性/超阈值源走采样（同一套 Python 聚合口径）
bool buildBar(const DATableChartRequest& request,
              const DATableChartSpec& spec,
              DAChartWidget* chart,
              DATableChartBuildInfo* info,
              QString* error,
              DAChartSeriesSampler::SampleBudget* budget)
{
    if (spec.categoryColumn < 0 || spec.categoryColumn >= request.columns.size()) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    const bool countMode = spec.valueColumns.isEmpty();
    const int valueIndex = countMode ? -1 : spec.valueColumns.first();
    const QString categoryName = columnDisplayName(request, spec.categoryColumn);
    const QString valueName    = countMode ? QString() : columnDisplayName(request, valueIndex);

    QVariantMap args;
    args[ QStringLiteral("sort") ] = spec.sortByValueDesc ? QStringLiteral("value_desc")
                                                          : QStringLiteral("category");
    if (!countMode) {
        args[ QStringLiteral("estimator") ] =
            DATableChartPlanner::aggregationKey(aggregationOf(spec));
    }

    const bool exact = DAChartSeriesSampler::canAggregateExactly(request);
    pybind11::dict result;
    if (exact) {
        result = DAPyScripts::getStatistics().aggregateByCategoryByIndex(
            request.data.toDataFrame(),
            dataColumnOf(request, spec.categoryColumn),
            countMode ? -1 : dataColumnOf(request, valueIndex),
            args);
    } else {
        const DAChartSeriesSampler::CategorySample sample = DAChartSeriesSampler::sampleCategory(
            request,
            dataColumnOf(request, spec.categoryColumn),
            countMode ? -1 : dataColumnOf(request, valueIndex),
            DAChartSeriesSampler::kMaxHistogramValues,
            budget);
        fillSampleInfo(info, sample.info);
        QVariantMap columns;
        columns[ QStringLiteral("__cat") ] = toVariantList(sample.labels);
        if (!countMode) {
            columns[ QStringLiteral("__val") ] = toVariantList(sample.values);
        }
        result = DAPyScripts::getStatistics().aggregateByCategoryValueLists(
            columns,
            QStringLiteral("__cat"),
            countMode ? QString() : QStringLiteral("__val"),
            args);
    }
    if (info) {
        info->approximate = !exact;
    }

    QStringList categories = dictToStringList(result, "categories");
    QVector< double > values = dictToDoubleVector(result, "values");
    if (categories.isEmpty() || categories.size() != values.size()) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "No valid data for the bar chart"));  // cn:没有可用于柱状图的数据
        return false;
    }

    const int totalCategories = categories.size();
    if (spec.categoryTopN > 0 && totalCategories > kCategoryGuardThreshold
        && totalCategories > spec.categoryTopN) {
        categories = categories.mid(0, spec.categoryTopN);
        values     = values.mid(0, spec.categoryTopN);
    }
    if (info) {
        info->totalCategories     = totalCategories;
        info->displayedCategories = categories.size();
    }

    DA::DABarChartData barData;
    barData.categories = categories;
    barData.values     = values;

    QVariantMap style;
    style[ QStringLiteral("title") ] =
        countMode ? QCoreApplication::translate("DAChartQuickPlot", "count of %1").arg(categoryName)  // cn:%1 的计数
                  : QCoreApplication::translate("DAChartQuickPlot", "%1 by %2").arg(valueName, categoryName);  // cn:%1 按 %2 聚合
    DAChartPlotRenderer renderer(chart);
    if (!renderer.renderBarChart(barData, style)) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to build the bar chart"));  // cn:构建柱状图失败
        return false;
    }
    applyAxisTitles(chart,
                    categoryName,
                    countMode ? QCoreApplication::translate("DAChartQuickPlot", "count")  // cn:计数
                              : aggregationDisplayName(aggregationOf(spec)));
    return true;
}

// ============================================================
// v3 新图型：等高线 / 误差棒 / 向量场 / 多重柱状 / 3D
// ============================================================

// 多重柱状：类别轴按序号（0..n-1）排列，刻度文字回填类别名
class QuickPlotCategoryScaleDraw : public QwtScaleDraw
{
public:
    explicit QuickPlotCategoryScaleDraw(const QStringList& labels) : m_labels(labels)
    {
    }
    QwtText label(double value) const override
    {
        const int index = qRound(value);
        if (index >= 0 && index < m_labels.size()) {
            return QwtText(m_labels.at(index));
        }
        return QwtText();
    }

private:
    QStringList m_labels;
};

// 校验一列是否可用于数值角色
bool requireNumericColumns(const DATableChartRequest& request, const QList< int >& columnIndexes, QString* error)
{
    for (int index : columnIndexes) {
        if (index < 0 || index >= request.columns.size() || !request.columns.at(index).isNumeric()) {
            setError(error,
                     QCoreApplication::translate("DAChartQuickPlot",
                                                 "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
            return false;
        }
    }
    return true;
}

// 通用多列采样 → 每列一个向量；失败时写入可读原因
bool sampleNumericColumns(const DATableChartRequest& request,
                          const QList< int >& columnIndexes,
                          int maxPoints,
                          DATableChartBuildInfo* info,
                          QString* error,
                          DAChartSeriesSampler::SampleBudget* budget,
                          DAChartSeriesSampler::ColumnsSample* sample)
{
    if (!requireNumericColumns(request, columnIndexes, error)) {
        return false;
    }
    QList< int > dataColumns;
    for (int index : columnIndexes) {
        dataColumns.append(dataColumnOf(request, index));
    }
    *sample = DAChartSeriesSampler::sampleColumns(request, dataColumns, maxPoints, budget);
    fillSampleInfo(info, sample->info);
    if (sample->rowCount() == 0) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "No valid numeric data in the selected columns"));  // cn:所选列没有可绘制的数值
        return false;
    }
    return true;
}

// 等高线：x/y/value 三列 → QwtPlotSpectroCurve（复用添加绘图页的图元构建）
bool buildContour(const DATableChartRequest& request,
                  const DATableChartSpec& spec,
                  DAChartWidget* chart,
                  DATableChartBuildInfo* info,
                  QString* error,
                  DAChartSeriesSampler::SampleBudget* budget)
{
    DAChartSeriesSampler::ColumnsSample sample;
    if (!sampleNumericColumns(request,
                              { spec.xColumn, spec.yColumn, spec.zColumn },
                              DAChartSeriesSampler::kMaxLinePoints,
                              info,
                              error,
                              budget,
                              &sample)) {
        return false;
    }
    QVector< QwtPoint3D > points;
    points.reserve(sample.rowCount());
    for (int i = 0; i < sample.rowCount(); ++i) {
        points.append(QwtPoint3D(sample.columns.at(0).at(i), sample.columns.at(1).at(i), sample.columns.at(2).at(i)));
    }
    DAChartAddContourWidget widget;
    widget.setProgrammaticSamples(points, columnDisplayName(request, spec.zColumn));
    QwtPlotItem* item = widget.createPlotItem();
    if (!item) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to build the contour plot"));  // cn:构建等高线图失败
        return false;
    }
    item->attach(chart);
    applyAxisTitles(chart, columnDisplayName(request, spec.xColumn), columnDisplayName(request, spec.yColumn));
    return true;
}

// 误差棒：x/y/误差 三列 → y ± error 的区间曲线
bool buildErrorBar(const DATableChartRequest& request,
                   const DATableChartSpec& spec,
                   DAChartWidget* chart,
                   DATableChartBuildInfo* info,
                   QString* error,
                   DAChartSeriesSampler::SampleBudget* budget)
{
    DAChartSeriesSampler::ColumnsSample sample;
    if (!sampleNumericColumns(request,
                              { spec.xColumn, spec.yColumn, spec.errorColumn },
                              DAChartSeriesSampler::kMaxLinePoints,
                              info,
                              error,
                              budget,
                              &sample)) {
        return false;
    }
    QVector< QwtIntervalSample > intervals;
    intervals.reserve(sample.rowCount());
    for (int i = 0; i < sample.rowCount(); ++i) {
        const double x    = sample.columns.at(0).at(i);
        const double y    = sample.columns.at(1).at(i);
        const double err  = std::abs(sample.columns.at(2).at(i));
        const double low  = std::min(y - err, y + err);
        const double high = std::max(y - err, y + err);
        intervals.append(QwtIntervalSample(x, QwtInterval(low, high)));
    }
    DAChartAddErrorBarWidget widget;
    widget.setProgrammaticSeries(intervals, columnDisplayName(request, spec.yColumn));
    QwtPlotItem* item = widget.createPlotItem();
    if (!item) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to build the error bar chart"));  // cn:构建误差棒图失败
        return false;
    }
    item->attach(chart);
    applyAxisTitles(chart, columnDisplayName(request, spec.xColumn), columnDisplayName(request, spec.yColumn));
    return true;
}

// 向量场：x/y/u/v 四列 → QwtPlotVectorField
bool buildVectorField(const DATableChartRequest& request,
                      const DATableChartSpec& spec,
                      DAChartWidget* chart,
                      DATableChartBuildInfo* info,
                      QString* error,
                      DAChartSeriesSampler::SampleBudget* budget)
{
    DAChartSeriesSampler::ColumnsSample sample;
    if (!sampleNumericColumns(request,
                              { spec.xColumn, spec.yColumn, spec.uColumn, spec.vColumn },
                              DAChartSeriesSampler::kMaxLinePoints,
                              info,
                              error,
                              budget,
                              &sample)) {
        return false;
    }
    QVector< QwtVectorFieldSample > vectors;
    vectors.reserve(sample.rowCount());
    for (int i = 0; i < sample.rowCount(); ++i) {
        vectors.append(QwtVectorFieldSample(sample.columns.at(0).at(i),
                                            sample.columns.at(1).at(i),
                                            sample.columns.at(2).at(i),
                                            sample.columns.at(3).at(i)));
    }
    DAChartAddVectorFieldWidget widget;
    widget.setProgrammaticSamples(
        vectors,
        QStringLiteral("%1 / %2")
            .arg(columnDisplayName(request, spec.uColumn), columnDisplayName(request, spec.vColumn)));
    QwtPlotItem* item = widget.createPlotItem();
    if (!item) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to build the vector field chart"));  // cn:构建向量场图失败
        return false;
    }
    item->attach(chart);
    applyAxisTitles(chart, columnDisplayName(request, spec.xColumn), columnDisplayName(request, spec.yColumn));
    return true;
}

// 单列的分类聚合并取结果（多重柱状复用）
bool aggregateCategoryValues(const DATableChartRequest& request,
                             int categoryIndex,
                             int valueIndex,
                             const DATableChartSpec& spec,
                             DATableChartBuildInfo* info,
                             QString* error,
                             DAChartSeriesSampler::SampleBudget* budget,
                             QStringList* categories,
                             QVector< double >* values)
{
    const int categoryData = dataColumnOf(request, categoryIndex);
    const int valueData    = dataColumnOf(request, valueIndex);
    if (categoryData < 0 || valueData < 0) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    QVariantMap args;
    args[ QStringLiteral("sort") ] = spec.sortByValueDesc ? QStringLiteral("value_desc")
                                                          : QStringLiteral("category");
    args[ QStringLiteral("estimator") ] = DATableChartPlanner::aggregationKey(spec.aggregation);

    const bool exact = DAChartSeriesSampler::canAggregateExactly(request);
    pybind11::dict result;
    if (exact) {
        result = DAPyScripts::getStatistics().aggregateByCategoryByIndex(
            request.data.toDataFrame(), categoryData, valueData, args);
    } else {
        const DAChartSeriesSampler::CategorySample sample = DAChartSeriesSampler::sampleCategory(
            request, categoryData, valueData, DAChartSeriesSampler::kMaxHistogramValues, budget);
        fillSampleInfo(info, sample.info);
        QVariantMap columns;
        columns[ QStringLiteral("__cat") ] = toVariantList(sample.labels);
        columns[ QStringLiteral("__val") ] = toVariantList(sample.values);
        result = DAPyScripts::getStatistics().aggregateByCategoryValueLists(
            columns, QStringLiteral("__cat"), QStringLiteral("__val"), args);
    }
    if (info) {
        info->approximate = info->approximate || !exact;
    }
    *categories = dictToStringList(result, "categories");
    *values     = dictToDoubleVector(result, "values");
    if (categories->isEmpty() || categories->size() != values->size()) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "No valid data for the multi-bar chart"));  // cn:没有可用于多重柱状图的数据
        return false;
    }
    return true;
}

// 多重柱状：分类 + 2+ 数值列，按类别聚合后每个类别一组柱
bool buildMultiBar(const DATableChartRequest& request,
                   const DATableChartSpec& spec,
                   DAChartWidget* chart,
                   DATableChartBuildInfo* info,
                   QString* error,
                   DAChartSeriesSampler::SampleBudget* budget)
{
    if (spec.categoryColumn < 0 || spec.valueColumns.size() < 2) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "The selected columns cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    QStringList categories;
    QList< QVector< double > > seriesValues;
    QStringList seriesNames;
    for (int i = 0; i < spec.valueColumns.size(); ++i) {
        QStringList valueCategories;
        QVector< double > values;
        if (!aggregateCategoryValues(request,
                                     spec.categoryColumn,
                                     spec.valueColumns.at(i),
                                     spec,
                                     info,
                                     error,
                                     budget,
                                     &valueCategories,
                                     &values)) {
            return false;
        }
        seriesNames.append(columnDisplayName(request, spec.valueColumns.at(i)));
        if (i == 0) {
            categories = valueCategories;
            seriesValues.append(values);
        } else {
            // 后续序列按类别名对齐到第一列的类别顺序，缺失的类别补 0
            QHash< QString, double > lookup;
            lookup.reserve(valueCategories.size());
            for (int c = 0; c < valueCategories.size(); ++c) {
                lookup.insert(valueCategories.at(c), values.at(c));
            }
            QVector< double > aligned;
            aligned.reserve(categories.size());
            for (const QString& category : std::as_const(categories)) {
                aligned.append(lookup.value(category, 0.0));
            }
            seriesValues.append(aligned);
        }
    }

    const int totalCategories = categories.size();
    if (spec.categoryTopN > 0 && totalCategories > kCategoryGuardThreshold
        && totalCategories > spec.categoryTopN) {
        categories = categories.mid(0, spec.categoryTopN);
        for (QVector< double >& values : seriesValues) {
            values = values.mid(0, spec.categoryTopN);
        }
    }
    if (info) {
        info->totalCategories     = totalCategories;
        info->displayedCategories = categories.size();
    }

    QVector< QwtSetSample > samples;
    samples.reserve(categories.size());
    for (int i = 0; i < categories.size(); ++i) {
        QVector< double > set;
        set.reserve(seriesValues.size());
        for (const QVector< double >& values : std::as_const(seriesValues)) {
            set.append(i < values.size() ? values.at(i) : 0.0);
        }
        samples.append(QwtSetSample(static_cast< double >(i), set));
    }

    const QString categoryName = columnDisplayName(request, spec.categoryColumn);
    DAChartAddMultiBarWidget widget;
    widget.setProgrammaticSamples(
        samples,
        seriesNames,
        categories,
        QCoreApplication::translate("DAChartQuickPlot", "%1 by %2")
            .arg(seriesNames.join(QStringLiteral(" / ")), categoryName));  // cn:%1 按 %2 聚合
    QwtPlotItem* item = widget.createPlotItem();
    if (!item) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to build the multi-bar chart"));  // cn:构建多重柱状图失败
        return false;
    }
    item->attach(chart);
    chart->setAxisScaleDraw(QwtAxis::XBottom, new QuickPlotCategoryScaleDraw(categories));
    chart->setAxisScale(QwtAxis::XBottom, -0.5, categories.size() - 0.5);
    applyAxisTitles(chart, categoryName, aggregationDisplayName(aggregationOf(spec)));
    return true;
}

// 3D 曲面/线图的公共数据准备
bool sample3DData(const DATableChartRequest& request,
                  const DATableChartSpec& spec,
                  int maxPoints,
                  DATableChartBuildInfo* info,
                  QString* error,
                  DAChartSeriesSampler::ColumnsSample* sample)
{
    DAChartSeriesSampler::SampleBudget budget;
    if (!sampleNumericColumns(request,
                              { spec.xColumn, spec.yColumn, spec.zColumn },
                              maxPoints,
                              info,
                              error,
                              &budget,
                              sample)) {
        return false;
    }
    if (sample->rowCount() < 2) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "Not enough valid rows for the 3D chart"));  // cn:有效数据行不足以绘制 3D 图
        return false;
    }
    return true;
}

}  // namespace

/**
 * @brief 把规格渲染到指定图表
 */
bool DAChartQuickPlot::render(const DATableChartRequest& request,
                              const DATableChartSpec& spec,
                              DAChartWidget* chart,
                              DATableChartBuildInfo* info,
                              QString* error,
                              bool clearExisting,
                              QList< QwtPlotItem* >* createdItems)
{
    if (!chart) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "There is no chart to draw on"));  // cn:没有可用的图表
        return false;
    }
    if (!request.isValid()) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The current selection cannot be plotted"));  // cn:当前选区无法绘图
        return false;
    }
    if (!spec.isValid() || spec.type == DATableChartType::None) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The chart specification is incomplete"));  // cn:绘图规格不完整
        return false;
    }

    // 取数与图元构建是同步的，期间给用户一个等待光标（实施方案 §8.4）
    DAWaitCursorScoped waitCursor;
    if (clearExisting) {
        chart->detachItems(QwtPlotItem::Rtti_PlotItem, true);
        resetAxes(chart);
    }
    const QwtPlotItemList itemsBefore = chart->itemList();

    // 同一次渲染的各列取数共用一个预算：任一列触及上限即整体标注「样本受限」
    DAChartSeriesSampler::SampleBudget budget;
    bool ok = false;
    switch (spec.type) {
    case DATableChartType::Line:
    case DATableChartType::Scatter:
    case DATableChartType::MultiLine:
        ok = buildLineLike(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::Histogram:
        ok = buildHistogram(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::Boxplot:
        ok = buildBoxplot(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::Bar:
        ok = buildBar(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::Contour:
        ok = buildContour(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::ErrorBar:
        ok = buildErrorBar(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::VectorField:
        ok = buildVectorField(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::MultiBar:
        ok = buildMultiBar(request, spec, chart, info, error, &budget);
        break;
    case DATableChartType::Surface3D:
    case DATableChartType::Line3D:
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "The 3D chart must be drawn on a 3D chart widget"));  // cn:3D 图需要绘制在 3D 图表控件上
        return false;
    case DATableChartType::None:
        break;
    }
    if (!ok) {
        return false;
    }

    if (createdItems) {
        const QwtPlotItemList itemsAfter = chart->itemList();
        for (QwtPlotItem* item : std::as_const(itemsAfter)) {
            if (!itemsBefore.contains(item)) {
                createdItems->append(item);
            }
        }
    }
    chart->replot();
    return true;
}

/**
 * @brief 图型是否走 3D 视图
 */
bool DAChartQuickPlot::is3DType(DATableChartType type)
{
    return type == DATableChartType::Surface3D || type == DATableChartType::Line3D;
}

/**
 * @brief 构建 3D 图元（不挂载）
 *
 * 3D 曲面走「散点 + scipy 插值成 30×30 网格」，scipy 不可用时给出可读原因；
 * 3D 线图直接使用采样后的轨迹点。fullData=true 时不做采样（转入全量重建）。
 */
Qwt3DPlotItem* DAChartQuickPlot::create3DItem(const DATableChartRequest& request,
                                              const DATableChartSpec& spec,
                                              DATableChartBuildInfo* info,
                                              QString* error,
                                              bool fullData)
{
    if (!request.isValid() || !spec.isValid() || !is3DType(spec.type)) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "The chart specification is incomplete"));  // cn:绘图规格不完整
        return nullptr;
    }
    const int maxPoints = fullData ? DAChartSeriesSampler::kFullDataPoints : DAChartSeriesSampler::kMaxLinePoints;
    DAChartSeriesSampler::ColumnsSample sample;
    if (!sample3DData(request, spec, maxPoints, info, error, &sample)) {
        return nullptr;
    }
    if (spec.type == DATableChartType::Surface3D) {
        DAChartAdd3DSurfaceWidget widget;
        widget.setProgrammaticScatterData(sample.columns.at(0), sample.columns.at(1), sample.columns.at(2));
        Qwt3DPlotItem* item = widget.create3DPlotItem();
        if (!item) {
            const QString reason = widget.programmaticError();
            setError(error,
                     reason.isEmpty()
                         ? QCoreApplication::translate("DAChartQuickPlot",
                                                       "Failed to build the 3D surface")  // cn:构建 3D 曲面失败
                         : reason);
            return nullptr;
        }
        return item;
    }

    QVector< QwtPoint3D > points;
    points.reserve(sample.rowCount());
    for (int i = 0; i < sample.rowCount(); ++i) {
        points.append(QwtPoint3D(sample.columns.at(0).at(i), sample.columns.at(1).at(i), sample.columns.at(2).at(i)));
    }
    DAChartAdd3DLineWidget widget;
    widget.setProgrammaticPoints(points);
    Qwt3DPlotItem* item = widget.create3DPlotItem();
    if (!item) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to build the 3D line chart"));  // cn:构建 3D 线图失败
        return nullptr;
    }
    return item;
}

/**
 * @brief 把规格渲染到 3D 图表（浮窗预览用）
 */
bool DAChartQuickPlot::render3D(const DATableChartRequest& request,
                                const DATableChartSpec& spec,
                                DAChart3DWidget* chart3d,
                                DATableChartBuildInfo* info,
                                QString* error,
                                bool fullData)
{
    if (!chart3d) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "There is no 3D chart to draw on"));  // cn:没有可用的 3D 图表
        return false;
    }
    DAWaitCursorScoped waitCursor;
    chart3d->clearAllData3D();
    Qwt3DPlotItem* item = create3DItem(request, spec, info, error, fullData);
    if (!item) {
        chart3d->update();
        return false;
    }
    chart3d->attach(item);
    chart3d->set3DAxisLabel(X1, columnDisplayName(request, spec.xColumn));
    chart3d->set3DAxisLabel(Y1, columnDisplayName(request, spec.yColumn));
    chart3d->set3DAxisLabel(Z1, columnDisplayName(request, spec.zColumn));
    chart3d->setChart3DTitle(QCoreApplication::translate("DAChartQuickPlot", "%1 · %2 · %3")
                                 .arg(columnDisplayName(request, spec.xColumn),
                                      columnDisplayName(request, spec.yColumn),
                                      columnDisplayName(request, spec.zColumn)));
    chart3d->resetView();
    chart3d->update();
    return true;
}

/**
 * @brief 转入绘图区（新建 Figure 或追加到当前 chart）
 *
 * 新建 Figure 时把「创建 figure + 建 chart + 挂图元」包成一个可撤销宏，
 * 压在这张新图自己的栈上；追加到当前图表时把新图元逐个登记为既有撤销命令。
 */
DAChartWidget* DAChartQuickPlot::transferToFigure(const DATableChartRequest& request,
                                                  const DATableChartSpec& spec,
                                                  DAChartOperateWidget* chartOperate,
                                                  bool appendToCurrent,
                                                  DATableChartBuildInfo* info,
                                                  QString* error)
{
    if (!chartOperate) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "The chart area is not available"));  // cn:绘图区不可用
        return nullptr;
    }

    if (!appendToCurrent) {
        DAFigureWidget* figure = chartOperate->createFigure();
        if (!figure) {
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to create a figure"));  // cn:创建绘图失败
            return nullptr;
        }
        QUndoStack* stack = figure->getUndoStack();
        if (stack) {
            stack->beginMacro(QCoreApplication::translate("DAChartQuickPlot", "quick plot"));  // cn:快速绘图
            stack->push(new DAChartOperateWidgetCommandDetachFigure(chartOperate, figure, true));
        }
        DAChartWidget* chart = figure->createChart_();
        if (!chart) {
            if (stack) {
                stack->endMacro();
                stack->undo();
            }
            chartOperate->removeFigure(figure, true);
            setError(error, QCoreApplication::translate("DAChartQuickPlot", "Failed to create a chart"));  // cn:创建子图失败
            return nullptr;
        }
        QList< QwtPlotItem* > created;
        if (!render(request, spec, chart, info, error, true, &created)) {
            if (stack) {
                stack->endMacro();
                stack->undo();  // 回滚整个宏：chart + figure 一起撤掉
            }
            // 撤销后 figure 仍保活（栈还在），失败路径要彻底移除，避免留下空图
            chartOperate->removeFigure(figure, true);
            return nullptr;
        }
        for (QwtPlotItem* item : std::as_const(created)) {
            figure->addItem_(chart, item, true);
        }
        if (stack) {
            stack->endMacro();
        }
        chartOperate->setCurrentFigure(figure);
        return chart;
    }

    DAFigureWidget* figure = chartOperate->getCurrentFigure();
    DAChartWidget* chart   = chartOperate->getCurrentChart();
    if (!figure || !chart) {
        setError(error, QCoreApplication::translate("DAChartQuickPlot", "There is no current chart to append to"));  // cn:当前没有可追加的图表
        return nullptr;
    }
    QUndoStack* stack = figure->getUndoStack();
    if (stack) {
        stack->beginMacro(QCoreApplication::translate("DAChartQuickPlot", "quick plot"));  // cn:快速绘图
    }
    QList< QwtPlotItem* > created;
    if (!render(request, spec, chart, info, error, false, &created)) {
        if (stack) {
            stack->endMacro();
            stack->undo();
        }
        return nullptr;
    }
    for (QwtPlotItem* item : std::as_const(created)) {
        figure->addItem_(chart, item, true);
    }
    if (stack) {
        stack->endMacro();
    }
    return chart;
}

/**
 * @brief 转入绘图区（3D 图型专用）
 *
 * appendToCurrent=false：新建 Figure + 新建 3D 图表 + 挂图元，整体一个撤销宏；
 * appendToCurrent=true：追加到当前 Figure 的当前 3D 图表（没有则在同一 Figure 内新建）。
 * 3D 图元按全量数据重建（不受预览采样预算限制）。
 */
bool DAChartQuickPlot::transferToFigure3D(const DATableChartRequest& request,
                                          const DATableChartSpec& spec,
                                          DAChartOperateWidget* chartOperate,
                                          bool appendToCurrent,
                                          DATableChartBuildInfo* info,
                                          QString* error)
{
    if (!chartOperate) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "The chart area is not available"));  // cn:绘图区不可用
        return false;
    }
    if (!is3DType(spec.type)) {
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot",
                                             "The chart type is not a 3D chart"));  // cn:当前图型不是 3D 图型
        return false;
    }

    DAFigureWidget* figure = appendToCurrent ? chartOperate->getCurrentFigure() : chartOperate->createFigure();
    if (!figure) {
        setError(error,
                 appendToCurrent
                     ? QCoreApplication::translate("DAChartQuickPlot",
                                                   "There is no current chart to append to")  // cn:当前没有可追加的图表
                     : QCoreApplication::translate("DAChartQuickPlot", "Failed to create a figure"));  // cn:创建绘图失败
        return false;
    }
    QUndoStack* stack = figure->getUndoStack();
    if (stack) {
        stack->beginMacro(QCoreApplication::translate("DAChartQuickPlot", "quick plot"));  // cn:快速绘图
        if (!appendToCurrent) {
            stack->push(new DAChartOperateWidgetCommandDetachFigure(chartOperate, figure, true));
        }
    }
    DAChart3DWidget* chart3d = appendToCurrent ? figure->getCurrent3DChart() : nullptr;
    if (!chart3d) {
        chart3d = figure->create3DChart_();
    }
    if (!chart3d) {
        if (stack) {
            stack->endMacro();
            stack->undo();
        }
        if (!appendToCurrent) {
            chartOperate->removeFigure(figure, true);
        }
        setError(error,
                 QCoreApplication::translate("DAChartQuickPlot", "Failed to create a 3D chart"));  // cn:创建 3D 子图失败
        return false;
    }
    Qwt3DPlotItem* item = create3DItem(request, spec, info, error, true);
    if (!item) {
        if (stack) {
            stack->endMacro();
            stack->undo();  // 回滚整个宏：3D chart + figure 一起撤掉
        }
        if (!appendToCurrent) {
            chartOperate->removeFigure(figure, true);
        }
        return false;
    }
    chart3d->set3DAxisLabel(X1, columnDisplayName(request, spec.xColumn));
    chart3d->set3DAxisLabel(Y1, columnDisplayName(request, spec.yColumn));
    chart3d->set3DAxisLabel(Z1, columnDisplayName(request, spec.zColumn));
    // 与 2D 转入同一模式：先 attach（新建图元尚未挂载），再以 skipFirst=true
    // 登记撤销命令（undo 时 detach + 标记删除）
    chart3d->attach(item);
    figure->add3DItem_(chart3d, item, true);
    if (stack) {
        stack->endMacro();
    }
    chartOperate->setCurrentFigure(figure);
    return true;
}

}  // namespace DA
