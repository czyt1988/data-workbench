#ifndef DASTATSPLOTREQUESTS_H
#define DASTATSPLOTREQUESTS_H
#include "DAGuiAPI.h"
#include "Chart/DAStatsPlotRequest.h"
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QPair>
#include <optional>

class DAChartPlotRenderer;
class DAStatsPlotCoordinator;

namespace DA
{
class DAPyDataFrame;

/**
 * @brief 直方图请求（histplot）
 *
 * 字段清单来自 DAChartAddStatsHistplotWidget 的 UI 实测；
 * 标注「未消费」的字段保留 UI 值但 Python/C++ 均不读取（见重构方案 §3.4 审计表）。
 */
class DAGUI_API DAStatsHistplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString column;    ///< 数据列名
    QString hue;       ///< 分组列名（空=不分组）
    int bins { 10 };   ///< 分箱数
    double binwidth { 0.0 };  ///< 分箱宽度（0=按 bins 分箱）
    std::optional< QPair< double, double > > binrange;  ///< 分箱数据范围（无=自动）
    QString stat { QStringLiteral("count") };  ///< 统计量（count/frequency/probability/percent/density）
    bool kde { false };      ///< 是否叠加 KDE 曲线（C++ 消费，分派第二次 computeKde1d 调用）
    bool cumulative { false };  ///< 是否累积计数
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    QString kdeBw;  ///< 未消费：compute_kde_1d 读 bw_method 而非 kde_bw，历史上从未生效
    bool fill { false };        ///< 未消费
    QString element;            ///< 未消费（常量 "bars"）
    QString multiple;           ///< 未消费（常量 "layer"）
};

/**
 * @brief 一维核密度估计请求（kdeplot_1d）
 */
class DAGUI_API DAStatsKdeplot1dRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString column;  ///< 数据列名
    QString bwMethod { QStringLiteral("scott") };  ///< 带宽方法（scott/silverman/custom）
    std::optional< double > bwValue;  ///< 自定义带宽（bwMethod=="custom" 时有效）
    int gridSize { 200 };  ///< 网格点数
    bool cumulative { false };  ///< 是否积分累积
    bool fill { false };  ///< 是否填充基线多边形（C++ 消费，渲染构造闭合多边形）
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    QString hue;  ///< 未消费：coordinator 现状不分组渲染
    bool shade { false };       ///< 未消费
    bool commonNorm { false };   ///< 未消费
    std::optional< double > below;  ///< 未消费
    std::optional< double > above;  ///< 未消费
};

/**
 * @brief 二维核密度估计请求（kdeplot_2d）
 */
class DAGUI_API DAStatsKdeplot2dRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString xColumn;  ///< X 轴列名
    QString yColumn;  ///< Y 轴列名
    QString bwMethod { QStringLiteral("scott") };  ///< 带宽方法（scott/silverman/custom）
    std::optional< double > bwValue;  ///< 自定义带宽（bwMethod=="custom" 时有效）
    int gridSize { 100 };  ///< 每轴网格点数
    int levels { 5 };      ///< 等高线级数
    double thresh { 0.05 };  ///< 密度阈值（z_max 的比例，低于则置 NaN）
    bool fillContours { false };  ///< 是否叠加等高线（C++ 消费，修复原 contour 死键）
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    bool showHeatmap { false };  ///< 未消费
    bool commonNorm { false };   ///< 未消费
};

/**
 * @brief 箱线图请求（boxplot）
 */
class DAGUI_API DAStatsBoxplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QStringList columns;  ///< 数据列名（一列一箱）
    QString hue;          ///< 分组列名（空=不分组）
    double whis { 1.5 };  ///< 须长（IQR 倍数）
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    double width { 0.0 };      ///< 未消费
    bool showfliers { true };  ///< 未消费
    bool showmeans { false };  ///< 未消费
};

/**
 * @brief 热力图请求（heatmap）
 *
 * 字段命名对齐 compute_pivot_matrix 的位置参数语义
 * （index_col/columns_col/values_col），修复原 widget 与 coordinator 的键名三元组断裂。
 */
