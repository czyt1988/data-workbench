#ifndef DATABLECONDITIONALFORMATSTORE_H
#define DATABLECONDITIONALFORMATSTORE_H
#include "DAGuiAPI.h"
#include "DATableConditionalFormatRule.h"
#include <QObject>
#include <QHash>
#include <QVector>
#include <QList>
#include <functional>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 条件格式规则子存储（元数据束成员，003 T1.5）
 *
 * 职责（设计稿 §3.1/§4.3）：
 * - 规则集合管理：QHash<列索引, QVector<Rule>> 按列分桶（位置键，R20 装饰类），
 *   分桶结构天然就是「每列缓存的规则列表」——delegate 每格只查本列桶，
 *   不遍历全部规则（红线2）；列表顺序即优先级，后命中的覆盖先命中的；
 * - 列插删重映射：onColumnsInserted/onColumnsRemoved（签名与 DATableStyleManager 一致，
 *   一批索引），完成后发 ruleReset()；
 * - 列统计缓存：色阶/数据条需要的 {min,max,median} 惰性计算 + 缓存，
 *   数据变更时由 widget 侧命令回调挂点调 invalidateColumnStats/invalidateAllStats 失效；
 *   统计计算经 StatsProvider 注入（widget 安装，内部走 Python da_column_stats 全表 O(n)
 *   归约，不采样不近似）；排序不改变列值集合，不失效（性能）；
 * - 预览规则集：对话框编辑期间 setPreviewRules 叠加（不落库、不入撤销栈），
 *   rulesForColumn 预览优先；确定后由 widget 提交命令，取消 clearPreview 还原；
 * - XML 段：<conditional-format> 进 DATableMetadata 束聚合（table-metadata.xml）。
 *
 * 求值入口 evaluate 为静态纯函数（单测直接调用，不依赖绘制环境）。
 */
class DAGUI_API DATableConditionalFormatStore : public QObject
{
    Q_OBJECT
public:
    // 列统计（色阶/数据条端点；全表归约，NaN 排除）
    struct ColumnStats
    {
        double minValue { 0.0 };
        double maxValue { 0.0 };
        double medianValue { 0.0 };
    };
    // 统计提供者：成功计算返回 true 并填充 out（widget 注入，内部走 Python；
    // 未安装或计算失败（非数值列/惰性源）返回 false，失败结果也缓存避免每格重算）
    using StatsProvider = std::function< bool(int column, ColumnStats& out) >;
    // 重复键提供者：成功返回 true 并填充 out = 列内出现≥2 次的值的归一化键集合
    //（DATableConditionalFormatRule::duplicateKey 口径；widget 注入，内部走 Python；
    // 未安装或计算失败（惰性源）返回 false，失败结果也缓存避免每格重算）
    using DuplicatesProvider = std::function< bool(int column, QSet< QString >& out) >;

    // ②层求值结果（纯函数输出，delegate 消费、单测断言）
    struct EvaluateResult
    {
        DATableCellStyle style;      ///< 合并后的前景/背景/字体（valid 标志区分未设置）
        bool hasDataBar { false };   ///< 是否绘制数据条
        double barRatio { 0.0 };     ///< 数据条归一化条长 [0,1]
        QColor barColor;             ///< 数据条填充色
        bool barGradient { false };  ///< 数据条渐变填充（沿条长方向透明度递增）
    };

    explicit DATableConditionalFormatStore(QObject* parent = nullptr);

    // ===== 规则集合管理（列索引 = 位置键）=====
    // 某列的规则列表（预览优先；无规则返回 nullptr）——delegate 的每列缓存查询入口
    const QVector< DATableConditionalFormatRule >* rulesForColumn(int column) const;
    QVector< DATableConditionalFormatRule > columnRules(int column) const;
    QList< int > columnsWithRules() const;
    bool hasRules(int column) const;
    bool isEmpty() const;
    int ruleCount() const;
    // 某列是否含依赖列统计的规则（色阶/数据条；delegate 借此决定是否触发统计计算）
    bool needsColumnStats(int column) const;
    // 某列是否含重复/唯一条件规则（delegate 借此决定是否取重复键集合）
    bool needsColumnDuplicates(int column) const;

