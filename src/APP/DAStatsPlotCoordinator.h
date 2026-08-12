#pragma once
#include <QString>
#include <QJsonObject>
#include <QPolygonF>
#include <QVector>
#include <QPointF>
#include <QVariantMap>
#include <QStringList>
#include "DAData.h"

// Forward declarations — keep pybind11 out of the header
namespace DA {
class DAFigureWidget;
class DAChartWidget;
class DAPyDataFrame;
}
class DAChartPlotRenderer;
namespace pybind11 {
class dict;
}

/**
 * @brief Orchestrator that connects statistics computation (DAPyScriptsStatistics)
 *        with chart rendering (DAChartPlotRenderer).
 *
 * The coordinator receives a plot type + params + chart + data, calls the
 * appropriate atomic statistics functions, extracts the results into Qwt
 * native types, and calls the renderer.
 *
 * @note The caller must hold the Python GIL before calling execute().
 *       The coordinator itself does NOT acquire the GIL — that responsibility
 *       belongs to the caller (e.g. DAAppController).
 *       The coordinator does NOT use attr(), module_::import, or
 *       gil_scoped_acquire — all Python interaction is encapsulated in
 *       DAPyScriptsStatistics. Extracting data from the returned
 *       pybind11::dict is allowed (it is consuming return values, not
 *       calling Python).
 */
class DAStatsPlotCoordinator
{
public:
    DAStatsPlotCoordinator();
    ~DAStatsPlotCoordinator();

    // Execute a statistical plot orchestration
    bool execute(const QString& plotType, const QJsonObject& params,
                 DA::DAFigureWidget* fig, DA::DAChartWidget* chart,
                 const DA::DAData& data);

private:
    // Per-plot-type orchestration
    void plotHistplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotKdeplot1d(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotKdeplot2d(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotBoxplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotHeatmap(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotScatterplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotBarplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotRegplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotEcdfplot(const QJsonObject& params, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);

    // Helpers: extract data from pybind11::dict to Qwt types
    QVector<double> dictToDoubleVector(const pybind11::dict& d, const QString& key);
    QPolygonF dictToPolygon(const pybind11::dict& d, const QString& xKey, const QString& yKey);
    QVector<QPointF> dictToPoints(const pybind11::dict& d, const QString& xKey, const QString& yKey);
    QStringList dictToStringList(const pybind11::dict& d, const QString& key);

    // Helper: QJsonObject -> QVariantMap (skip __ private fields)
    QVariantMap toVariantMap(const QJsonObject& params, bool skipPrivate = true) const;
};
