#include "DATableMetadataRegistry.h"
#include "DATableMetadata.h"
#include "DADataManager.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param dm 数据管理器，用于连接 datasCleared 信号（非拥有）
 * @param parent 父对象
 */
DATableMetadataRegistry::DATableMetadataRegistry(DADataManager* dm, QObject* parent) : QObject(parent), mDataManager(dm)
{
    if (mDataManager) {
        // 仅 datasCleared 时清空全部；dataRemoved 时保留以支持撤销（DAData 身份跨撤销保持）
        connect(mDataManager, &DADataManager::datasCleared, this, &DATableMetadataRegistry::clear);
    }
}

DATableMetadataRegistry::~DATableMetadataRegistry()
{
    clear();
}

/**
 * @brief 获取或创建某数据的元数据束
 *
 * 束的 QObject parent = this，注册表析构/clear() 时自动销毁。
 * @param d 数据
 * @return 该数据对应的元数据束
 */
DATableMetadata* DATableMetadataRegistry::getOrCreate(const DAData& d)
{
    auto it = mRegistry.find(d);
    if (it != mRegistry.end()) {
        return it.value();
    }
    DATableMetadata* meta = new DATableMetadata(this);
    mRegistry.insert(d, meta);
    Q_EMIT metadataCreated(d, meta);
    return meta;
}

/**
 * @brief 仅查询，不创建
 * @param d 数据
 * @return 不存在返回 nullptr
 */
DATableMetadata* DATableMetadataRegistry::get(const DAData& d) const
{
    auto it = mRegistry.find(d);
    return (it != mRegistry.end()) ? it.value() : nullptr;
}

/**
 * @brief 是否存在该数据的元数据束
 * @param d 数据
 * @return 存在返回 true
 */
bool DATableMetadataRegistry::has(const DAData& d) const
{
    return mRegistry.contains(d);
}

/**
 * @brief 显式移除并销毁某数据的束
 * @param d 数据
 */
void DATableMetadataRegistry::remove(const DAData& d)
{
    auto it = mRegistry.find(d);
    if (it != mRegistry.end()) {
        delete it.value();
        mRegistry.erase(it);
    }
}

/**
 * @brief 清空全部
 */
void DATableMetadataRegistry::clear()
{
    qDeleteAll(mRegistry);
    mRegistry.clear();
}

/**
 * @brief 返回所有非空束及其 DAData
 *
 * 供 DAAppProject 保存遍历使用，跳过 isEmpty() 的束避免写出空条目。
 * @return 非空条目列表
 */
QList< QPair< DAData, DATableMetadata* > > DATableMetadataRegistry::nonEmptyEntries() const
{
    QList< QPair< DAData, DATableMetadata* > > res;
    res.reserve(mRegistry.size());
    for (auto it = mRegistry.constBegin(); it != mRegistry.constEnd(); ++it) {
        DATableMetadata* meta = it.value();
        if (meta && !meta->isEmpty()) {
            res.append(qMakePair(it.key(), meta));
        }
    }
    return res;
}

}  // end of namespace DA
