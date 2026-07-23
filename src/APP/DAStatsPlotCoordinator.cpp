#include "DAStatsPlotCoordinator.h"

// pybind11 — needed only in the .cpp for dict access (consuming return values)
#include <pybind11/pybind11.h>

#include "DAChartPlotRenderer.h"
#include "DAPyScripts.h"
#include "DALogCategory.h"

// DAPyDataFrame and DAPySeries are available transitively via
// DAPyScripts.h -> DAPyScriptsStatistics.h -> pandas/DAPyDataFrame.h -> DAPySeries.h
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"

#include <QDebug>
#include <QColor>
#include <QtGlobal>

// ============================================================
// Constructor / Destructor
// ============================================================

DAStatsPlotCoordinator::DAStatsPlotCoordinator() {}
DAStatsPlotCoordinator::~DAStatsPlotCoordinator() {}

// ============================================================
// Public entry point
// ============================================================

bool DAStatsPlotCoordinator::execute(const QString& plotType,
                                      const QJsonObject& params,
                                      DA::DAFigureWidget* fig,
                                      DA::DAChartWidget* chart,
                                      const DA::DAData& data)
{
    Q_UNUSED(fig);

    if (!chart || !data) {
        return false;
    }

    DA::DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        daWarning << QObject::tr("The selected data source is empty");  // cn: 选中的数据源为空
        return false;
    }

    DAChartPlotRenderer renderer(chart);

    try {
        if (plotType == "histplot") {
            plotHistplot(params, renderer, df);
        } else if (plotType == "kdeplot_1d") {
            plotKdeplot1d(params, renderer, df);
        } else if (plotType == "kdeplot_2d") {
            plotKdeplot2d(params, renderer, df);
        } else if (plotType == "boxplot") {
            plotBoxplot(params, renderer, df);
        } else if (plotType == "heatmap") {
            plotHeatmap(params, renderer, df);
        } else if (plotType == "scatterplot") {
            plotScatterplot(params, renderer, df);
        } else if (plotType == "barplot") {
            plotBarplot(params, renderer, df);
        } else if (plotType == "regplot") {
            plotRegplot(params, renderer, df);
        } else if (plotType == "ecdfplot") {
            plotEcdfplot(params, renderer, df);
        } else {
            daWarning << QObject::tr("Unknown plot type: %1").arg(plotType);  // cn: 未知的绘图类型: %1
            return false;
        }

        renderer.replot();
        return true;
    } catch (const pybind11::error_already_set& e) {
        daCritical << QObject::tr("Python error in statistical plot: %1").arg(e.what());  // cn: 统计绘图 Python 错误: %1
    } catch (const std::exception& e) {
        daCritical << QObject::tr("Error in statistical plot: %1").arg(e.what());  // cn: 统计绘图错误: %1
    }
    return false;
}

// ============================================================
// histplot: computeHistogram → renderHistogram
//           if kde=true → computeKde1d → renderCurve
// ============================================================

