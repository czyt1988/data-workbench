#include "DAChartSeriesSampler.h"
#include "DATableDataSource.h"
#include <QDateTime>
#include <QMetaType>
#include <QVariant>
#include <cmath>
#include <algorithm>
#include <limits>

namespace DA
{

namespace
{

/// 一段连续行区间 [start, start + count)
struct RowRange
{
    qint64 start { 0 };
    int count { 0 };
};

/**
 * @brief 把全区间切成若干连续窗口（确定性，不引入随机数）
 *
 * 小表直接返回一个全量窗口；大表按 kSampleWindowCount 个窗口均匀铺满全区间，
 * 每个窗口取前 perWindow 行，从而在不放大取数次数的前提下覆盖整个行区间。
 */
QVector< RowRange > buildRanges(qint64 totalRows, int maxPoints)
{
    QVector< RowRange > ranges;
    if (totalRows <= 0) {
        return ranges;
    }
    if (totalRows <= DAChartSeriesSampler::kFullPlotRowThreshold) {
        ranges.append(RowRange { 0, static_cast< int >(totalRows) });
        return ranges;
    }
    if (maxPoints <= 0) {
        maxPoints = DAChartSeriesSampler::kMaxLinePoints;
    }
    const int windowCount = std::max(1, std::min(DAChartSeriesSampler::kSampleWindowCount, maxPoints - 1));
    // 预留一个窗口给「尾部对齐」窗口，保证最后一行不会被均匀采样漏掉
    const int perWindow = std::max(1, maxPoints / (windowCount + 1));
    for (int i = 0; i < windowCount; ++i) {
        const qint64 start      = totalRows * i / windowCount;
        const qint64 nextStart  = totalRows * (i + 1) / windowCount;
        const qint64 available  = nextStart - start;
        if (available <= 0) {
            continue;
        }
        ranges.append(RowRange { start, static_cast< int >(std::min< qint64 >(perWindow, available)) });
    }
    // 尾部窗口：从 totalRows - perWindow 取到结尾，使采样点覆盖行区间两端
    const qint64 tailStart = std::max< qint64 >(0, totalRows - perWindow);
    if (ranges.isEmpty() || ranges.constLast().start + ranges.constLast().count < totalRows) {
        ranges.append(RowRange { tailStart, static_cast< int >(totalRows - tailStart) });
    }
    return ranges;
}

/// 把单元格值转成 double；不可转换返回 false
bool cellToDouble(const QVariant& v, double& out)
{
    if (!v.isValid() || v.isNull()) {
        return false;
    }
    const int t = v.userType();
    switch (t) {
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Long:
    case QMetaType::ULong:
        out = v.toDouble();
        return true;
    case QMetaType::Bool:
        out = v.toBool() ? 1.0 : 0.0;
        return true;
    case QMetaType::QDateTime: {
        const QDateTime dt = v.toDateTime();
        if (!dt.isValid()) {
            return false;
        }
        out = static_cast< double >(dt.toMSecsSinceEpoch());
        return true;
    }
    case QMetaType::QDate: {
        const QDate d = v.toDate();
        if (!d.isValid()) {
            return false;
        }
        out = static_cast< double >(QDateTime(d, QTime(0, 0)).toMSecsSinceEpoch());
        return true;
    }
    case QMetaType::QString:
    case QMetaType::QChar: {
        bool ok = false;
        out     = v.toString().toDouble(&ok);
        return ok;
    }
    default:
        break;
    }
    bool ok = false;
    out     = v.toDouble(&ok);
    return ok && !std::isnan(out);
}

QVariantList fetchColumn(DATableDataSource* source, int dataColumn, qint64 start, int count)
{
    if (source == nullptr || dataColumn < 0 || count <= 0) {
        return QVariantList();
    }
    return source->fetchColumnRange(static_cast< std::size_t >(dataColumn),
                                    static_cast< std::size_t >(start),
                                    static_cast< std::size_t >(count));
}

/// 分类标签：缺失/非法返回 false；正常值转成显示字符串
bool cellToLabel(const QVariant& v, QString& out)
{
    if (!v.isValid() || v.isNull()) {
        return false;
    }
    if (v.userType() == QMetaType::QString) {
        out = v.toString();
        return !out.isEmpty();
    }
    if (v.userType() == QMetaType::QDateTime) {
        out = v.toDateTime().toString(Qt::ISODate);
        return !out.isEmpty();
    }
    if (v.userType() == QMetaType::QDate) {
        out = v.toDate().toString(Qt::ISODate);
        return !out.isEmpty();
    }
    if (v.userType() == QMetaType::Bool) {
        out = v.toBool() ? QStringLiteral("True") : QStringLiteral("False");
        return true;
    }
    out = v.toString();
    return !out.isEmpty();
}

}  // namespace

/**
 * @brief 是否可以对全量数据做精确聚合/分位数
 *
 * 内存 pandas 源且行数在阈值内才是精确的；惰性/数据库源与超阈值大表
 * 走采样路径，调用方必须把结果标注为近似。
 */
bool DAChartSeriesSampler::canAggregateExactly(const DATableChartRequest& request)
{
    if (request.lazy || !request.data.isDataFrame()) {
        return false;
    }
    return static_cast< std::int64_t >(request.rowCount) <= kExactAggregationRowLimit;
}

/**
 * @brief 采样一条 xy 序列
 *
 * x 为行号时取原始绝对行号，保证与表格行位置对应；x 为列时逐行成对取值，
 * 任一侧缺失都整点剔除并计入 droppedMissing。
 */
DAChartSeriesSampler::XYSample DAChartSeriesSampler::sampleXY(const DATableChartRequest& request,
                                                              int xDataColumn,
                                                              int yDataColumn,
                                                              int maxPoints,
                                                              SampleBudget* budget)
{
    XYSample result;
    SampleBudget defaultBudget;
    SampleBudget* usedBudget = budget ? budget : &defaultBudget;
    DAData dataCopy            = request.data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || yDataColumn < 0) {
        return result;
    }
    const qint64 totalRows = static_cast< qint64 >(request.rowCount);
    result.info.totalRows  = static_cast< int >(std::min< qint64 >(totalRows, std::numeric_limits< int >::max()));
    const QVector< RowRange > ranges = buildRanges(totalRows, maxPoints);
    result.info.sampled              = (ranges.size() != 1 || (ranges.size() == 1 && ranges.first().count < totalRows));

