#ifndef DACHARTSERIESSAMPLER_H
#define DACHARTSERIESSAMPLER_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QList>
#include <QPointF>
#include <QStringList>
#include <QVector>
#include <cstdint>

namespace DA
{

/**
 * @brief 表格列 → 采样点/值的唯一采样入口（浮窗与正式图表共用）
 *
 * 采样策略（实施方案 §8.2 / D4 / D9）：
 * - 小表（≤ 5000 行）全量绘制，不标注采样；
 * - 大表确定性分块均匀采样：折线/散点约 2000 点、直方图约 2 万个值；
 * - 不引入随机数，保证「浮窗 ↔ 正式图表」结果一致、可单测；
 * - 分类聚合/分位数能精确就精确：内存 pandas 源在阈值内走全量（见 canAggregateExactly），
 *   超出阈值或惰性源才采样并标注。
 */
class DAGUI_API DAChartSeriesSampler
{
public:
    // 小表阈值：行数不超过它时全量绘制
    static constexpr int kFullPlotRowThreshold = 5000;
    // 折线/散点目标点数
    static constexpr int kMaxLinePoints = 2000;
    // 直方图目标值数
    static constexpr int kMaxHistogramValues = 20000;
    // 内存源可精确聚合的行数上限（超出则采样并标注）
    static constexpr std::int64_t kExactAggregationRowLimit = 5000000;
    // 分块采样把全区间切成多少个窗口
    static constexpr int kSampleWindowCount = 20;

    /**
     * @brief 一次构建可用的取数预算（实施方案 §8.4）
     *
     * 按「取数调用次数 + 取数行数」设硬上限，避免一次点击在惰性/慢速源上做无限取数；
     * 预算耗尽时采样器立即停止并置 SampleInfo::limited，界面据此标注「样本受限」。
     * 同一次渲染的多个采样共用一个预算对象（各列取数一起计数）。
     */
    struct SampleBudget
    {
        int maxFetchCalls { 128 };  ///< 最多多少次取数调用
        int maxFetchRows { 250000 };  ///< 最多取多少行
        int fetchCalls { 0 };       ///< 已用调用次数
        int fetchRows { 0 };        ///< 已取行数

        bool isExhausted() const { return fetchCalls >= maxFetchCalls || fetchRows >= maxFetchRows; }
        // 申请一次取数；预算不足返回 false，调用方应停止采样并标记受限
        bool request(int rowCount)
        {
            if (fetchCalls + 1 > maxFetchCalls || fetchRows + rowCount > maxFetchRows) {
                return false;
            }
            ++fetchCalls;
            fetchRows += rowCount;
            return true;
        }
    };

    /// 一次采样的统计信息
    struct SampleInfo
    {
        bool sampled { false };       ///< 是否做了采样（false = 全量）
        int sampledRows { 0 };        ///< 实际参与绘制的行数
        int totalRows { 0 };          ///< 数据总行数
        int droppedMissing { 0 };     ///< 因缺失/非法值被剔除的行数
        bool limited { false };       ///< 是否因预算上限提前停止
    };

    /// xy 点列采样结果
    struct XYSample
    {
        SampleInfo info;
        QVector< QPointF > points;
    };

    /// 多序列 xy 点列采样结果
    struct MultiXYSample
    {
        SampleInfo info;
        QList< QVector< QPointF > > series;
        QStringList names;
    };

    /// 分类（标签 + 数值）采样结果
    struct CategorySample
    {
        SampleInfo info;
        QStringList labels;
        QVector< double > values;
    };

    /// 多列按行对齐的数值采样结果（等高线/误差棒/向量场/3D 用）
    struct ColumnsSample
    {
        SampleInfo info;
        QList< QVector< double > > columns;  ///< 与请求的 dataColumns 顺序一致
        int rowCount() const { return columns.isEmpty() ? 0 : columns.first().size(); }
    };

    /// 分类 + 多列数值的按行对齐采样结果（按类别分组的多列箱线图用）
    struct CategoryMultiSample
    {
        SampleInfo info;
        QStringList labels;
        QList< QVector< double > > columns;  ///< 与 valueDataColumns 顺序一致
        int rowCount() const { return labels.size(); }
    };

    // 是否可以对全量数据做精确聚合/分位数
    static bool canAggregateExactly(const DATableChartRequest& request);

    /**
     * @brief 采样一条 xy 序列
     * @param xDataColumn x 的数据列号；-1 表示使用行号
     * @param yDataColumn y 的数据列号
     * @param maxPoints 目标点数上限（小表仍全量）
     */
    static XYSample sampleXY(const DATableChartRequest& request,
                             int xDataColumn,
                             int yDataColumn,
                             int maxPoints,
                             SampleBudget* budget = nullptr);

    /**
     * @brief 采样多序列 xy（同一个 x，按行对齐）
     */
    static MultiXYSample sampleMultiXY(const DATableChartRequest& request,
                                       int xDataColumn,
                                       const QList< int >& yDataColumns,
                                       int maxPoints,
                                       SampleBudget* budget = nullptr);

    /**
     * @brief 采样单列数值（直方图/箱线图用，不需要行号对齐）
     */
    static QVector< double > sampleValues(const DATableChartRequest& request,
                                          int dataColumn,
                                          int maxValues,
                                          SampleInfo* info = nullptr,
                                          SampleBudget* budget = nullptr);

    /**
     * @brief 采样分类列（可选数值列），返回逐行的标签 + 数值
     * @param valueDataColumn 数值列号；-1 表示纯分类（计数值留空）
     */
    static CategorySample sampleCategory(const DATableChartRequest& request,
                                         int categoryDataColumn,
                                         int valueDataColumn,
                                         int maxPoints,
                                         SampleBudget* budget = nullptr);

    /**
     * @brief 分类 + 多列数值的按行对齐采样（分组多列箱线图用）
     * @note 任一行在分类列或任一数值列上缺失都会整行剔除，保证 hue 与各列对齐
     */
    static CategoryMultiSample sampleCategoryMulti(const DATableChartRequest& request,
                                                   int categoryDataColumn,
                                                   const QList< int >& valueDataColumns,
                                                   int maxPoints,
                                                   SampleBudget* budget = nullptr);

    /**
     * @brief 多列按行对齐采样（每行所有列都必须可转数值，否则整行剔除）
     * @param dataColumns 数据列号列表（不是 request.columns 下标）
     * @param maxPoints 目标行数上限；传 kFullDataPoints(-1) 表示不限行数（转入全量重建用）
     */
    static constexpr int kFullDataPoints = -1;
    static ColumnsSample sampleColumns(const DATableChartRequest& request,
                                       const QList< int >& dataColumns,
                                       int maxPoints,
                                       SampleBudget* budget = nullptr);
};

}  // namespace DA

#endif  // DACHARTSERIESSAMPLER_H