    // 增删改移（全部发 rulesChanged(column)；命令层调用，本类不入撤销栈）
    void addRule(int column, const DATableConditionalFormatRule& rule, int index = -1);
    void removeRule(int column, int index);
    void updateRule(int column, int index, const DATableConditionalFormatRule& rule);
    void moveRule(int column, int from, int to);
    void setColumnRules(int column, const QVector< DATableConditionalFormatRule >& rules);
    void clearColumn(int column);
    void clearAll();

    // ===== 列插删重映射（装饰类位置键，R20；签名同 DATableStyleManager）=====
    void onColumnsInserted(const QList< int >& actualCols);
    void onColumnsRemoved(const QList< int >& actualCols);

    // ===== 列统计缓存 =====
    void setStatsProvider(StatsProvider provider);
    // 查询列统计（惰性计算 + 缓存）；该列无规则或未安装提供者或计算失败返回 false
    bool columnStats(int column, ColumnStats& out);
    void invalidateColumnStats(int column);
    void invalidateAllStats();

    // ===== 列重复键缓存（重复/唯一条件求值用）=====
    void setDuplicatesProvider(DuplicatesProvider provider);
    // 查询列重复键集合（惰性计算 + 缓存）；该列无规则或未安装提供者或计算失败返回 false
    bool columnDuplicateKeys(int column, QSet< QString >& out);

    // ===== 预览规则集（对话框编辑期间叠加，不落库；取消即还原）=====
    void setPreviewRules(const QHash< int, QVector< DATableConditionalFormatRule > >& preview);
    void clearPreview();
    bool hasPreview() const;

    // ===== ②层求值（静态纯函数：规则列表 + 单元格值 + 列统计 → 样式与数据条）=====
    // 按列表顺序求值（后者覆盖前者）；statsValid=false 时色阶/数据条规则跳过；
    // duplicateKeys 非空指针时重复/唯一条件按其集合求值，空指针时该两条件不命中
    static EvaluateResult evaluate(const QVector< DATableConditionalFormatRule >& rules,
                                   const QVariant& cellValue,
                                   bool statsValid,
                                   const ColumnStats& stats,
                                   const QSet< QString >* duplicateKeys = nullptr);

    // ===== 序列化（<conditional-format> 段，短属性名照样式段风格）=====
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    bool fromXml(const QDomElement& e);

Q_SIGNALS:
    // 某列规则变更（-1 表示整表，预览/清空用），视图据此重绘该列
    void rulesChanged(int column);
    // 整体重置（列插删重映射 / clearAll），与 DATableStyleManager::styleReset 同义
    void ruleReset();

private:
    // 统计缓存条目（failed=true 记录一次失败，失效前不再重算，避免每格打 Python）
    struct StatsCacheEntry
    {
        ColumnStats stats;
        bool failed { false };
    };

    // 重复键缓存条目（failed=true 记录一次失败，失效前不再重算，避免每格打 Python）
    struct DupCacheEntry
    {
        QSet< QString > keys;
        bool failed { false };
    };

    QHash< int, QVector< DATableConditionalFormatRule > > mRules;  ///< 列索引 → 规则列表（顺序即优先级）
    QHash< int, QVector< DATableConditionalFormatRule > > mPreviewRules;  ///< 预览规则集（对话框编辑期）
    QHash< int, StatsCacheEntry > mStatsCache;                     ///< 列统计缓存
    StatsProvider mStatsProvider;                                  ///< 统计提供者（widget 注入）
    QHash< int, DupCacheEntry > mDupCache;                         ///< 列重复键缓存
    DuplicatesProvider mDupProvider;                               ///< 重复键提供者（widget 注入）
};
}  // end of namespace DA
#endif  // DATABLECONDITIONALFORMATSTORE_H
