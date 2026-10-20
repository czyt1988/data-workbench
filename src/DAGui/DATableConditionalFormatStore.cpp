#include "DATableConditionalFormatStore.h"
#include "DATableKeyRemap.h"
#include <algorithm>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象（一般为 DATableMetadata 束）
 */
DATableConditionalFormatStore::DATableConditionalFormatStore(QObject* parent) : QObject(parent)
{
}

/**
 * @brief 某列的规则列表（预览优先）
 *
 * delegate 的每列缓存查询入口：按列分桶即缓存，每格只查本列桶（红线2）。
 * @param column 列索引（位置键）
 * @return 规则列表指针；该列无规则返回 nullptr（空列表不建桶）
 */
const QVector< DATableConditionalFormatRule >* DATableConditionalFormatStore::rulesForColumn(int column) const
{
    auto pit = mPreviewRules.constFind(column);
    if (pit != mPreviewRules.constEnd()) {
        return &pit.value();  // 预览优先（含"预览清空该列"的空列表语义）
    }
    auto it = mRules.constFind(column);
    if (it != mRules.constEnd()) {
        return &it.value();
    }
    return nullptr;
}

/**
 * @brief 某列规则列表的拷贝（预览优先）
 * @param column 列索引
 * @return 规则列表（无规则返回空）
 */
QVector< DATableConditionalFormatRule > DATableConditionalFormatStore::columnRules(int column) const
{
    const QVector< DATableConditionalFormatRule >* r = rulesForColumn(column);
    return r ? *r : QVector< DATableConditionalFormatRule >();
}

/**
 * @brief 所有含规则的列索引（升序）
 * @return 列索引列表
 */
QList< int > DATableConditionalFormatStore::columnsWithRules() const
{
    QList< int > cols = mRules.keys();
    std::sort(cols.begin(), cols.end());
    return cols;
}

/**
 * @brief 某列是否有规则（不含预览）
 * @param column 列索引
 * @return 有规则返回 true
 */
bool DATableConditionalFormatStore::hasRules(int column) const
{
    return mRules.contains(column);
}

/**
 * @brief 是否没有任何规则（含预览；delegate 快速路径用）
 * @return 规则与预览都空返回 true
 */
bool DATableConditionalFormatStore::isEmpty() const
{
    return mRules.isEmpty() && mPreviewRules.isEmpty();
}

/**
 * @brief 规则总数（不含预览）
 * @return 各列规则数之和
 */
int DATableConditionalFormatStore::ruleCount() const
{
    int n = 0;
    for (auto it = mRules.constBegin(); it != mRules.constEnd(); ++it) {
        n += it.value().size();
    }
    return n;
}

/**
 * @brief 某列是否含依赖列统计的规则（色阶/数据条，含预览）
 *
 * delegate 借此决定是否触发统计计算：纯单元格规则的列不打 Python（性能）。
 * @param column 列索引
 * @return 含色阶/数据条规则返回 true
 */
