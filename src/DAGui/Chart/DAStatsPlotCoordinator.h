#pragma once
#include "DAGuiAPI.h"
#include <QString>
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
class DAStatsPlotRequest;
class DAStatsHistplotRequest;
class DAStatsKdeplot1dRequest;
class DAStatsKdeplot2dRequest;
class DAStatsBoxplotRequest;
class DAStatsHeatmapRequest;
class DAStatsScatterplotRequest;
class DAStatsBarplotRequest;
class DAStatsRegplotRequest;
class DAStatsEcdfplotRequest;
}
class DAChartPlotRenderer;
namespace pybind11 {
class dict;
}

/**
 * @brief Orchestrator that connects statistics computation (DAPyScriptsStatistics)
 *        with chart rendering (DAChartPlotRenderer).
 *
 * The coordinator receives a typed DAStatsPlotRequest, extracts the dataframe,
 * dispatches to the per-type plotXxx orchestration via the request's
 * dispatchTo (double dispatch), and renders via DAChartPlotRenderer.
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
class DAGUI_API DAStatsPlotCoordinator
{
public:
    DAStatsPlotCoordinator();
    ~DAStatsPlotCoordinator();

    // Execute a statistical plot orchestration (double dispatch via req.dispatchTo)
    bool execute(const DA::DAStatsPlotRequest& req);

    /**
     * @brief 把 compute_boxplot_stats 的返回结果渲染成箱线图
     *
     * 「Python 统计结果 → 箱体 + 离群点 + 类别轴」的唯一实现，统计向导与
     * 快速绘图的箱线图都走这里，避免两套箱体构建逻辑。
     * @param result Python 返回的统计字典（samples / hue_categories）
     * @param renderer 目标图表的渲染器
     * @param title 图表标题，空串时使用 "Boxplot"
     * @return 是否成功渲染
     */
    static bool renderBoxplotResult(const pybind11::dict& result,
                                    DAChartPlotRenderer& renderer,
                                    const QString& title = QString());

    // Per-plot-type orchestration (called from the request's dispatchTo)
    void plotHistplot(const DA::DAStatsHistplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotKdeplot1d(const DA::DAStatsKdeplot1dRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotKdeplot2d(const DA::DAStatsKdeplot2dRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotBoxplot(const DA::DAStatsBoxplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotHeatmap(const DA::DAStatsHeatmapRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotScatterplot(const DA::DAStatsScatterplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotBarplot(const DA::DAStatsBarplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotRegplot(const DA::DAStatsRegplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);
    void plotEcdfplot(const DA::DAStatsEcdfplotRequest& req, DAChartPlotRenderer& renderer, const DA::DAPyDataFrame& df);

private:
    // Helpers: extract data from pybind11::dict to Qwt types
    QVector<double> dictToDoubleVector(const pybind11::dict& d, const QString& key);
    QPolygonF dictToPolygon(const pybind11::dict& d, const QString& xKey, const QString& yKey);
    QVector<QPointF> dictToPoints(const pybind11::dict& d, const QString& xKey, const QString& yKey);
    QStringList dictToStringList(const pybind11::dict& d, const QString& key);
};