class DAGUI_API DAStatsHeatmapRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString indexColumn;   ///< 行方向（Y 轴）列名
    QString columnsColumn;  ///< 列方向（X 轴）列名
    QString valueColumn;   ///< 值列名（空=计数矩阵）
    QString aggfunc { QStringLiteral("mean") };  ///< 聚合函数（mean/sum/count/median/max/min）
    QString cmap;          ///< 色图名（C++ 消费，进渲染 style）
    std::optional< QString > standardScale;  ///< 标准化方向（row/column，无=不标准化）
    std::optional< double > vmin;  ///< 色彩映射下界（无=自动）
    std::optional< double > vmax;  ///< 色彩映射上界（无=自动）
    std::optional< double > center;  ///< 居中值（无=不居中）
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    bool annot { false };  ///< 未消费
    QString fmt;           ///< 未消费
};

/**
 * @brief 统计散点图请求（scatterplot，纯 C++ 提取绘制，不调 Python）
 */
class DAGUI_API DAStatsScatterplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString xColumn;  ///< X 轴列名
    QString yColumn;  ///< Y 轴列名
    QString hue;      ///< 分组列名（空=不分组；C++ 消费，按组渲染多散点）
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    int size { 5 };         ///< 未消费
    QString style;           ///< 未消费
    double alpha { 1.0 };   ///< 未消费
    bool legend { false };  ///< 未消费
};

/**
 * @brief 柱状图/计数图请求（barplot / yColumn 为空时为 count plot）
 */
class DAGUI_API DAStatsBarplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString xColumn;  ///< 类别列名
    QString yColumn;  ///< 数值列名（空=count plot）
    QString hue;      ///< 分组列名（空=不分组；透传 Python 分组聚合）
    QString estimator { QStringLiteral("mean") };  ///< 聚合估计器（mean/median/sum/count/std/var/min/max）
    int ci { 0 };       ///< 置信水平百分比（0=无；C++ 消费，>0 且 y 非空时调 computeCI）
    int nBoot { 1000 };  ///< bootstrap 迭代次数
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    QString orient;          ///< 未消费
    double width { 0.8 };     ///< 未消费
    bool legend { false };    ///< 未消费
};

/**
 * @brief 回归图请求（regplot）
 */
class DAGUI_API DAStatsRegplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString xColumn;       ///< X 轴列名（自变量）
    QString yColumn;       ///< Y 轴列名（因变量）
    int order { 1 };       ///< 多项式阶数
    int ci { 0 };           ///< 置信水平百分比（0=无；行为修复：原实现 toInt(95) 使"None"选项失效）
    int nBoot { 1000 };     ///< bootstrap 迭代次数
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    bool scatter { true };            ///< 未消费
    bool fitReg { true };             ///< 未消费
    QVariantMap scatterKws;           ///< 未消费
    QVariantMap regKws;               ///< 未消费
};

/**
 * @brief 经验累积分布图请求（ecdfplot）
 */
class DAGUI_API DAStatsEcdfplotRequest : public DAStatsPlotRequest
{
public:
    DAChartTypes type() const override;
    bool isValid() const override;
    QString errorString() const override;
    QVariantMap toPythonParams() const override;
    void dispatchTo(DAStatsPlotCoordinator& coordinator,
                    DAChartPlotRenderer& renderer,
                    const DAPyDataFrame& df) const override;

    QString column;  ///< 数据列名
    QString weights;  ///< 权重列名（空=无权重）
    QString stat { QStringLiteral("proportion") };  ///< 统计量（proportion/count）
    bool complementary { false };  ///< 是否计算 1-CDF
    // ---- 未消费字段（保留 UI 值，行为不变） ----
    QString hue;             ///< 未消费
    double lineWidth { 1.0 };  ///< 未消费
    bool legend { false };     ///< 未消费
};

}  // namespace DA

#endif  // DASTATSPLOTREQUESTS_H
