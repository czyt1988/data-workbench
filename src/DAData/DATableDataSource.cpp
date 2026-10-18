#include "DATableDataSource.h"
#include <QMetaType>
namespace DA
{

//===================================================
// DATableDataSource
//===================================================

/**
 * @brief 虚析构函数
 */
DATableDataSource::~DATableDataSource()
{
}

/**
 * @brief 列类型（QMetaType类型id）
 *
 * 默认无法确定列类型，子类可按数据源schema给出更精确的类型
 * @param column 列号
 * @return 默认返回QMetaType::UnknownType
 */
int DATableDataSource::tableColumnType(std::size_t column) const
{
    Q_UNUSED(column);
    return QMetaType::UnknownType;
}

/**
 * @brief 是否为惰性加载数据源
 *
 * 惰性数据源的数据不全量驻留内存，fetchBlock存在IO开销，
 * 消费端（如表格模型）应对块数据进行缓存并避免高频小块取数
 * @return 默认返回false（数据全量在内存）
 */
bool DATableDataSource::isLazyLoaded() const
{
    return false;
}

/**
 * @brief 表格是否可编辑
 * @return 默认返回true
 */
bool DATableDataSource::isTableEditable() const
{
    return true;
}

/**
 * @brief 源侧列统计（默认不支持）
 *
 * 基类默认返回 nullopt（口径同 isLazyLoaded/isTableEditable 的默认值风格）；
 * 调用方据此明说"当前数据源不支持选区统计"。pandas 源（DADataPyDataFrame）
 * 覆写本方法走 numpy 一次算完；数据库源将来翻译成 SELECT 聚合下推。
 * @param req 扫描请求（行区间 + 列序号 + 唯一值数阈值）
 * @return 统计结果列表；不支持时 nullopt
 */
std::optional< DATableStatsResult > DATableDataSource::computeColumnStats(const DATableScanRequest& req) const
{
    Q_UNUSED(req);
    return std::nullopt;
}

}  // namespace DA