    for (const RowRange& range : ranges) {
        // 预算：这一轮要取 1~2 块，任何一块取不了就停止采样并标记受限（实施方案 §8.4）
        const int callsNeeded = (xDataColumn >= 0) ? 2 : 1;
        bool budgetOk         = true;
        for (int c = 0; c < callsNeeded; ++c) {
            if (!usedBudget->request(range.count)) {
                budgetOk = false;
                break;
            }
        }
        if (!budgetOk) {
            result.info.limited = true;
            break;
        }
        const QVariantList ys = fetchColumn(source, yDataColumn, range.start, range.count);
        const QVariantList xs = (xDataColumn >= 0) ? fetchColumn(source, xDataColumn, range.start, range.count) : QVariantList();
        const int n           = ys.size();
        for (int i = 0; i < n; ++i) {
            double y = 0.0;
            if (!cellToDouble(ys.at(i), y)) {
                ++result.info.droppedMissing;
                continue;
            }
            double x = static_cast< double >(range.start + i);
            if (xDataColumn >= 0) {
                if (i >= xs.size() || !cellToDouble(xs.at(i), x)) {
                    ++result.info.droppedMissing;
                    continue;
                }
            }
            result.points.append(QPointF(x, y));
        }
    }
    result.info.sampledRows = result.points.size();
    return result;
}

/**
 * @brief 采样多序列 xy（同一个 x，按行对齐）
 *
 * 多序列要求各行在所有 y 列上都有有效值，任一列缺失则整行剔除，
 * 避免不同序列的点位错行。
 */
DAChartSeriesSampler::MultiXYSample DAChartSeriesSampler::sampleMultiXY(const DATableChartRequest& request,
                                                                        int xDataColumn,
                                                                        const QList< int >& yDataColumns,
                                                                        int maxPoints,
                                                                        SampleBudget* budget)
{
    MultiXYSample result;
    SampleBudget defaultBudget;
    SampleBudget* usedBudget = budget ? budget : &defaultBudget;
    DAData dataCopy            = request.data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || yDataColumns.isEmpty()) {
        return result;
    }
    for (int i = 0; i < yDataColumns.size(); ++i) {
        result.series.append(QVector< QPointF >());
    }
    for (int idx : yDataColumns) {
        result.names.append(request.columns.size() > idx && idx >= 0 ? request.columns.at(idx).name : QString());
    }
    const qint64 totalRows = static_cast< qint64 >(request.rowCount);
    result.info.totalRows  = static_cast< int >(std::min< qint64 >(totalRows, std::numeric_limits< int >::max()));
    const QVector< RowRange > ranges = buildRanges(totalRows, maxPoints);
    result.info.sampled              = (ranges.size() != 1 || (ranges.size() == 1 && ranges.first().count < totalRows));

    for (const RowRange& range : ranges) {
        // 每轮取 1(可选 x) + 每列 y 各一块；预算不足立即停止并标记受限
        const int callsNeeded = static_cast< int >(yDataColumns.size()) + (xDataColumn >= 0 ? 1 : 0);
        bool budgetOk         = true;
        for (int c = 0; c < callsNeeded; ++c) {
            if (!usedBudget->request(range.count)) {
                budgetOk = false;
                break;
            }
        }
        if (!budgetOk) {
            result.info.limited = true;
            break;
        }
        QList< QVariantList > columns;
        qsizetype minSize = -1;
        for (int colIdx : yDataColumns) {
            // yDataColumns 是 request.columns 下标，需要换成数据列号
            const int dataCol = (colIdx >= 0 && colIdx < request.columns.size()) ? request.columns.at(colIdx).dataIndex : -1;
            const QVariantList values = fetchColumn(source, dataCol, range.start, range.count);
            columns.append(values);
            const qsizetype n = values.size();
            minSize = (minSize < 0) ? n : std::min(minSize, n);
        }
        if (minSize <= 0) {
            continue;
        }
        QVariantList xs;
        if (xDataColumn >= 0) {
            const int dataCol = (xDataColumn < request.columns.size()) ? request.columns.at(xDataColumn).dataIndex : -1;
            xs                = fetchColumn(source, dataCol, range.start, range.count);
        }
        for (qsizetype i = 0; i < minSize; ++i) {
            QVector< double > ys;
            ys.reserve(columns.size());
            bool rowOk = true;
            for (const QVariantList& values : std::as_const(columns)) {
                double y = 0.0;
                if (!cellToDouble(values.at(i), y)) {
                    rowOk = false;
                    break;
                }
                ys.append(y);
            }
            if (!rowOk) {
                ++result.info.droppedMissing;
                continue;
            }
            double x = static_cast< double >(range.start + i);
            if (xDataColumn >= 0) {
                if (i >= xs.size() || !cellToDouble(xs.at(i), x)) {
                    ++result.info.droppedMissing;
                    continue;
                }
            }
            for (int s = 0; s < ys.size(); ++s) {
                result.series[ s ].append(QPointF(x, ys.at(s)));
            }
        }
    }
    result.info.sampledRows = result.series.isEmpty() ? 0 : result.series.first().size();
    return result;
}

