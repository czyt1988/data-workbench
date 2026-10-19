#ifndef DATABLESCAN_H
#define DATABLESCAN_H
#include "DADataAPI.h"
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>
#include <QList>

namespace DA
{
/// 唯一值数"未计算"哨兵（选区超过 uniqueLimit，或该列口径上不需要唯一值数）
constexpr std::size_t kUniqueNotComputed = std::numeric_limits< std::size_t >::max();

/// 一次列扫描请求：v1 只支持单个连续行区间（多段选区只显示计数，不到这里）
struct DATableScanRequest {
    std::size_t rowBegin { 0 };          // 数据行号，半开区间 [rowBegin, rowEnd)
    std::size_t rowEnd { 0 };
    std::vector< std::size_t > columns;  // 列序号（位置键，不持久化）
    std::size_t uniqueLimit { 0 };       // 0 = 不要唯一值数；行数超过它时返回 kUniqueNotComputed
};

/// 单列统计结果
struct DATableColumnStats {
    std::size_t count { 0 };             // 非空计数
    std::size_t nullCount { 0 };
    std::size_t uniqueCount { 0 };       // kUniqueNotComputed = 未计算（显示"—"）
    std::optional< double > sum, mean, min, max;   // 非数值列（文本/日期/布尔）留空 => "该列不适用数值指标"
};
using DATableStatsResult = QList< DATableColumnStats >;   // 与 req.columns 同序
}  // namespace DA
#endif  // DATABLESCAN_H