bool DATableConditionalFormatStore::needsColumnStats(int column) const
{
    const QVector< DATableConditionalFormatRule >* rules = rulesForColumn(column);
    if (!rules) {
        return false;
    }
    for (const DATableConditionalFormatRule& r : *rules) {
        if (r.enabled && (r.type == DATableConditionalFormatRule::Type::ColorScale
                          || r.type == DATableConditionalFormatRule::Type::DataBar)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 某列是否含重复/唯一条件规则（delegate 借此决定是否取重复键集合）
 * @param column 列索引
 * @return 含启用的重复/唯一条件单元格规则返回 true
 */
bool DATableConditionalFormatStore::needsColumnDuplicates(int column) const
{
    const QVector< DATableConditionalFormatRule >* rules = rulesForColumn(column);
    if (!rules) {
        return false;
    }
    for (const DATableConditionalFormatRule& r : *rules) {
        if (r.enabled && r.type == DATableConditionalFormatRule::Type::CellRule
            && (r.condition == DATableConditionalFormatRule::Condition::Duplicate
                || r.condition == DATableConditionalFormatRule::Condition::Unique)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 追加/插入规则
 * @param column 列索引
 * @param rule 规则
 * @param index 插入位置（-1 或越界 = 追加到末尾；末尾 = 最低优先级之上最后求值）
 */
void DATableConditionalFormatStore::addRule(int column, const DATableConditionalFormatRule& rule, int index)
{
    QVector< DATableConditionalFormatRule >& v = mRules[ column ];
    if (index < 0 || index >= v.size()) {
        v.append(rule);
    } else {
        v.insert(index, rule);
    }
    Q_EMIT rulesChanged(column);
}

/**
 * @brief 删除某列指定位置的规则
 * @param column 列索引
 * @param index 位置（越界无操作）
 */
void DATableConditionalFormatStore::removeRule(int column, int index)
{
    auto it = mRules.find(column);
    if (it == mRules.end() || index < 0 || index >= it.value().size()) {
        return;
    }
    it.value().removeAt(index);
    if (it.value().isEmpty()) {
        mRules.erase(it);  // 不留空桶（isEmpty/columnsWithRules 语义依赖）
    }
    Q_EMIT rulesChanged(column);
}

/**
 * @brief 替换某列指定位置的规则
 * @param column 列索引
 * @param index 位置（越界无操作）
 * @param rule 新规则
 */
void DATableConditionalFormatStore::updateRule(int column, int index, const DATableConditionalFormatRule& rule)
{
    auto it = mRules.find(column);
    if (it == mRules.end() || index < 0 || index >= it.value().size()) {
        return;
    }
    it.value()[ index ] = rule;
    Q_EMIT rulesChanged(column);
}

/**
 * @brief 移动规则位置（上移/下移 = 优先级调整）
 * @param column 列索引
 * @param from 原位置
 * @param to 目标位置（越界无操作）
 */
void DATableConditionalFormatStore::moveRule(int column, int from, int to)
{
    auto it = mRules.find(column);
    if (it == mRules.end() || from < 0 || from >= it.value().size() || to < 0 || to >= it.value().size() || from == to) {
        return;
    }
    const DATableConditionalFormatRule r = it.value().takeAt(from);
    it.value().insert(to, r);
    Q_EMIT rulesChanged(column);
}

/**
 * @brief 整体替换某列规则列表（命令 undo 还原用）
 * @param column 列索引
 * @param rules 新规则列表（空 = 清除该列）
 */
void DATableConditionalFormatStore::setColumnRules(int column, const QVector< DATableConditionalFormatRule >& rules)
{
    if (rules.isEmpty()) {
        mRules.remove(column);
    } else {
        mRules[ column ] = rules;
    }
    Q_EMIT rulesChanged(column);
}

/**
 * @brief 清除某列全部规则
 * @param column 列索引
 */
void DATableConditionalFormatStore::clearColumn(int column)
{
    if (mRules.remove(column)) {
        Q_EMIT rulesChanged(column);
    }
}

/**
 * @brief 清除全部规则与统计缓存
 */
void DATableConditionalFormatStore::clearAll()
{
    mRules.clear();
    mStatsCache.clear();
    mDupCache.clear();
    Q_EMIT ruleReset();
}

/**
 * @brief 列插入重映射：>= idx 的列键 +1（一批索引，签名同 DATableStyleManager）
 *
 * 统计缓存一并失效（列位置整体位移，缓存键不再可靠），完成后发 ruleReset()。
 * @param actualCols 插入的列索引列表
 */
void DATableConditionalFormatStore::onColumnsInserted(const QList< int >& actualCols)
{
    if (actualCols.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnInserted(mRules, actualCols);
    mStatsCache.clear();
    mDupCache.clear();
    Q_EMIT ruleReset();
}

/**
 * @brief 列删除重映射：== idx 的规则删除，> idx 的列键 -1（一批索引）
 *
 * 统计缓存一并失效，完成后发 ruleReset()。
 * @param actualCols 删除的列索引列表
 */
void DATableConditionalFormatStore::onColumnsRemoved(const QList< int >& actualCols)
{
    if (actualCols.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnRemoved(mRules, actualCols);
    mStatsCache.clear();
    mDupCache.clear();
    Q_EMIT ruleReset();
}

/**
 * @brief 安装列统计提供者（widget 注入，内部走 Python da_column_stats）
 *
 * 提供者变更（含置空）时统计缓存全部失效。
 * @param provider 统计提供者
 */
void DATableConditionalFormatStore::setStatsProvider(StatsProvider provider)
{
    mStatsProvider = std::move(provider);
    mStatsCache.clear();
}

/**
 * @brief 查询列统计（惰性计算 + 缓存；红线2：禁止每格重算）
 *
 * 该列无规则（预览优先）不计算返回 false；首次查询经提供者全表 O(n) 归约后缓存，
 * 失败（非数值列/无提供者/惰性源）也缓存为失败，失效前不再重试。
 * @param column 列索引
 * @param out 输出统计
 * @return 有效统计返回 true
 */
bool DATableConditionalFormatStore::columnStats(int column, ColumnStats& out)
{
    if (!rulesForColumn(column)) {
        return false;  // 无规则的列不算（计划 WP1 步骤 2）
    }
    auto it = mStatsCache.find(column);
    if (it != mStatsCache.end()) {
        if (it.value().failed) {
            return false;
        }
        out = it.value().stats;
        return true;
    }
    StatsCacheEntry entry;
    if (!mStatsProvider || !mStatsProvider(column, entry.stats)) {
        entry.failed = true;
        mStatsCache.insert(column, entry);
        return false;
    }
    mStatsCache.insert(column, entry);
    out = entry.stats;
    return true;
}

/**
 * @brief 失效某列统计缓存（widget 侧命令回调挂点调用）
 * @param column 列索引
 */
void DATableConditionalFormatStore::invalidateColumnStats(int column)
{
    mStatsCache.remove(column);
    mDupCache.remove(column);  // 重复键同属列值集合派生量，与统计同挂点失效
}

/**
 * @brief 失效全部统计缓存（插删行保守失效 / 列重映射 / 替换全表用）
 */
void DATableConditionalFormatStore::invalidateAllStats()
{
    mStatsCache.clear();
    mDupCache.clear();
}

/**
 * @brief 安装列重复键提供者（widget 注入，内部走 Python da_column_duplicate_values）
 *
 * 提供者变更（含置空）时重复键缓存全部失效。
 * @param provider 重复键提供者
 */
void DATableConditionalFormatStore::setDuplicatesProvider(DuplicatesProvider provider)
{
    mDupProvider = std::move(provider);
    mDupCache.clear();
}

/**
 * @brief 查询列重复键集合（惰性计算 + 缓存；红线2：禁止每格重算）
 *
 * 该列无规则（预览优先）不计算返回 false；首次查询经提供者全表 O(n) 归约后缓存，
 * 失败（无提供者/惰性源）也缓存为失败，失效前不再重试。
 * @param column 列索引
 * @param out 输出重复键集合
 * @return 有效集合返回 true
 */
bool DATableConditionalFormatStore::columnDuplicateKeys(int column, QSet< QString >& out)
{
    if (!rulesForColumn(column)) {
        return false;
    }
    auto it = mDupCache.find(column);
    if (it != mDupCache.end()) {
        if (it.value().failed) {
            return false;
        }
        out = it.value().keys;
        return true;
    }
    DupCacheEntry entry;
    if (!mDupProvider || !mDupProvider(column, entry.keys)) {
        entry.failed = true;
        mDupCache.insert(column, entry);
        return false;
    }
    mDupCache.insert(column, entry);
    out = entry.keys;
    return true;
}

/**
 * @brief 设置预览规则集（对话框编辑期整体替换；不落库、不入撤销栈）
 * @param preview 列索引 → 预览规则列表
 */
void DATableConditionalFormatStore::setPreviewRules(const QHash< int, QVector< DATableConditionalFormatRule > >& preview)
{
    mPreviewRules = preview;
    Q_EMIT rulesChanged(-1);
}

/**
 * @brief 清除预览规则集（对话框确定提交后或取消时调用，还原落库规则显示）
 */
void DATableConditionalFormatStore::clearPreview()
{
    if (mPreviewRules.isEmpty()) {
        return;
    }
    mPreviewRules.clear();
    Q_EMIT rulesChanged(-1);
}

/**
 * @brief 是否存在预览规则集
 * @return 预览非空返回 true
 */
bool DATableConditionalFormatStore::hasPreview() const
{
    return !mPreviewRules.isEmpty();
}

/**
 * @brief ②层求值（静态纯函数，单测直接调用）
 *
 * 按列表顺序求值，后命中的覆盖先命中的（Excel 语义，§4.3）：
 * - CellRule 命中 → mergeFrom 输出样式（后规则覆盖前规则已设置的属性）；
 * - ColorScale → 列统计有效且单元格值为数值时插值背景色（前景不变，保证可读性）；
 * - DataBar → 列统计有效且单元格值为数值时记录条长比例与颜色（末条生效）。
 * @param rules 该列规则列表（已按优先级排序）
 * @param cellValue 单元格原始值（Qt::EditRole）
 * @param statsValid 列统计是否有效
 * @param stats 列统计（statsValid=false 时忽略）
 * @return 求值结果（样式 + 数据条）
 */
DATableConditionalFormatStore::EvaluateResult
DATableConditionalFormatStore::evaluate(const QVector< DATableConditionalFormatRule >& rules,
                                        const QVariant& cellValue,
                                        bool statsValid,
                                        const ColumnStats& stats,
                                        const QSet< QString >* duplicateKeys)
{
    EvaluateResult result;
    double cellNum    = 0.0;
    bool cellNumValid = false;
    bool numericTried = false;
    for (const DATableConditionalFormatRule& r : rules) {
        if (!r.enabled) {
            continue;
        }
        switch (r.type) {
        case DATableConditionalFormatRule::Type::CellRule: {
            bool hit = false;
            if (r.condition == DATableConditionalFormatRule::Condition::Duplicate
                || r.condition == DATableConditionalFormatRule::Condition::Unique) {
                // 重复/唯一需列上下文：无集合（惰性源等）一律不命中
                static const QSet< QString > c_emptyKeys;
                hit = DATableConditionalFormatRule::evaluateDuplicate(
                    r.condition, cellValue, duplicateKeys ? *duplicateKeys : c_emptyKeys, duplicateKeys != nullptr);
            } else {
                hit = DATableConditionalFormatRule::evaluateCondition(r.condition, cellValue, r.value1, r.value2);
            }
            if (hit) {
                result.style.mergeFrom(r.style);  // 复用样式叠加语义（①层同一代码路径）
            }
            break;
        }
        case DATableConditionalFormatRule::Type::ColorScale:
        case DATableConditionalFormatRule::Type::DataBar: {
            if (!statsValid) {
                break;
            }
            if (!numericTried) {
                cellNumValid = DATableConditionalFormatRule::toNumeric(cellValue, &cellNum);
                numericTried = true;
            }
            if (!cellNumValid) {
                break;  // 非数值单元格：色阶/数据条跳过
            }
            if (r.type == DATableConditionalFormatRule::Type::ColorScale) {
                const QColor bg = DATableConditionalFormatRule::colorScaleColor(
                    cellNum, stats.minValue, stats.maxValue, stats.medianValue,
                    r.threeColorScale, r.scaleMinColor, r.scaleMidColor, r.scaleMaxColor);
                result.style.setBackground(bg);  // 后规则直接覆盖前规则的背景
            } else {
                result.hasDataBar = true;
                result.barRatio   = DATableConditionalFormatRule::dataBarRatio(cellNum, stats.minValue, stats.maxValue);
                result.barColor   = r.barColor;
                result.barGradient = r.barGradient;
            }
            break;
        }
        }
    }
    return result;
}

/**
 * @brief 序列化 <conditional-format> 段（按列索引组织，短属性名）
 *
 * 段结构：<conditional-format><column col="n"><rule type="..." .../>...</column>...</conditional-format>
 * @param doc QDomDocument
 * @param parent table 元素
 */
void DATableConditionalFormatStore::toXml(QDomDocument& doc, QDomElement& parent) const
{
    if (mRules.isEmpty()) {
        return;
    }
    QDomElement cf = doc.createElement(QStringLiteral("conditional-format"));
    // 按列索引升序写出（round-trip 稳定）
    QList< int > cols = mRules.keys();
    std::sort(cols.begin(), cols.end());
    for (int col : std::as_const(cols)) {
        QDomElement colEle = doc.createElement(QStringLiteral("column"));
        colEle.setAttribute(QStringLiteral("col"), col);
        const QVector< DATableConditionalFormatRule >& rules = mRules[ col ];
        for (const DATableConditionalFormatRule& r : rules) {
            r.toXml(doc, colEle);
        }
        cf.appendChild(colEle);
    }
    parent.appendChild(cf);
}

/**
 * @brief 从 <conditional-format> 段反序列化（先清空再写入；未知标签忽略）
 * @param e conditional-format 元素
 * @return 始终返回 true
 */
bool DATableConditionalFormatStore::fromXml(const QDomElement& e)
{
    mRules.clear();
    mStatsCache.clear();
    mDupCache.clear();
    QDomNode cn = e.firstChild();
    while (!cn.isNull()) {
        QDomElement colEle = cn.toElement();
        if (!colEle.isNull() && colEle.tagName() == QLatin1String("column")) {
            const int col = colEle.attribute(QStringLiteral("col")).toInt();
            QVector< DATableConditionalFormatRule > rules;
            QDomNode rn = colEle.firstChild();
            while (!rn.isNull()) {
                QDomElement ruleEle = rn.toElement();
                if (!ruleEle.isNull() && ruleEle.tagName() == QLatin1String("rule")) {
                    DATableConditionalFormatRule r;
                    if (r.fromXml(ruleEle)) {
                        rules.append(r);
                    }
                }
                rn = rn.nextSibling();
            }
            if (!rules.isEmpty()) {
                mRules.insert(col, rules);
            }
        }
        cn = cn.nextSibling();
    }
    Q_EMIT ruleReset();
    return true;
}

}  // end of namespace DA
