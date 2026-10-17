#ifndef DATABLEKEYREMAP_H
#define DATABLEKEYREMAP_H
#include "DAGuiAPI.h"
#include <QHash>
#include <QList>
#include <QPair>
#include <QVector>
#include <algorithm>
#include <functional>
#include <utility>

namespace DA
{
/**
 * @brief 表位置键重映射工具（纯头文件模板，元数据束层共享）
 *
 * 行重排（如表头排序，D1 破坏式重排）后，一切按「行位置键」存储的元数据
 * （样式，后续批注/行级校验缓存）都要按键重映射，本工具是唯一的共享实现，
 * 避免每类元数据各写一遍。按维度参数化：行键 QHash<int,T> 一个入口，
 * 单元格键 QHash<QPair<int,int>,T> 一个入口（按行或按列重映射）。
 *
 * **置换语义（钉死，勿改）**：
 * - @code perm[i] = 新表第 i 行在旧表中的行号 @endcode（新→旧，与
 *   @code df.iloc[perm] @endcode / @code numpy.take @endcode 一致）；
 * - 逆置换 @code inv[旧行号] = 新位置 @endcode（旧→新），供 undo 与重映射内部使用。
 * 两个方向命名必须区分：正置换一律叫 perm（新→旧），逆置换一律叫 inv（旧→新）。
 * 设计稿 §1.3 术语表的「旧行号→新位置」描述的是同一置换的逆映射视角，以本注释为准。
 *
 * 复杂度：O(存储条目数 + perm 长度)，不扫全表数据。
 * 边界：perm 为空视为「新表 0 行」，全部键失效（哈希被清空）；未被 perm 覆盖的
 * 旧键（重排后不存在的行）丢弃。
 *
 * 列维度重排（T1.8 列视图）用同一工具按 Dimension::Column 扩展，
 * 本计划不提供空的 onColumnsReordered 占位方法（避免死代码）。
 */
class DATableKeyRemap
{
public:
    // 重映射的维度（单元格键的第一维=行，第二维=列）
    enum class Dimension
    {
        Row,
        Column
    };

    /**
     * @brief 由正置换求逆置换
     *
     * perm[i] = 新表第 i 行在旧表中的行号 → inv[旧行号] = 新位置。
     * inv 长度为 max(perm)+1，未被覆盖的旧行号对应 -1。
     * @param perm 正置换（新→旧）
     * @return 逆置换（旧→新）
     */
    static QVector< int > inversePermutation(const QVector< int >& perm)
    {
        int maxOld = -1;
        for (int oldIdx : perm) {
            if (oldIdx > maxOld) {
                maxOld = oldIdx;
            }
        }
        QVector< int > inv(maxOld + 1, -1);
        for (int newPos = 0; newPos < perm.size(); ++newPos) {
            const int oldIdx = perm[ newPos ];
            if (oldIdx >= 0 && oldIdx <= maxOld) {
                inv[ oldIdx ] = newPos;
            }
        }
        return inv;
    }

    /**
     * @brief 一维键哈希（行键/列键）按置换重映射
     *
     * 新键 = inv[旧键]；未被置换覆盖的旧键丢弃；perm 为空时清空哈希。
     * @param hash 待重映射的哈希（就地替换）
     * @param perm 正置换（新→旧）
     */
    template < typename T >
    static void remapKeys(QHash< int, T >& hash, const QVector< int >& perm)
    {
        if (perm.isEmpty()) {
            hash.clear();
            return;
        }
        const QVector< int > inv = inversePermutation(perm);
        QHash< int, T > remapped;
        remapped.reserve(hash.size());
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            const int oldKey = it.key();
            if (oldKey >= 0 && oldKey < inv.size() && inv[ oldKey ] >= 0) {
                remapped.insert(inv[ oldKey ], it.value());
            }
        }
        hash.swap(remapped);
    }