/**
 * @brief 采样单列数值（直方图/箱线图）
 */
QVector< double > DAChartSeriesSampler::sampleValues(const DATableChartRequest& request,
                                                     int dataColumn,
                                                     int maxValues,
                                                     SampleInfo* info,
                                                     SampleBudget* budget)
{
    QVector< double > values;
    SampleInfo local;
    SampleBudget defaultBudget;
    SampleBudget* usedBudget = budget ? budget : &defaultBudget;
    DAData dataCopy            = request.data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || dataColumn < 0) {
        if (info) {
            *info = local;
        }
        return values;
    }
    const qint64 totalRows = static_cast< qint64 >(request.rowCount);
    local.totalRows        = static_cast< int >(std::min< qint64 >(totalRows, std::numeric_limits< int >::max()));
    const QVector< RowRange > ranges = buildRanges(totalRows, maxValues);
    local.sampled                    = (ranges.size() != 1 || (ranges.size() == 1 && ranges.first().count < totalRows));

    for (const RowRange& range : ranges) {
        if (!usedBudget->request(range.count)) {
            local.limited = true;
            break;
        }
        const QVariantList cells = fetchColumn(source, dataColumn, range.start, range.count);
        for (const QVariant& v : cells) {
            double d = 0.0;
            if (cellToDouble(v, d)) {
                values.append(d);
            } else {
                ++local.droppedMissing;
            }
        }
    }
    local.sampledRows = values.size();
    if (info) {
        *info = local;
    }
    return values;
}

/**
 * @brief 采样分类列（可选数值列）
 */
DAChartSeriesSampler::CategorySample DAChartSeriesSampler::sampleCategory(const DATableChartRequest& request,
                                                                          int categoryDataColumn,
                                                                          int valueDataColumn,
                                                                          int maxPoints,
                                                                          SampleBudget* budget)
{
    CategorySample result;
    SampleBudget defaultBudget;
    SampleBudget* usedBudget = budget ? budget : &defaultBudget;
    DAData dataCopy            = request.data;  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source == nullptr || categoryDataColumn < 0) {
        return result;
    }
    const qint64 totalRows = static_cast< qint64 >(request.rowCount);
    result.info.totalRows  = static_cast< int >(std::min< qint64 >(totalRows, std::numeric_limits< int >::max()));
    const QVector< RowRange > ranges = buildRanges(totalRows, maxPoints);
    result.info.sampled              = (ranges.size() != 1 || (ranges.size() == 1 && ranges.first().count < totalRows));

    for (const RowRange& range : ranges) {
        const int callsNeeded = (valueDataColumn >= 0) ? 2 : 1;
        bool budgetOk         = true;
        for (int c = 0; c < callsNeeded; ++c) {
            if (!usedBudget->request(range.count)) {
                budgetOk = false;
                break;
            }
        }
        if (!budgetOk) {
            result.info.limited = true;
            break;
        }
        const QVariantList cats   = fetchColumn(source, categoryDataColumn, range.start, range.count);
        const QVariantList values = (valueDataColumn >= 0) ? fetchColumn(source, valueDataColumn, range.start, range.count) : QVariantList();
        const int n               = cats.size();
        for (int i = 0; i < n; ++i) {
            QString label;
            if (!cellToLabel(cats.at(i), label)) {
                ++result.info.droppedMissing;
                continue;
            }
            double value = 1.0;
            if (valueDataColumn >= 0) {
                if (i >= values.size() || !cellToDouble(values.at(i), value)) {
                    ++result.info.droppedMissing;
                    continue;
                }
            }
            result.labels.append(label);
            result.values.append(value);
        }
        result.info.sampledRows = result.labels.size();
    }
    return result;
}

}  // namespace DA