void DAStatsPlotCoordinator::plotHistplot(const QJsonObject& params,
                                           DAChartPlotRenderer& renderer,
                                           const DA::DAPyDataFrame& df)
{
    QString column = params.value("column").toString();
    if (column.isEmpty()) return;

    QString hueCol = params.value("hue").toString();
    QVariantMap args = toVariantMap(params);

    // Color palette for hue groups (matplotlib tab10)
    static const QVector<QColor> kHuePalette = {
        QColor(31, 119, 180),   // blue
        QColor(255, 127, 14),   // orange
        QColor(44, 160, 44),    // green
        QColor(214, 39, 40),    // red
        QColor(148, 103, 189),  // purple
        QColor(140, 86, 75),    // brown
        QColor(227, 119, 194),  // pink
        QColor(127, 127, 127),  // gray
        QColor(188, 189, 34),   // olive
        QColor(23, 190, 207),   // cyan
    };

    if (hueCol.isEmpty()) {
        // === No hue: single histogram (existing behaviour) ===
        DA::DAPySeries series = df[column];
        if (series.isNone()) {
            daWarning << QObject::tr("Column '%1' not found in data").arg(column);  // cn: 数据中找不到列 '%1'
            return;
        }

        QString err;
        pybind11::dict result = DA::DAPyScripts::getStatistics().computeHistogram(series, args, &err);
        if (result.is_none() || pybind11::len(result) == 0) {
            daWarning << QObject::tr("Failed to compute histogram for column '%1': %2").arg(column, err);  // cn: 计算列 '%1' 的直方图失败: %2
            return;
        }

        QVector<double> counts = dictToDoubleVector(result, "counts");
        QVector<double> edges   = dictToDoubleVector(result, "edges");
        if (counts.isEmpty() || edges.isEmpty()) {
            daWarning << QObject::tr("Histogram result is empty for column '%1'").arg(column);  // cn: 列 '%1' 的直方图结果为空
            return;
        }

        renderer.renderHistogram(edges, counts);

        // Optional KDE overlay (atomic: separate statistics call)
        if (params.value("kde").toBool(false)) {
            QString kdeErr;
            pybind11::dict kdeResult = DA::DAPyScripts::getStatistics().computeKde1d(series, args, &kdeErr);
            if (!kdeResult.is_none() && pybind11::len(kdeResult) > 0) {
                QPolygonF kdeCurve = dictToPolygon(kdeResult, "x", "y");
                if (!kdeCurve.isEmpty()) {
                    renderer.renderCurve(kdeCurve);
                }
            } else if (!kdeErr.isEmpty()) {
                daWarning << QObject::tr("KDE overlay failed for column '%1': %2").arg(column, kdeErr);  // cn: 列 '%1' 的 KDE 叠加失败: %2
            }
        }
        return;
    }

    // === Hue grouping: multiple histograms with shared bin edges ===
    QString histErr;
    pybind11::dict result = DA::DAPyScripts::getStatistics().computeHistogramByHue(
        df, column, hueCol, args, &histErr);
    if (result.is_none() || pybind11::len(result) == 0) {
        daWarning << QObject::tr("Failed to compute grouped histogram (column '%1', hue '%2'): %3")
                         .arg(column, hueCol, histErr);  // cn: 计算分组直方图失败 (列 '%1', 分组 '%2'): %3
        return;
    }

    // Extract groups list from result
    pybind11::str groupsKey("groups");
    if (!result.contains(groupsKey)) return;
    pybind11::object groupsObj = result[groupsKey];
    if (groupsObj.is_none() || !pybind11::isinstance<pybind11::list>(groupsObj)) return;

    auto groupsList = pybind11::cast<pybind11::list>(groupsObj);
    int groupCount  = static_cast<int>(groupsList.size());

    for (int i = 0; i < groupCount; ++i) {
        pybind11::object groupObj = groupsList[i];
        if (!pybind11::isinstance<pybind11::dict>(groupObj)) continue;
        auto groupDict = pybind11::cast<pybind11::dict>(groupObj);

        QVector<double> counts = dictToDoubleVector(groupDict, "counts");
        QVector<double> edges   = dictToDoubleVector(groupDict, "edges");
        if (counts.isEmpty() || edges.isEmpty()) continue;

        // Assign a colour from the palette and build style
        const QColor& base = kHuePalette[i % kHuePalette.size()];
        QVariantMap style;
        style["color"]     = base;
        style["fillColor"] = QColor(base.red(), base.green(), base.blue(), 80);

        // Extract hue label for the item title
        pybind11::str labelKey("hue_label");
        if (groupDict.contains(labelKey)) {
            pybind11::object labelObj = groupDict[labelKey];
            if (!labelObj.is_none()) {
                style["title"] = QString::fromStdString(
                    pybind11::str(labelObj).cast<std::string>());
            }
        }

        renderer.renderHistogram(edges, counts, style);
    }

    // Optional KDE overlay per group (atomic: separate statistics call)
    if (params.value("kde").toBool(false)) {
        QString kdeErr;
        pybind11::dict kdeResult = DA::DAPyScripts::getStatistics().computeKde1dByHue(
            df, column, hueCol, args, &kdeErr);
        if (kdeResult.is_none() || pybind11::len(kdeResult) == 0) {
            if (!kdeErr.isEmpty()) {
                daWarning << QObject::tr("KDE overlay failed for hue groups: %1").arg(kdeErr);  // cn: 分组 KDE 叠加失败: %1
            }
            return;
        }

        pybind11::str kdeGroupsKey("groups");
        if (!kdeResult.contains(kdeGroupsKey)) return;
        pybind11::object kdeGroupsObj = kdeResult[kdeGroupsKey];
        if (kdeGroupsObj.is_none() || !pybind11::isinstance<pybind11::list>(kdeGroupsObj)) return;

        auto kdeGroupsList = pybind11::cast<pybind11::list>(kdeGroupsObj);
        for (int i = 0; i < static_cast<int>(kdeGroupsList.size()); ++i) {
            pybind11::object groupObj = kdeGroupsList[i];
            if (!pybind11::isinstance<pybind11::dict>(groupObj)) continue;
            auto groupDict = pybind11::cast<pybind11::dict>(groupObj);

            // y is None when KDE could not be computed for a group
            pybind11::str yKey("y");
            if (!groupDict.contains(yKey)) continue;
            pybind11::object yObj = groupDict[yKey];
            if (yObj.is_none()) continue;

            QPolygonF kdeCurve = dictToPolygon(groupDict, "x", "y");
            if (kdeCurve.isEmpty()) continue;

            const QColor& base = kHuePalette[i % kHuePalette.size()];
            QVariantMap style;
            style["color"] = base;
            renderer.renderCurve(kdeCurve, style);
        }
    }
}

