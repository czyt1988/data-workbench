#include "DATableOperationRegistry.h"
#include "DATableDataOperation.h"

namespace DA
{
//===================================================
// DATableOperationRegistry
//===================================================

/**
 * @brief 注册一个算子
 *
 * @param op 算子实例（生命周期归页面的 QObject 父子，本表只登记指针，不拥有）
 */
void DATableOperationRegistry::add(DATableDataOperation* op)
{
    if (!op || mById.contains(op->id())) {
        return;  // 空/重复 id 忽略（id 是唯一键）
    }
    mOps.append(op);
    mById.insert(op->id(), op);
}

/**
 * @brief 按 id 查找
 *
 * @param id 算子标识（如 "split-column"）
 * @return 算子实例；未注册返回 nullptr
 */
DATableDataOperation* DATableOperationRegistry::find(const QString& id) const
{
    return mById.value(id, nullptr);
}

/**
 * @brief 全部已注册算子（登记顺序）
 */
QList< DATableDataOperation* > DATableOperationRegistry::all() const
{
    return mOps;
}
}  // end of namespace DA
