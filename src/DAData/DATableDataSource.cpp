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

}  // namespace DA