// ============================================================
// kdeplot_1d: computeKde1d → renderCurve
//             if fill=true → construct closed polygon → renderShape
// ============================================================

void DAStatsPlotCoordinator::plotKdeplot1d(const QJsonObject& params,
                                            DAChartPlotRenderer& renderer,
                                            const DA::DAPyDataFrame& df)
{
    QString column = params.value("column").toString();
    if (column.isEmpty()) return;

    DA::DAPySeries series = df[column];
    if (series.isNone()) return;

    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().computeKde1d(series, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: x (list), y (list)
    QPolygonF curve = dictToPolygon(result, "x", "y");
    if (curve.isEmpty()) return;

    renderer.renderCurve(curve);

    // Optional fill: construct a closed polygon (baseline → curve → baseline)
    if (params.value("fill").toBool(false)) {
        QPolygonF fillPoly;
        fillPoly.append(QPointF(curve.first().x(), 0.0));
        fillPoly.append(curve);
        fillPoly.append(QPointF(curve.last().x(), 0.0));
        if (fillPoly.size() >= 3) {
            renderer.renderShape(fillPoly);
        }
    }
}

// ============================================================
// kdeplot_2d: computeKde2d → renderSpectrogram
//             if contour=true → computeContours → renderContours
// ============================================================

void DAStatsPlotCoordinator::plotKdeplot2d(const QJsonObject& params,
                                            DAChartPlotRenderer& renderer,
                                            const DA::DAPyDataFrame& df)
{
    QString xCol = params.value("x_column").toString();
    QString yCol = params.value("y_column").toString();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    DA::DAPySeries xSeries = df[xCol];
    DA::DAPySeries ySeries = df[yCol];
    if (xSeries.isNone() || ySeries.isNone()) return;

    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().computeKde2d(xSeries, ySeries, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: Z (list of list), x_grid (list), y_grid (list)
    QVector<double> xGrid = dictToDoubleVector(result, "x_grid");
    QVector<double> yGrid = dictToDoubleVector(result, "y_grid");
    if (xGrid.isEmpty() || yGrid.isEmpty()) return;

    int nrows = yGrid.size();
    int ncols = xGrid.size();

    // Extract Z matrix and flatten (row-major: Z[row][col])
    pybind11::str zKey("Z");
    if (!result.contains(zKey)) return;
    pybind11::object zObj = result[zKey];
    if (zObj.is_none() || !pybind11::isinstance<pybind11::list>(zObj)) return;

    auto zRows = pybind11::cast<pybind11::list>(zObj);
    QVector<double> values;
    values.reserve(nrows * ncols);
    for (auto row : zRows) {
        if (!pybind11::isinstance<pybind11::list>(row)) continue;
        auto rowList = pybind11::cast<pybind11::list>(row);
        for (auto val : rowList) {
            if (val.is_none()) {
                values.append(qQNaN());
            } else {
                values.append(pybind11::cast<double>(val));
            }
        }
    }

    DA::DASpectrogramData specData;
    specData.values = values;
    specData.nrows   = nrows;
    specData.ncols   = ncols;
    specData.xmin    = xGrid.first();
    specData.xmax    = xGrid.last();
    specData.ymin    = yGrid.first();
    specData.ymax    = yGrid.last();

    renderer.renderSpectrogram(specData);

    // Optional contour overlay (atomic: separate computeContours call)
    if (params.value("contour").toBool(false)) {
        DA::DAContourData contourData =
            DA::DAPyScripts::getStatistics().computeContours(result, args);
        if (!contourData.polygons.isEmpty()) {
            renderer.renderContours(contourData);
        }
    }
}

// ============================================================
// boxplot: computeBoxplotStats → construct DABoxPlotData → renderBoxChart
//          outliers → renderScatter
// ============================================================

void DAStatsPlotCoordinator::plotBoxplot(const QJsonObject& params,
                                          DAChartPlotRenderer& renderer,
                                          const DA::DAPyDataFrame& df)
{
    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().computeBoxplotStats(df, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: samples (list of dict), each dict has:
    //   position, whisker_lower, q1, median, q3, whisker_upper, outliers, mean
    pybind11::str samplesKey("samples");
    if (!result.contains(samplesKey)) return;
    pybind11::object samplesObj = result[samplesKey];
    if (samplesObj.is_none() || !pybind11::isinstance<pybind11::list>(samplesObj)) return;

    auto samplesList = pybind11::cast<pybind11::list>(samplesObj);

    DA::DABoxPlotData boxData;
    QVector<QPointF> allOutliers;

    for (auto sample : samplesList) {
        if (!pybind11::isinstance<pybind11::dict>(sample)) continue;
        auto sd = pybind11::cast<pybind11::dict>(sample);

        boxData.positions.append(sd["position"].cast<double>());
        boxData.q1.append(sd["q1"].cast<double>());
        boxData.median.append(sd["median"].cast<double>());
        boxData.q3.append(sd["q3"].cast<double>());
        boxData.whiskerLower.append(sd["whisker_lower"].cast<double>());
        boxData.whiskerUpper.append(sd["whisker_upper"].cast<double>());

        // Outliers for this box — collect for separate scatter rendering
        QVector<QPointF> boxOutliers;
        pybind11::object outObj = sd["outliers"];
        if (!outObj.is_none() && pybind11::isinstance<pybind11::list>(outObj)) {
            auto outList = pybind11::cast<pybind11::list>(outObj);
            double pos = sd["position"].cast<double>();
            for (auto val : outList) {
                if (!val.is_none()) {
                    boxOutliers.append(QPointF(pos, pybind11::cast<double>(val)));
                }
            }
        }
        boxData.outliers.append(boxOutliers);  // stored in data for completeness
        allOutliers.append(boxOutliers);
    }

    if (boxData.positions.isEmpty()) return;

    // Clear outliers from boxData so renderer doesn't duplicate; we render them as scatter
    for (int i = 0; i < boxData.outliers.size(); ++i) {
        boxData.outliers[i].clear();
    }

    renderer.renderBoxChart(boxData);

    if (!allOutliers.isEmpty()) {
        renderer.renderScatter(allOutliers);
    }
}

// ============================================================
// heatmap: computePivotMatrix → construct DASpectrogramData → renderSpectrogram
// ============================================================

void DAStatsPlotCoordinator::plotHeatmap(const QJsonObject& params,
                                          DAChartPlotRenderer& renderer,
                                          const DA::DAPyDataFrame& df)
{
    QString indexCol   = params.value("index_col").toString();
    QString columnsCol = params.value("columns_col").toString();
    QString valuesCol  = params.value("values_col").toString();
    if (indexCol.isEmpty() || columnsCol.isEmpty()) return;

    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().computePivotMatrix(
        df, indexCol, columnsCol, valuesCol, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: z (list of list), x_labels (list), y_labels (list),
    //                 n_rows, n_cols, vmin, vmax
    QStringList xLabels = dictToStringList(result, "x_labels");
    QStringList yLabels = dictToStringList(result, "y_labels");

    int nrows = yLabels.size();
    int ncols = xLabels.size();

    // Extract z matrix and flatten (row-major)
    pybind11::str zKey("z");
    if (!result.contains(zKey)) return;
    pybind11::object zObj = result[zKey];
    if (zObj.is_none() || !pybind11::isinstance<pybind11::list>(zObj)) return;

    auto zRows = pybind11::cast<pybind11::list>(zObj);
    QVector<double> values;
    values.reserve(nrows * ncols);
    for (auto row : zRows) {
        if (!pybind11::isinstance<pybind11::list>(row)) continue;
        auto rowList = pybind11::cast<pybind11::list>(row);
        for (auto val : rowList) {
            if (val.is_none()) {
                values.append(qQNaN());
            } else {
                values.append(pybind11::cast<double>(val));
            }
        }
    }

    DA::DASpectrogramData specData;
    specData.values    = values;
    specData.nrows      = nrows;
    specData.ncols      = ncols;
    specData.xmin       = 0;
    specData.xmax       = ncols > 0 ? ncols - 1 : 0;
    specData.ymin       = 0;
    specData.ymax       = nrows > 0 ? nrows - 1 : 0;
    specData.rowLabels  = yLabels;
    specData.colLabels  = xLabels;

    // Pass vmin/vmax and cmap via style
    QVariantMap style;
    pybind11::str vminKey("vmin");
    if (result.contains(vminKey)) {
        pybind11::object vminObj = result[vminKey];
        if (!vminObj.is_none()) {
            style["vmin"] = pybind11::cast<double>(vminObj);
        }
    }
    pybind11::str vmaxKey("vmax");
    if (result.contains(vmaxKey)) {
        pybind11::object vmaxObj = result[vmaxKey];
        if (!vmaxObj.is_none()) {
            style["vmax"] = pybind11::cast<double>(vmaxObj);
        }
    }
    if (params.contains("cmap")) {
        style["cmap"] = params.value("cmap").toString();
    }

    renderer.renderSpectrogram(specData, style);
}

// ============================================================
// scatterplot: extract x/y directly from df → renderScatter
//              hue groups → multiple renderScatter calls
// ============================================================

void DAStatsPlotCoordinator::plotScatterplot(const QJsonObject& params,
                                              DAChartPlotRenderer& renderer,
                                              const DA::DAPyDataFrame& df)
{
    QString xCol = params.value("x_column").toString();
    QString yCol = params.value("y_column").toString();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    DA::DAPySeries xSeries = df[xCol];
    DA::DAPySeries ySeries = df[yCol];
    if (xSeries.isNone() || ySeries.isNone()) return;

    QVector<double> xVals = DA::toQVectorDouble(xSeries);
    QVector<double> yVals = DA::toQVectorDouble(ySeries);

    QString hueCol = params.value("hue").toString();

    if (hueCol.isEmpty()) {
        // Simple scatter — no grouping
        int n = qMin(xVals.size(), yVals.size());
        QVector<QPointF> points;
        points.reserve(n);
        for (int i = 0; i < n; ++i) {
            points.append(QPointF(xVals[i], yVals[i]));
        }
        if (!points.isEmpty()) {
            renderer.renderScatter(points);
        }
    } else {
        // Group by hue column values
        DA::DAPySeries hueSeries = df[hueCol];
        if (hueSeries.isNone()) return;

        int n = qMin(qMin(xVals.size(), yVals.size()), static_cast<int>(hueSeries.size()));

        // Group points by hue value
        QHash<QString, QVector<QPointF>> groupedPoints;
        for (int i = 0; i < n; ++i) {
            QString hueVal = hueSeries.valueAsString(static_cast<std::size_t>(i));
            groupedPoints[hueVal].append(QPointF(xVals[i], yVals[i]));
        }

        // Render each group as a separate scatter
        for (auto it = groupedPoints.begin(); it != groupedPoints.end(); ++it) {
            if (!it.value().isEmpty()) {
                renderer.renderScatter(it.value());
            }
        }
    }
}

// ============================================================
// barplot: aggregateByCategory → construct DABarChartData
//          if ci>0 → computeCI → renderBarChart
// ============================================================

void DAStatsPlotCoordinator::plotBarplot(const QJsonObject& params,
                                          DAChartPlotRenderer& renderer,
                                          const DA::DAPyDataFrame& df)
{
    QString xCol = params.value("column").toString();
    QString yCol = params.value("y_column").toString();  // empty for count plot
    if (xCol.isEmpty()) return;

    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().aggregateByCategory(
        df, xCol, yCol, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: categories (list), values (list or list-of-lists if hue),
    //                 hue_categories (list or None)
    QStringList categories = dictToStringList(result, "categories");

    // Check if hue grouping is used
    pybind11::str hueKey("hue_categories");
    bool hasHue = result.contains(hueKey) &&
                  !result[hueKey].is_none() &&
                  pybind11::isinstance<pybind11::list>(result[hueKey]);

    if (!hasHue) {
        // Simple bar chart — values is a flat list
        DA::DABarChartData barData;
        barData.categories = categories;
        barData.values      = dictToDoubleVector(result, "values");
        if (barData.values.isEmpty()) return;

        // Optional CI (atomic: separate computeCI call)
        int ci = params.value("ci").toInt(0);
        if (ci > 0 && !yCol.isEmpty()) {
            QVariantMap ciArgs  = args;
            ciArgs["x_col"]     = xCol;
            ciArgs["y_col"]     = yCol;
            pybind11::dict ciResult = DA::DAPyScripts::getStatistics().computeCI(df, ciArgs);
            if (!ciResult.is_none() && pybind11::len(ciResult) > 0) {
                barData.ciLower = dictToDoubleVector(ciResult, "ci_lower");
                barData.ciUpper = dictToDoubleVector(ciResult, "ci_upper");
            }
        }

        renderer.renderBarChart(barData);
    } else {
        // Hue grouping — values is a list of lists, render each hue group separately
        QStringList hueCategories = dictToStringList(result, "hue_categories");

        pybind11::str valuesKey("values");
        if (!result.contains(valuesKey)) return;
        pybind11::object valuesObj = result[valuesKey];
        if (valuesObj.is_none() || !pybind11::isinstance<pybind11::list>(valuesObj)) return;

        auto valuesList = pybind11::cast<pybind11::list>(valuesObj);
        for (int h = 0; h < hueCategories.size() && h < static_cast<int>(valuesList.size()); ++h) {
            pybind11::object groupValsObj = valuesList[h];
            if (groupValsObj.is_none() || !pybind11::isinstance<pybind11::list>(groupValsObj))
                continue;

            auto groupVals = pybind11::cast<pybind11::list>(groupValsObj);
            DA::DABarChartData groupBarData;
            groupBarData.categories = categories;
            groupBarData.values.reserve(groupVals.size());
            for (auto val : groupVals) {
                if (!val.is_none()) {
                    groupBarData.values.append(pybind11::cast<double>(val));
                }
            }
            if (!groupBarData.values.isEmpty()) {
                renderer.renderBarChart(groupBarData);
            }
        }
    }
}

// ============================================================
// regplot: extract scatter → renderScatter
//          fitPolynomial → renderCurve
//          if ci>0 → computeBootstrapCI → renderIntervalCurve
// ============================================================

void DAStatsPlotCoordinator::plotRegplot(const QJsonObject& params,
                                          DAChartPlotRenderer& renderer,
                                          const DA::DAPyDataFrame& df)
{
    QString xCol = params.value("x_column").toString();
    QString yCol = params.value("y_column").toString();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    DA::DAPySeries xSeries = df[xCol];
    DA::DAPySeries ySeries = df[yCol];
    if (xSeries.isNone() || ySeries.isNone()) return;

    QVariantMap args = toVariantMap(params);

    // 1. Scatter (directly from DataFrame — no statistics function needed)
    QVector<double> xVals = DA::toQVectorDouble(xSeries);
    QVector<double> yVals = DA::toQVectorDouble(ySeries);
    int n = qMin(xVals.size(), yVals.size());
    QVector<QPointF> scatterPoints;
    scatterPoints.reserve(n);
    for (int i = 0; i < n; ++i) {
        scatterPoints.append(QPointF(xVals[i], yVals[i]));
    }
    if (!scatterPoints.isEmpty()) {
        renderer.renderScatter(scatterPoints);
    }

    // 2. Polynomial regression fit (atomic statistics function)
    pybind11::dict fitResult =
        DA::DAPyScripts::getStatistics().fitPolynomial(xSeries, ySeries, args);
    if (!fitResult.is_none() && pybind11::len(fitResult) > 0) {
        // Python returns: x_grid (list), y_pred (list)
        QPolygonF regLine = dictToPolygon(fitResult, "x_grid", "y_pred");
        if (!regLine.isEmpty()) {
            renderer.renderCurve(regLine);
        }
    }

    // 3. Optional confidence interval (independent atomic function)
    int ci = params.value("ci").toInt(95);
    if (ci > 0) {
        pybind11::dict ciResult =
            DA::DAPyScripts::getStatistics().computeBootstrapCI(xSeries, ySeries, args);
        if (!ciResult.is_none() && pybind11::len(ciResult) > 0) {
            // Python returns: x_grid, ci_lower (list or None), ci_upper (list or None)
            QVector<double> ciX     = dictToDoubleVector(ciResult, "x_grid");
            QVector<double> ciLower = dictToDoubleVector(ciResult, "ci_lower");
            QVector<double> ciUpper = dictToDoubleVector(ciResult, "ci_upper");
            if (!ciX.isEmpty() && !ciLower.isEmpty() && !ciUpper.isEmpty()) {
                DA::DAIntervalCurveData ciData;
                ciData.x      = ciX;
                ciData.yLower  = ciLower;
                ciData.yUpper  = ciUpper;
                renderer.renderIntervalCurve(ciData);
            }
        }
    }
}

// ============================================================
// ecdfplot: computeECDF → extractPolygon → renderCurve (step style)
// ============================================================

void DAStatsPlotCoordinator::plotEcdfplot(const QJsonObject& params,
                                           DAChartPlotRenderer& renderer,
                                           const DA::DAPyDataFrame& df)
{
    QString column = params.value("column").toString();
    if (column.isEmpty()) return;

    DA::DAPySeries series = df[column];
    if (series.isNone()) return;

    QVariantMap args = toVariantMap(params);

    pybind11::dict result = DA::DAPyScripts::getStatistics().computeECDF(series, args);
    if (result.is_none() || pybind11::len(result) == 0) return;

    // Python returns: x (list), y (list)
    QPolygonF curve = dictToPolygon(result, "x", "y");
    if (curve.isEmpty()) return;

    QVariantMap style;
    style["curveStyle"] = QStringLiteral("Steps");
    renderer.renderCurve(curve, style);
}

// ============================================================
// Helper methods
// ============================================================

QVariantMap DAStatsPlotCoordinator::toVariantMap(const QJsonObject& params, bool skipPrivate) const
{
    QVariantMap result;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (skipPrivate && it.key().startsWith("__")) continue;
        result[it.key()] = it.value().toVariant();
    }
    return result;
}

QVector<double> DAStatsPlotCoordinator::dictToDoubleVector(const pybind11::dict& d,
                                                            const QString& key)
{
    QVector<double> result;
    if (d.is_none()) return result;

    pybind11::str k(key.toStdString());
    if (!d.contains(k)) return result;

    pybind11::object val = d[k];
    if (val.is_none()) return result;

    if (pybind11::isinstance<pybind11::list>(val)) {
        auto list = pybind11::cast<pybind11::list>(val);
        result.reserve(static_cast<int>(list.size()));
        for (auto item : list) {
            if (!item.is_none()) {
                result.append(pybind11::cast<double>(item));
            }
        }
    }
    return result;
}

QPolygonF DAStatsPlotCoordinator::dictToPolygon(const pybind11::dict& d,
                                                 const QString& xKey,
                                                 const QString& yKey)
{
    QPolygonF result;
    if (d.is_none()) return result;

    QVector<double> xVals = dictToDoubleVector(d, xKey);
    QVector<double> yVals = dictToDoubleVector(d, yKey);

    int n = qMin(xVals.size(), yVals.size());
    result.reserve(n);
    for (int i = 0; i < n; ++i) {
        result.append(QPointF(xVals[i], yVals[i]));
    }
    return result;
}

QVector<QPointF> DAStatsPlotCoordinator::dictToPoints(const pybind11::dict& d,
                                                        const QString& xKey,
                                                        const QString& yKey)
{
    QPolygonF poly = dictToPolygon(d, xKey, yKey);
    return QVector<QPointF>(poly.begin(), poly.end());
}

QStringList DAStatsPlotCoordinator::dictToStringList(const pybind11::dict& d,
                                                      const QString& key)
{
    QStringList result;
    if (d.is_none()) return result;

    pybind11::str k(key.toStdString());
    if (!d.contains(k)) return result;

    pybind11::object val = d[k];
    if (val.is_none()) return result;

    if (pybind11::isinstance<pybind11::list>(val)) {
        auto list = pybind11::cast<pybind11::list>(val);
        for (auto item : list) {
            if (!item.is_none()) {
                result.append(QString::fromStdString(
                    pybind11::str(item).cast<std::string>()));
            }
        }
    }
    return result;
}