    /**
     * @brief 二维键哈希（单元格键）按指定维度重映射
     *
     * Dimension::Row 重映射键的第一维（行），Dimension::Column 重映射第二维（列）；
     * 另一维保持不变。未被置换覆盖的旧键丢弃；perm 为空时清空哈希。
     * @param hash 待重映射的哈希（就地替换）
     * @param perm 正置换（新→旧）
     * @param dim 重映射维度
     */
    template < typename T >
    static void remapKeys(QHash< QPair< int, int >, T >& hash, const QVector< int >& perm, Dimension dim)
    {
        if (perm.isEmpty()) {
            hash.clear();
            return;
        }
        const QVector< int > inv = inversePermutation(perm);
        QHash< QPair< int, int >, T > remapped;
        remapped.reserve(hash.size());
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            const int oldKey = (dim == Dimension::Row) ? it.key().first : it.key().second;
            if (oldKey >= 0 && oldKey < inv.size() && inv[ oldKey ] >= 0) {
                const QPair< int, int > newKey =
                    (dim == Dimension::Row) ? qMakePair(inv[ oldKey ], it.key().second)
                                            : qMakePair(it.key().first, inv[ oldKey ]);
                remapped.insert(newKey, it.value());
            }
        }
        hash.swap(remapped);
    }

    /**
     * @brief 一维键哈希按「一批插入索引」位移（003 列维度扩展）
     *
     * 与 DATableStyleManager::onColumnsInserted 语义一致：每个插入索引 idx，
     * 键 >= idx 的条目 +1（降序逐个处理避免连锁覆盖）。行为对行/列维度通用，
     * 由调用方决定键的含义（条件格式规则按列索引分桶）。空列表视为无操作。
     * @param hash 待位移的哈希（就地修改）
     * @param insertedIndexes 插入的索引列表（一批）
     */
    template < typename T >
    static void shiftKeysOnInserted(QHash< int, T >& hash, const QList< int >& insertedIndexes)
    {
        if (insertedIndexes.isEmpty() || hash.isEmpty()) {
            return;
        }
        QList< int > sorted = insertedIndexes;
        std::sort(sorted.begin(), sorted.end(), std::greater< int >());
        for (int idx : std::as_const(sorted)) {
            QHash< int, T > shifted;
            for (auto it = hash.begin(); it != hash.end();) {
                if (it.key() >= idx) {
                    shifted.insert(it.key() + 1, it.value());
                    it = hash.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto sit = shifted.constBegin(); sit != shifted.constEnd(); ++sit) {
                hash.insert(sit.key(), sit.value());
            }
        }
    }

    /**
     * @brief 一维键哈希按「一批删除索引」删除并位移（003 列维度扩展）
     *
     * 与 DATableStyleManager::onColumnsRemoved 语义一致：每个删除索引 idx，
     * 键 == idx 的条目删除，键 > idx 的条目 -1（降序逐个处理）。空列表视为无操作。
     * @param hash 待位移的哈希（就地修改）
     * @param removedIndexes 删除的索引列表（一批）
     */
    template < typename T >
    static void shiftKeysOnRemoved(QHash< int, T >& hash, const QList< int >& removedIndexes)
    {
        if (removedIndexes.isEmpty() || hash.isEmpty()) {
            return;
        }
        QList< int > sorted = removedIndexes;
        std::sort(sorted.begin(), sorted.end(), std::greater< int >());
        for (int idx : std::as_const(sorted)) {
            hash.remove(idx);
            QHash< int, T > shifted;
            for (auto it = hash.begin(); it != hash.end();) {
                if (it.key() > idx) {
                    shifted.insert(it.key() - 1, it.value());
                    it = hash.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto sit = shifted.constBegin(); sit != shifted.constEnd(); ++sit) {
                hash.insert(sit.key(), sit.value());
            }
        }
    }

    /**
     * @brief 二维键哈希（单元格键）按「一批插入索引」位移（005 批注行/列维度扩展）
     *
     * 与一维版同一语义，只位移指定维度：Dimension::Row 位移键的第一维（行），
     * Dimension::Column 位移第二维（列）；每个插入索引 idx，该维 >= idx 的键 +1
     * （降序逐个处理避免连锁覆盖），另一维不变。空列表视为无操作。
     * @param hash 待位移的哈希（就地修改）
     * @param insertedIndexes 插入的索引列表（一批）
     * @param dim 位移维度
     */
    template < typename T >
    static void shiftKeysOnInserted(QHash< QPair< int, int >, T >& hash, const QList< int >& insertedIndexes, Dimension dim)
    {
        if (insertedIndexes.isEmpty() || hash.isEmpty()) {
            return;
        }
        QList< int > sorted = insertedIndexes;
        std::sort(sorted.begin(), sorted.end(), std::greater< int >());
        for (int idx : std::as_const(sorted)) {
            QHash< QPair< int, int >, T > shifted;
            for (auto it = hash.begin(); it != hash.end();) {
                const int keyOnDim = (dim == Dimension::Row) ? it.key().first : it.key().second;
                if (keyOnDim >= idx) {
                    const QPair< int, int > newKey = (dim == Dimension::Row) ? qMakePair(it.key().first + 1, it.key().second)
                                                                             : qMakePair(it.key().first, it.key().second + 1);
                    shifted.insert(newKey, it.value());
                    it = hash.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto sit = shifted.constBegin(); sit != shifted.constEnd(); ++sit) {
                hash.insert(sit.key(), sit.value());
            }
        }
    }

    /**
     * @brief 二维键哈希（单元格键）按「一批删除索引」删除并位移（005 批注行/列维度扩展）
     *
     * 与一维版同一语义，只处理指定维度：每个删除索引 idx，该维 == idx 的条目删除，
     * 该维 > idx 的条目 -1（降序逐个处理），另一维不变。空列表视为无操作。
     * @param hash 待位移的哈希（就地修改）
     * @param removedIndexes 删除的索引列表（一批）
     * @param dim 位移维度
     */
    template < typename T >
    static void shiftKeysOnRemoved(QHash< QPair< int, int >, T >& hash, const QList< int >& removedIndexes, Dimension dim)
    {
        if (removedIndexes.isEmpty() || hash.isEmpty()) {
            return;
        }
        QList< int > sorted = removedIndexes;
        std::sort(sorted.begin(), sorted.end(), std::greater< int >());
        for (int idx : std::as_const(sorted)) {
            QHash< QPair< int, int >, T > shifted;
            for (auto it = hash.begin(); it != hash.end();) {
                const int keyOnDim = (dim == Dimension::Row) ? it.key().first : it.key().second;
                if (keyOnDim == idx) {
                    it = hash.erase(it);
                } else if (keyOnDim > idx) {
                    const QPair< int, int > newKey = (dim == Dimension::Row) ? qMakePair(it.key().first - 1, it.key().second)
                                                                             : qMakePair(it.key().first, it.key().second - 1);
                    shifted.insert(newKey, it.value());
                    it = hash.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto sit = shifted.constBegin(); sit != shifted.constEnd(); ++sit) {
                hash.insert(sit.key(), sit.value());
            }
        }
    }
};
}  // end of namespace DA
#endif  // DATABLEKEYREMAP_H
