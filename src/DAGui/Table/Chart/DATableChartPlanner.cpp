#include "Table/Chart/DATableChartPlanner.h"
#include "DATableDataSource.h"
#include "DATableScan.h"
#include <QMetaType>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QVariant>
#include <QSet>
#include <algorithm>
#include <utility>

namespace DA
{

namespace
{

// 低基数数值列「作为分类」的唯一值上限
constexpr int kLowCardinalityLimit = 50;

DATableColumnRole roleFromMetaType(int metaType)
{
    switch (metaType) {
    case QMetaType::Bool:
        return DATableColumnRole::Categorical;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
    // DAPyDType::toMetaType 把 int8/uint8 映射成 QChar，这里按数值处理
    case QMetaType::QChar:
        return DATableColumnRole::Numeric;
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return DATableColumnRole::DateTime;
    case QMetaType::QString:
    case QMetaType::QByteArray:
        return DATableColumnRole::Categorical;
    default:
        break;
    }
    return DATableColumnRole::Unsupported;
}

// 类型无法静态判定时，用少量非空样本值推断角色
DATableColumnRole roleFromSample(const QVariantList& values)
{
    for (const QVariant& v : values) {
        if (!v.isValid() || v.isNull()) {
            continue;
        }
        const DATableColumnRole r = roleFromMetaType(v.userType());
        if (r != DATableColumnRole::Unsupported) {
            return r;
        }
        bool ok = false;
        v.toDouble(&ok);
        if (ok) {
            return DATableColumnRole::Numeric;
        }
        if (v.canConvert< QDateTime >()) {
            return DATableColumnRole::DateTime;
        }
        return DATableColumnRole::Categorical;
    }
    return DATableColumnRole::Unsupported;
}

bool specValuesEqual(const DATableChartSpec& a, const DATableChartSpec& b)
{
    return a.type == b.type && a.xColumn == b.xColumn && a.yColumn == b.yColumn
           && a.zColumn == b.zColumn && a.errorColumn == b.errorColumn && a.uColumn == b.uColumn
           && a.vColumn == b.vColumn && a.valueColumns == b.valueColumns
           && a.categoryColumn == b.categoryColumn && a.aggregation == b.aggregation
           && a.categoryTopN == b.categoryTopN && a.histogramBins == b.histogramBins
           && a.sortByValueDesc == b.sortByValueDesc;
}

}  // namespace

// ============================================================
// DATableChartRequest
// ============================================================

/**
 * @brief 请求是否可用
 */
bool DATableChartRequest::isValid() const
{
    return data.tableSource() != nullptr && !columns.isEmpty();
}

/**
 * @brief 数值列个数
 */
int DATableChartRequest::numericCount() const
{
    int n = 0;
    for (const DATableChartColumn& c : columns) {
        if (c.isNumeric()) {
            ++n;
        }
    }
    return n;
}

/**
 * @brief 分类列个数
 */
int DATableChartRequest::categoricalCount() const
{
    int n = 0;
    for (const DATableChartColumn& c : columns) {
        if (c.isCategorical()) {
            ++n;
        }
    }
    return n;
}

/**
 * @brief 时间列个数
 */
int DATableChartRequest::dateTimeCount() const
{
    int n = 0;
    for (const DATableChartColumn& c : columns) {
        if (c.isDateTime()) {
            ++n;
        }
    }
    return n;
}

/**
 * @brief 第一个指定角色的列下标
 */
int DATableChartRequest::indexOfFirst(DATableColumnRole role) const
{
    for (int i = 0; i < columns.size(); ++i) {
        if (columns[ i ].role == role) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 指定角色的全部列下标
 */
QList< int > DATableChartRequest::indexesOf(DATableColumnRole role) const
{
    QList< int > res;
    for (int i = 0; i < columns.size(); ++i) {
        if (columns[ i ].role == role) {
            res.append(i);
        }
    }
    return res;
}

// ============================================================
// DATableChartSpec
// ============================================================

/**
 * @brief 规格是否可执行
 */
bool DATableChartSpec::isValid() const
{
    switch (type) {
    case DATableChartType::None:
        return false;
    case DATableChartType::Line:
    case DATableChartType::MultiLine:
        return !valueColumns.isEmpty();
    case DATableChartType::Scatter:
        return !valueColumns.isEmpty();
    case DATableChartType::Bar:
        // 柱状图必须有分类轴；valueColumns 为空表示 countplot
        return categoryColumn >= 0;
    case DATableChartType::Histogram:
        return !valueColumns.isEmpty();
    case DATableChartType::Boxplot:
        return !valueColumns.isEmpty();
    case DATableChartType::Contour:
    case DATableChartType::Surface3D:
    case DATableChartType::Line3D:
        return xColumn >= 0 && yColumn >= 0 && zColumn >= 0;
    case DATableChartType::ErrorBar:
        return xColumn >= 0 && yColumn >= 0 && errorColumn >= 0;
    case DATableChartType::VectorField:
        return xColumn >= 0 && yColumn >= 0 && uColumn >= 0 && vColumn >= 0;
    case DATableChartType::MultiBar:
        return categoryColumn >= 0 && !valueColumns.isEmpty();
    }
    return false;
}

/**
 * @brief 深比较
 */
bool DATableChartSpec::equals(const DATableChartSpec& other) const
{
    return specValuesEqual(*this, other);
}

// ============================================================
// DATableChartPlanner
// ============================================================

/**
 * @brief 判定单列角色
 *
 * 先看数据源给出的列类型（对惰性/数据库源同样有效），类型无法静态判定时
 * 取起始的少量非空样本值推断；全空列视为不支持。
 */
DATableColumnRole DATableChartPlanner::detectRole(const DAData& data, int dataColumn, int sampleRowLimit)
{
    DAData dataCopy            = data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || dataColumn < 0) {
        return DATableColumnRole::Unsupported;
    }
    const DATableColumnRole declared = roleFromMetaType(source->tableColumnType(static_cast< std::size_t >(dataColumn)));
    if (declared != DATableColumnRole::Unsupported) {
        return declared;
    }
    if (sampleRowLimit <= 0) {
        return DATableColumnRole::Unsupported;
    }
    const std::size_t rowCount = source->tableRowCount();
    const std::size_t take     = std::min(rowCount, static_cast< std::size_t >(sampleRowLimit));
    if (take == 0) {
        return DATableColumnRole::Unsupported;
    }
    return roleFromSample(source->fetchColumnRange(static_cast< std::size_t >(dataColumn), 0, take));
}

/**
 * @brief 组装请求中的一列
 */
DATableChartColumn DATableChartPlanner::makeColumn(const DAData& data,
                                                   int dataColumn,
                                                   int occurrence,
                                                   int sampleRowLimit)
{
    DATableChartColumn c;
    c.dataIndex  = dataColumn;
    c.occurrence = occurrence;
    DAData dataCopy           = data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source = dataCopy.tableSource();
    if (source) {
        if (dataColumn >= 0 && static_cast< std::size_t >(dataColumn) < source->tableColumnCount()) {
            c.name = source->tableColumnName(static_cast< std::size_t >(dataColumn));
        }
    }
    c.role = detectRole(data, dataColumn, sampleRowLimit);
    return c;
}

/**
 * @brief 按能力矩阵规划可选图型与默认规格
 *
 * 规则见实施方案 v3 §2.2：v3 覆盖 12 种图型（含分类+多数值的多重柱状与 3D 类），
 * 2 分类交叉仍明确拒绝并给出原因。
 */
DATableChartPlan DATableChartPlanner::plan(const DATableChartRequest& request)
{
    DATableChartPlan result;
    if (request.data.tableSource() == nullptr) {
        result.reject = DATableChartRejectReason::NotTableData;
        return result;
    }
    if (request.columns.isEmpty()) {
        result.reject = DATableChartRejectReason::EmptySelection;
        return result;
    }
    const int total  = request.columns.size();
    const int numCnt = request.numericCount();
    const int catCnt = request.categoricalCount();
    const int dtCnt  = request.dateTimeCount();

    if (total == 1) {
        if (numCnt == 1) {
            result.types = { DATableChartType::Line, DATableChartType::Histogram, DATableChartType::Boxplot };
        } else if (catCnt == 1) {
            result.types = { DATableChartType::Bar };
        } else if (dtCnt == 1) {
            result.reject = DATableChartRejectReason::OnlyTimeColumn;
            return result;
        } else {
            result.reject = DATableChartRejectReason::NoNumericColumn;
            return result;
        }
    } else if (total == 2) {
        if (numCnt == 2) {
            result.types = { DATableChartType::Scatter, DATableChartType::Line, DATableChartType::Boxplot };
            // 低基数数值列可「作为分类」出柱状图（实施方案 D11）
            for (int idx : request.indexesOf(DATableColumnRole::Numeric)) {
                if (probeUniqueCount(request, request.columns.at(idx).dataIndex, kLowCardinalityLimit) > 0) {
                    result.types.append(DATableChartType::Bar);
                    break;
                }
            }
        } else if (dtCnt == 1 && numCnt == 1) {
            result.types = { DATableChartType::Line, DATableChartType::Scatter };
        } else if (catCnt == 1 && numCnt == 1) {
            result.types = { DATableChartType::Bar, DATableChartType::Boxplot };
        } else {
            // 2 分类（交叉柱状）与其它无数值列组合：v1 明确不支持，给出可读原因
            result.reject = (catCnt >= 2) ? DATableChartRejectReason::UnsupportedCombination
                                          : DATableChartRejectReason::NoNumericColumn;
            return result;
        }
    } else {
        // 3 列及以上（v3 能力矩阵，实施方案 §2.2）
        if (numCnt == total) {
            // 纯数值：散点/多序列/箱线图为通用项；3 列数值起开放等高线/误差棒/3D，
            // 4 列起开放向量场（严格角色门槛：三列数值才出 3D 类，四列才出向量场）
            result.types = { DATableChartType::Scatter,
                             DATableChartType::MultiLine,
                             DATableChartType::Boxplot };
            if (numCnt >= 3) {
                result.types.append(DATableChartType::Contour);
                result.types.append(DATableChartType::ErrorBar);
                result.types.append(DATableChartType::Surface3D);
                result.types.append(DATableChartType::Line3D);
            }
            if (numCnt >= 4) {
                result.types.append(DATableChartType::VectorField);
            }
        } else if (dtCnt == 1 && numCnt == total - 1) {
            // 时间 + 2+ 数值：多序列折线（时间列不作为 3D/等高线的坐标轴）
            result.types = { DATableChartType::MultiLine };
        } else if (catCnt == 1 && numCnt == total - 1 && numCnt >= 2) {
            // 分类 + 2+ 数值：多重柱状（v3 新增，原为拒绝）+ 按类别箱线图
            result.types = { DATableChartType::MultiBar, DATableChartType::Boxplot };
        } else if (numCnt == 0) {
            result.reject = (catCnt >= 2) ? DATableChartRejectReason::UnsupportedCombination
                                          : DATableChartRejectReason::NoNumericColumn;
            return result;
        } else {
            result.reject = DATableChartRejectReason::UnsupportedCombination;
            return result;
        }
    }
    result.defaultSpec = defaultSpec(request, result.types.first());
    for (DATableChartType type : std::as_const(result.types)) {
        result.typeSpecs.append(defaultSpec(request, type));
    }
    return result;
}

/**
 * @brief 给定图型生成默认规格
 */
DATableChartSpec DATableChartPlanner::defaultSpec(const DATableChartRequest& request, DATableChartType type)
{
    DATableChartSpec spec;
    spec.type = type;
    const int numIdx     = request.indexOfFirst(DATableColumnRole::Numeric);
    const int catIdx     = request.indexOfFirst(DATableColumnRole::Categorical);
    const int dtIdx      = request.indexOfFirst(DATableColumnRole::DateTime);
    const QList< int > nums = request.indexesOf(DATableColumnRole::Numeric);

    switch (type) {
    case DATableChartType::None:
        break;
    case DATableChartType::Line:
        if (dtIdx >= 0 && numIdx >= 0) {
            spec.xColumn = dtIdx;
        } else if (nums.size() >= 2) {
            spec.xColumn = nums.at(0);
        } else {
            spec.xColumn = -1;
        }
        spec.valueColumns = { nums.size() == 1 ? nums.first() : (nums.size() >= 2 ? nums.at(1) : numIdx) };
        break;
    case DATableChartType::Scatter:
        if (nums.size() >= 2) {
            spec.xColumn      = nums.at(0);
            spec.valueColumns = { nums.at(1) };
        } else {
            spec.xColumn      = -1;
            spec.valueColumns = nums;
            spec.type         = DATableChartType::Line;
        }
        break;
    case DATableChartType::Histogram:
        if (numIdx >= 0) {
            spec.valueColumns = { numIdx };
        }
        break;
    case DATableChartType::Boxplot:
        spec.valueColumns = nums;
        if (catIdx >= 0) {
            // 分类 + 数值：默认按类别分组（分类 + 2+ 数值时每个数值列一组）
            spec.categoryColumn = catIdx;
        }
        break;
    case DATableChartType::Bar:
        if (catIdx >= 0) {
            spec.categoryColumn = catIdx;
            if (numIdx >= 0) {
                spec.valueColumns = { numIdx };
            }
        } else {
            // 没有分类列时，只有「低基数数值列作为分类」才有柱状图（与 X 轴下拉同一口径）
            int chosen = -1;
            for (int idx : nums) {
                if (probeUniqueCount(request, request.columns.at(idx).dataIndex, 30) > 0) {
                    chosen = idx;
                    break;
                }
            }
            if (chosen >= 0) {
                spec.categoryColumn = chosen;
                for (int idx : nums) {
                    if (idx != chosen) {
                        spec.valueColumns.append(idx);
                    }
                }
            } else if (numIdx >= 0) {
                spec.valueColumns = { numIdx };
            }
        }
        spec.aggregation     = DATableChartAggregation::Mean;
        spec.sortByValueDesc = true;
        spec.categoryTopN    = 30;
        break;
    case DATableChartType::MultiLine:
        spec.xColumn      = (dtIdx >= 0) ? dtIdx : -1;
        spec.valueColumns = nums;
        break;
    case DATableChartType::Contour:
        if (nums.size() >= 3) {
            spec.xColumn = nums.at(0);
            spec.yColumn = nums.at(1);
            spec.zColumn = nums.at(2);
        }
        break;
    case DATableChartType::ErrorBar:
        if (nums.size() >= 3) {
            spec.xColumn     = nums.at(0);
            spec.yColumn     = nums.at(1);
            spec.errorColumn = nums.at(2);
        }
        break;
    case DATableChartType::VectorField:
        if (nums.size() >= 4) {
            spec.xColumn = nums.at(0);
            spec.yColumn = nums.at(1);
            spec.uColumn = nums.at(2);
            spec.vColumn = nums.at(3);
        }
        break;
    case DATableChartType::MultiBar:
        if (catIdx >= 0 && nums.size() >= 2) {
            spec.categoryColumn  = catIdx;
            spec.valueColumns    = nums;
            spec.aggregation     = DATableChartAggregation::Mean;
            spec.sortByValueDesc = false;  // 多序列按类别名顺序展示，便于横向对照
            spec.categoryTopN    = 30;
        }
        break;
    case DATableChartType::Surface3D:
    case DATableChartType::Line3D:
        if (nums.size() >= 3) {
            spec.xColumn = nums.at(0);
            spec.yColumn = nums.at(1);
            spec.zColumn = nums.at(2);
        }
        break;
    }
    return spec;
}

/**
 * @brief 当前请求是否支持指定图型
 */
bool DATableChartPlanner::supports(const DATableChartRequest& request, DATableChartType type)
{
    return plan(request).types.contains(type);
}

/**
 * @brief X 轴下拉候选
 *
 * 只有「可用行号或已选数值/时间列当 X」的多列场景才给出候选；
 * 1 列数值、直方图、箱线图、分类柱状一律返回空（界面隐藏下拉）。
 */
QList< DATableChartXOption > DATableChartPlanner::xAxisCandidates(const DATableChartRequest& request,
                                                                  DATableChartType type)
{
    QList< DATableChartXOption > res;
    const QList< int > nums = request.indexesOf(DATableColumnRole::Numeric);
    const int dtIdx         = request.indexOfFirst(DATableColumnRole::DateTime);

    // 分类柱状只有在「低基数数值列作为分类」时仍需要 X 下拉（用于切回行号/其它列）
    if (type == DATableChartType::Bar) {
        const bool numericCategory = !nums.isEmpty();
        if (!numericCategory || nums.size() < 2) {
            return res;
        }
    } else if (type != DATableChartType::Line && type != DATableChartType::Scatter
               && type != DATableChartType::MultiLine) {
        return res;
    }

    if (dtIdx >= 0 && !nums.isEmpty() && type != DATableChartType::Bar) {
        // 时间 + 数值：默认时间列，可切行号
        res.append(DATableChartXOption { -1, false });
        res.append(DATableChartXOption { dtIdx, false });
        return res;
    }
    if (nums.size() < 2) {
        return res;
    }
    res.append(DATableChartXOption { -1, false });
    for (int idx : nums) {
        res.append(DATableChartXOption { idx, false });
    }
    // 低基数数值列可「作为分类」当柱状图的 X
    for (int idx : nums) {
        const int unique = probeUniqueCount(request, request.columns.at(idx).dataIndex, kLowCardinalityLimit);
        if (unique > 0) {
            res.append(DATableChartXOption { idx, true });
        }
    }
    return res;
}

/**
 * @brief 把 spec 的 X 轴切换到候选项
 *
 * 普通候选只改 xColumn；「作为分类」候选把图型切到分类柱状。从「作为分类」
 * 切回行号/普通列时，图型回落到该请求的默认非柱状图型（避免留在不可用的 Bar）。
 */
void DATableChartPlanner::applyXOption(const DATableChartRequest& request,
                                       DATableChartSpec& spec,
                                       const DATableChartXOption& option)
{
    const bool wasNumericCategory = (spec.type == DATableChartType::Bar) && (spec.categoryColumn >= 0)
                                    && spec.categoryColumn < request.columns.size()
                                    && request.columns.at(spec.categoryColumn).isNumeric();

    if (option.isRowIndex()) {
        spec.xColumn = -1;
        if (wasNumericCategory) {
            const DATableChartPlan p = plan(request);
            for (DATableChartType t : p.types) {
                if (t != DATableChartType::Bar) {
                    spec.type            = t;
                    spec.categoryColumn  = -1;
                    spec.valueColumns    = defaultSpec(request, t).valueColumns;
                    break;
                }
            }
        }
        return;
    }
    if (option.asCategory) {
        spec.type           = DATableChartType::Bar;
        spec.categoryColumn = option.column;
        spec.xColumn        = -1;
        spec.valueColumns.clear();
        const QList< int > nums = request.indexesOf(DATableColumnRole::Numeric);
        for (int idx : nums) {
            if (idx != option.column) {
                spec.valueColumns.append(idx);
            }
        }
        spec.aggregation     = DATableChartAggregation::Mean;
        spec.sortByValueDesc = true;
        return;
    }
    spec.xColumn = option.column;
}

/**
 * @brief 聚合口径 → Python 估计器名
 */
QString DATableChartPlanner::aggregationKey(DATableChartAggregation aggregation)
{
    switch (aggregation) {
    case DATableChartAggregation::Mean:
        return QStringLiteral("mean");
    case DATableChartAggregation::Median:
        return QStringLiteral("median");
    case DATableChartAggregation::Sum:
        return QStringLiteral("sum");
    case DATableChartAggregation::Count:
        return QStringLiteral("count");
    case DATableChartAggregation::Std:
        return QStringLiteral("std");
    case DATableChartAggregation::Var:
        return QStringLiteral("var");
    case DATableChartAggregation::Min:
        return QStringLiteral("min");
    case DATableChartAggregation::Max:
        return QStringLiteral("max");
    }
    return QStringLiteral("mean");
}

/**
 * @brief 聚合口径的全部取值（默认 mean 在最前）
 */
QList< DATableChartAggregation > DATableChartPlanner::allAggregations()
{
    return { DATableChartAggregation::Mean,
             DATableChartAggregation::Min,
             DATableChartAggregation::Max,
             DATableChartAggregation::Sum,
             DATableChartAggregation::Median,
             DATableChartAggregation::Count,
             DATableChartAggregation::Std,
             DATableChartAggregation::Var };
}

/**
 * @brief 低基数数值列的唯一值数（采样估计，未知/超限返回 -1）
 *
 * 不扫全列：只取起始最多 2000 个非空值估计基数，避免每次规划都做全量 unique。
 * 结果只用于「是否提供『作为分类』候选」与柱状图类型，真正聚合时的 Top N 守卫
 * 由 DAChartQuickPlot 按聚合结果执行。
 */
int DATableChartPlanner::probeUniqueCount(const DATableChartRequest& request, int dataColumn, int limit)
{
    DAData dataCopy            = request.data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || dataColumn < 0) {
        return -1;
    }
    constexpr std::size_t kProbeRows = 2000;
    const std::size_t rows           = source->tableRowCount();
    const std::size_t take           = std::min(rows, kProbeRows);
    if (take == 0) {
        return -1;
    }
    const QVariantList values = source->fetchColumnRange(static_cast< std::size_t >(dataColumn), 0, take);
    QSet< QString > distinct;
    for (const QVariant& v : values) {
        if (!v.isValid() || v.isNull()) {
            continue;
        }
        distinct.insert(v.toString());
        if (distinct.size() > limit) {
            return -1;
        }
    }
    return distinct.isEmpty() ? -1 : distinct.size();
}

}  // namespace DA
