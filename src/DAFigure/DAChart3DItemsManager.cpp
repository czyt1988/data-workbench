#include "DAChart3DItemsManager.h"
#include <QDebug>
#include <atomic>
#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"

namespace DA
{
namespace
{
/// 进程级 key 序号：工程存档里所有 figure 的 3D 图元共用 chart3d-data/ 目录，key 就是文件名，
/// 跨 figure/跨 3D 子图必须互不重复（每个 3D 子图一个管理者，用各自局部计数会撞名）。
std::atomic< int >& keySequence()
{
    static std::atomic< int > s_sequence { 0 };
    return s_sequence;
}
}  // namespace

DAChart3DItemsManager::DAChart3DItemsManager()
{
}

DAChart3DItemsManager::~DAChart3DItemsManager()
{
}

/**
 * @brief 登记图元并返回它的 key
 *
 * 已登记过的图元直接返回原 key，不重复分配（3D 图元可能被重复 attach，例如撤销/重做重建后
 * 再次挂回同一个 3D 子图）；item 为空返回空串。
 * @param item 图元
 * @return 图元对应的 key
 * @sa recordItem(Qwt3DPlotItem*, const QString&)
 */
QString DAChart3DItemsManager::recordItem(Qwt3DPlotItem* item)
{
    if (!item) {
        return QString();
    }
    const auto it = mItemToKey.constFind(item);
    if (it != mItemToKey.constEnd()) {
        return it.value();
    }
    const QString key = makeItemKey(item);
    recordItem(item, key);
    return key;
}

/**
 * @brief 用指定 key 登记图元
 *
 * 加载存档时使用：key 来自存档索引，figure XML 里的 `<item key>` 要靠它反查图元。
 * 同一图元换 key 登记时先清掉旧映射，避免 itemToKey 与 keyToItem 两个方向不一致。
 * @param item 图元，为空时忽略
 * @param key 键，为空时忽略
 */
void DAChart3DItemsManager::recordItem(Qwt3DPlotItem* item, const QString& key)
{
    if (!item || key.isEmpty()) {
        return;
    }
    const auto oldKeyIt = mItemToKey.constFind(item);
    if (oldKeyIt != mItemToKey.constEnd() && oldKeyIt.value() != key) {
        mKeyToItem.remove(oldKeyIt.value());
    }
    // 同一个 key 被另一个图元占用时，先把旧图元的反查映射摘掉
    const auto oldItemIt = mKeyToItem.constFind(key);
    if (oldItemIt != mKeyToItem.constEnd() && oldItemIt.value() != item) {
        mItemToKey.remove(oldItemIt.value());
    }
    mItemToKey[ item ] = key;
    mKeyToItem[ key ]  = item;
}

/**
 * @brief 注销图元的登记（双向映射一起清）
 * @param item 图元，为空时忽略
 */
void DAChart3DItemsManager::unrecordItem(Qwt3DPlotItem* item)
{
    if (!item) {
        return;
    }
    const auto it = mItemToKey.find(item);
    if (it == mItemToKey.end()) {
        return;
    }
    mKeyToItem.remove(it.value());
    mItemToKey.erase(it);
}

/**
 * @brief 图元对应的 key
 * @param item 图元
 * @return 已登记返回 key，未登记返回空串
 */
QString DAChart3DItemsManager::itemToKey(Qwt3DPlotItem* item) const
{
    return mItemToKey.value(item, QString());
}

/**
 * @brief key 对应的图元
 * @param key 键
 * @return 已登记返回图元，未登记返回 nullptr
 */
Qwt3DPlotItem* DAChart3DItemsManager::keyToItem(const QString& key) const
{
    return mKeyToItem.value(key, nullptr);
}

/**
 * @brief 生成一个唯一的 key（不登记）
 *
 * 形如 `Surface-12`：前缀是 3D 图元 rtti 名，序号是进程级自增（见 @ref keySequence）。
 * @param item 图元，可为空（此时前缀为 Item）
 * @return 新 key
 */
QString DAChart3DItemsManager::makeItemKey(Qwt3DPlotItem* item)
{
    const int rtti = item ? item->rtti() : static_cast< int >(Rtti_Plot3DItem);
    return QStringLiteral("%1-%2").arg(rttiName(rtti)).arg(++keySequence());
}

bool DAChart3DItemsManager::isEmpty() const
{
    return mItemToKey.isEmpty();
}

int DAChart3DItemsManager::size() const
{
    return mItemToKey.size();
}

QList< QString > DAChart3DItemsManager::keys() const
{
    return mKeyToItem.keys();
}

QList< Qwt3DPlotItem* > DAChart3DItemsManager::items() const
{
    return mItemToKey.keys();
}

void DAChart3DItemsManager::clear()
{
    mItemToKey.clear();
    mKeyToItem.clear();
}

/**
 * @brief 3D 图元 rtti 的短名
 * @param rtti Qwt3DPlotItem::rtti() 的值
 * @return Surface/Bar/Line/Waterfall/Item 之一
 */
QString DAChart3DItemsManager::rttiName(int rtti)
{
    switch (rtti) {
    case Rtti_Plot3DSurface:
        return QStringLiteral("Surface");
    case Rtti_Plot3DBar:
        return QStringLiteral("Bar");
    case Rtti_Plot3DLine:
        return QStringLiteral("Line");
    case Rtti_Plot3DWaterfall:
        return QStringLiteral("Waterfall");
    default:
        break;
    }
    return QStringLiteral("Item");
}
}  // namespace DA