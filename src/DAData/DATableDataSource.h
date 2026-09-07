#ifndef DATABLEDATASOURCE_H
#define DATABLEDATASOURCE_H
#include <QString>
#include <QVariant>
#include <cstddef>
#include "DADataAPI.h"
#include "DATableDataBlock.h"
namespace DA
{
/**
 * @brief 表格型数据源的抽象接口（mixin）
 *
 * 此接口刻意不继承 DAAbstractData：数据类通过多继承同时获得"受管理数据"与"表格数据源"
 * 两种身份（如 DADataPyDataFrame : DADataPyObject, DATableDataSource），避免与
 * DADataPyObject 等中间基类形成菱形继承。消费者经 DAAbstractData::tableSource()
 * 取得接口指针，返回非空即代表该数据是表格型数据。
 *
 * 设计约定：
 * - schema（行列数、列名、列类型）与数据本体分离，schema 应为低开销操作
 * - fetchBlock 为同步批量取数接口，行号为从0开始的绝对行号；实现方自带缓存/游标，
 *   数据库等惰性数据源内部以分页查询（如 LIMIT/OFFSET）实现，不应要求全表驻留内存
 * - 接口不承诺线程安全性，默认由GUI线程调用（与现有模型取数路径一致）
 */
class DADATA_API DATableDataSource
{
public:
    virtual ~DATableDataSource();
    // 总行数
    virtual std::size_t tableRowCount() const = 0;
    // 总列数
    virtual std::size_t tableColumnCount() const = 0;
    // 列名
    virtual QString tableColumnName(std::size_t column) const = 0;
    // 列类型（QMetaType类型id），无法确定时返回QMetaType::UnknownType
    virtual int tableColumnType(std::size_t column) const;
    // 批量取从startRow开始的rowCount行，失败返回无效的DATableDataBlock
    virtual DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) = 0;
    // 是否为惰性加载数据源（数据不全量驻留内存，fetchBlock有IO开销）
    virtual bool isLazyLoaded() const;
    // 表格是否可编辑
    virtual bool isTableEditable() const;
};
}  // namespace DA
#endif  // DATABLEDATASOURCE_H
