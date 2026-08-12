#ifndef DA_HASH_TABLE_H
#define DA_HASH_TABLE_H

#include <unordered_map>
#include <utility>
#include <functional>
#include <initializer_list>
#include <vector>
#include <algorithm>
#include <limits>

namespace DA
{

/**
 * @brief Hash function for std::pair to be used in unordered containers
 */
struct pair_hash
{
    template< class T1, class T2 >
    std::size_t operator()(const std::pair< T1, T2 >& p) const noexcept;
};

/**
 * @brief 基于 std::unordered_map 的稀疏表格数据结构
 *
 * 此类提供了一个稀疏表格数据结构，使用哈希表存储非默认值，
 * 适合存储稀疏的二维数据，其中大部分元素为默认值。
 *
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型，默认为 std::size_t
 * @tparam col_index_type 列索引类型，默认为 row_index_type
 * @tparam hasher 哈希函数类型，默认为 pair_hash
 */
template< typename T, typename row_index_type = std::size_t, typename col_index_type = row_index_type, typename hasher = pair_hash >
class da_hash_table
{
public:
    using key_type         = std::pair< row_index_type, col_index_type >;
    using value_type       = std::pair< const key_type, T >;
    using table_index_type = key_type;

    // 迭代器类型定义
    using iterator       = typename std::unordered_map< key_type, T, hasher >::iterator;
    using const_iterator = typename std::unordered_map< key_type, T, hasher >::const_iterator;
    using size_type      = typename std::unordered_map< key_type, T, hasher >::size_type;

public:
    // 构造函数
    da_hash_table()                               = default;  // 默认构造函数
    da_hash_table(const da_hash_table& other)     = default;  // 拷贝构造函数
    da_hash_table(da_hash_table&& other) noexcept = default;  // 移动构造函数
    da_hash_table(std::initializer_list< value_type > init);  // 初始化列表构造函数

    // 赋值运算符
    da_hash_table& operator=(const da_hash_table& other)     = default;  // 拷贝赋值运算符
    da_hash_table& operator=(da_hash_table&& other) noexcept = default;  // 移动赋值运算符

    // 元素访问
    T& at(row_index_type r, col_index_type c);                     // 带边界检查的元素访问
    const T& at(row_index_type r, col_index_type c) const;         // 带边界检查的常量元素访问
    T& at(key_type k);                                             // 带边界检查的元素访问
    const T& at(key_type k) const;                                 // 带边界检查的常量元素访问
    T& operator()(row_index_type r, col_index_type c);             // 函数调用运算符访问元素
    const T operator()(row_index_type r, col_index_type c) const;  // 函数调用运算符访问元素（常量版本）
    T& operator()(key_type k);                                     // 函数调用运算符访问元素
    const T operator()(key_type k) const;               // 函数调用运算符访问元素（常量版本）
    T value(row_index_type r, col_index_type c) const;  // 安全的元素访问，返回默认值如果不存在
    T value(row_index_type r, col_index_type c, const T& defaultValue) const;  // 安全的元素访问，返回指定默认值如果不存在
    T& operator[](key_type k);                                                 // 函数调用运算符访问元素
    // 容量操作
    bool empty() const noexcept;          // 检查表格是否为空
    size_type size() const noexcept;      // 返回元素数量
    size_type max_size() const noexcept;  // 返回最大可能元素数量

    // 修改器
    void clear() noexcept;                                        // 清空表格
    std::pair< iterator, bool > insert(const value_type& value);  // 插入元素
    template< class P >
    std::pair< iterator, bool > insert(P&& value);  // 插入元素（通用引用）
    template< class InputIt >
    void insert(InputIt first, InputIt last);                // 插入元素范围
    void insert(std::initializer_list< value_type > ilist);  // 插入初始化列表
    template< class... Args >
    std::pair< iterator, bool > emplace(Args&&... args);        // 原位构造元素
    iterator erase(const_iterator pos);                         // 删除指定位置元素
    iterator erase(const_iterator first, const_iterator last);  // 删除元素范围
    size_type erase(row_index_type r, col_index_type c);        // 删除指定行列元素

    // 查找操作
    iterator find(key_type k);                                      // 查找元素
    const_iterator find(key_type k) const;                          // 查找元素（常量版本）
    iterator find(row_index_type r, col_index_type c);              // 查找元素
    const_iterator find(row_index_type r, col_index_type c) const;  // 查找元素（常量版本）
    size_type count(row_index_type r, col_index_type c) const;      // 统计元素出现次数

    // 迭代器访问
    iterator begin() noexcept;               // 返回指向起始的迭代器
    const_iterator begin() const noexcept;   // 返回指向起始的常量迭代器
    const_iterator cbegin() const noexcept;  // 返回指向起始的常量迭代器
    iterator end() noexcept;                 // 返回指向末尾的迭代器
    const_iterator end() const noexcept;     // 返回指向末尾的常量迭代器
    const_iterator cend() const noexcept;    // 返回指向末尾的常量迭代器

    // 哈希策略
    float load_factor() const noexcept;      // 返回当前负载因子
    float max_load_factor() const noexcept;  // 返回最大负载因子
    void max_load_factor(float ml);          // 设置最大负载因子
    void rehash(size_type count);            // 设置桶数并重新哈希
    void reserve(size_type count);           // 预留空间

    // 桶接口
    size_type bucket_count() const noexcept;      // 返回桶数
    size_type max_bucket_count() const noexcept;  // 返回最大桶数

    // 形状和结构操作
    table_index_type shape() const;                                             // 计算表格形状
    std::vector< row_index_type > row_indices() const;                          // 获取实际使用的行索引列表
    std::vector< col_index_type > column_indices() const;                       // 获取实际使用的列索引列表
    std::vector< std::pair< col_index_type, T > > row(row_index_type r) const;  // 获取指定行的所有元素
    std::vector< std::pair< row_index_type, T > > column(col_index_type c) const;  // 获取指定列的所有元素

    // 其他操作
    void swap(da_hash_table& other) noexcept;  // 交换两个表格的内容

private:
    std::unordered_map< key_type, T, hasher > mData;
};

// Hash function for std::pair to be used in unordered containers
template< class T1, class T2 >
std::size_t pair_hash::operator()(const std::pair< T1, T2 >& p) const noexcept
{
    // 更好的哈希组合方式，减少冲突
    auto h1 = std::hash< T1 > {}(p.first);
    auto h2 = std::hash< T2 > {}(p.second);

    // 使用更安全的哈希组合方法
    // 参考 Boost 的 hash_combine 方法
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

// 初始化列表构造函数
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
da_hash_table< T, row_index_type, col_index_type, hasher >::da_hash_table(std::initializer_list< value_type > init)
    : mData(init)
{
}

// 带边界检查的元素访问
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(row_index_type r, col_index_type c)
{
    return mData.at({ r, c });
}

template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(key_type k)
{
    return mData.at(k);
}

// 带边界检查的常量元素访问
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(row_index_type r, col_index_type c) const
{
    return mData.at({ r, c });
}

template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(key_type k) const
{
    return mData.at(k);
}

// 函数调用运算符访问元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(row_index_type r, col_index_type c)
{
    return mData[ { r, c } ];
}
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(key_type k)
{
    return mData[ k ];
}
// 函数调用运算符访问元素（常量版本）
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(row_index_type r, col_index_type c) const
{
    auto it = mData.find({ r, c });
    return it != mData.end() ? it->second : T();
}
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(key_type k) const
{
    auto it = mData.find(k);
    return it != mData.end() ? it->second : T();
}

template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::operator[](
    da_hash_table< T, row_index_type, col_index_type, hasher >::key_type k)
{
    return mData[ k ];
}

// 安全的元素访问，返回默认值如果不存在
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T da_hash_table< T, row_index_type, col_index_type, hasher >::value(row_index_type r, col_index_type c) const
{
    auto it = mData.find({ r, c });
    return it != mData.end() ? it->second : T();
}

// 安全的元素访问，返回指定默认值如果不存在
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T da_hash_table< T, row_index_type, col_index_type, hasher >::value(row_index_type r, col_index_type c, const T& defaultValue) const
{
    auto it = mData.find({ r, c });
    return it != mData.end() ? it->second : defaultValue;
}

// 检查表格是否为空
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
bool da_hash_table< T, row_index_type, col_index_type, hasher >::empty() const noexcept
{
    return mData.empty();
}

// 返回元素数量
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::size()
    const noexcept
{
    return mData.size();
}

// 返回最大可能元素数量
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::max_size()
    const noexcept
{
    return mData.max_size();
}

// 清空表格
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::clear() noexcept
{
    mData.clear();
}

// 插入元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::insert(const value_type& value)
{
    return mData.insert(value);
}

// 插入元素（通用引用）
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class P >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::insert(P&& value)
{
    return mData.insert(std::forward< P >(value));
}

// 插入元素范围
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class InputIt >
void da_hash_table< T, row_index_type, col_index_type, hasher >::insert(InputIt first, InputIt last)
{
    mData.insert(first, last);
}

// 插入初始化列表
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::insert(std::initializer_list< value_type > ilist)
{
    mData.insert(ilist);
}

// 原位构造元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class... Args >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::emplace(Args&&... args)
{
    return mData.emplace(std::forward< Args >(args)...);
}

// 删除指定位置元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    const_iterator pos)
{
    return mData.erase(pos);
}

// 删除元素范围
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    const_iterator first,
    const_iterator last)
{
    return mData.erase(first, last);
}

// 删除指定行列元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    row_index_type r,
    col_index_type c)
{
    return mData.erase({ r, c });
}

// 查找元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    key_type k)
{
    return mData.find(k);
}

// 查找元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    key_type k) const
{
    return mData.find(k);
}

// 查找元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    row_index_type r,
    col_index_type c)
{
    return mData.find({ r, c });
}

// 查找元素（常量版本）
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    row_index_type r,
    col_index_type c) const
{
    return mData.find({ r, c });
}

// 统计元素出现次数
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::count(
    row_index_type r,
    col_index_type c) const
{
    return mData.count({ r, c });
}

// 返回指向起始的迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::begin() noexcept
{
    return mData.begin();
}

// 返回指向起始的常量迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::begin() const noexcept
{
    return mData.begin();
}

// 返回指向起始的常量迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::cbegin() const noexcept
{
    return mData.cbegin();
}

// 返回指向末尾的迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::end() noexcept
{
    return mData.end();
}

// 返回指向末尾的常量迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::end() const noexcept
{
    return mData.end();
}

// 返回指向末尾的常量迭代器
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::cend() const noexcept
{
    return mData.cend();
}

// 返回当前负载因子
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
float da_hash_table< T, row_index_type, col_index_type, hasher >::load_factor() const noexcept
{
    return mData.load_factor();
}

// 返回最大负载因子
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
float da_hash_table< T, row_index_type, col_index_type, hasher >::max_load_factor() const noexcept
{
    return mData.max_load_factor();
}

// 设置最大负载因子
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::max_load_factor(float ml)
{
    mData.max_load_factor(ml);
}

// 设置桶数并重新哈希
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::rehash(size_type count)
{
    mData.rehash(count);
}

// 预留空间
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::reserve(size_type count)
{
    mData.reserve(count);
}

// 返回桶数
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::
    bucket_count() const noexcept
{
    return mData.bucket_count();
}

// 返回最大桶数
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::
    max_bucket_count() const noexcept
{
    return mData.max_bucket_count();
}

// 计算表格的形状（最大行索引和最大列索引加1）
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::table_index_type da_hash_table< T,
                                                                                                     row_index_type,
                                                                                                     col_index_type,
                                                                                                     hasher >::shape() const
{
    if (mData.empty()) {
        return { row_index_type(), col_index_type() };
    }

    auto it                = mData.cbegin();
    row_index_type max_row = it->first.first;
    col_index_type max_col = it->first.second;

    ++it;
    for (; it != mData.cend(); ++it) {
        if (it->first.first > max_row) {
            max_row = it->first.first;
        }
        if (it->first.second > max_col) {
            max_col = it->first.second;
        }
    }

    // 返回最大索引+1，表示形状
    // 防御性检查：如果 max_row/max_col 已是类型最大值，+1 会溢出
    if (max_row == std::numeric_limits< row_index_type >::max()) {
        return { max_row, max_col + 1 };
    }
    if (max_col == std::numeric_limits< col_index_type >::max()) {
        return { max_row + 1, max_col };
    }
    return { max_row + 1, max_col + 1 };
}

// 获取实际使用的行索引列表
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< row_index_type > da_hash_table< T, row_index_type, col_index_type, hasher >::row_indices() const
{
    std::vector< row_index_type > rows;
    for (const auto& item : mData) {
        rows.push_back(item.first.first);
    }

    // 去重并排序
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    return rows;
}

// 获取实际使用的列索引列表
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< col_index_type > da_hash_table< T, row_index_type, col_index_type, hasher >::column_indices() const
{
    std::vector< col_index_type > cols;
    for (const auto& item : mData) {
        cols.push_back(item.first.second);
    }

    // 去重并排序
    std::sort(cols.begin(), cols.end());
    cols.erase(std::unique(cols.begin(), cols.end()), cols.end());

    return cols;
}

// 获取指定行的所有元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< std::pair< col_index_type, T > > da_hash_table< T, row_index_type, col_index_type, hasher >::row(row_index_type r) const
{
    std::vector< std::pair< col_index_type, T > > result;

    for (const auto& item : mData) {
        if (item.first.first == r) {
            result.emplace_back(item.first.second, item.second);
        }
    }

    // 按列索引排序
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    return result;
}

// 获取指定列的所有元素
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< std::pair< row_index_type, T > > da_hash_table< T, row_index_type, col_index_type, hasher >::column(
    col_index_type c) const
{
    std::vector< std::pair< row_index_type, T > > result;

    for (const auto& item : mData) {
        if (item.first.second == c) {
            result.emplace_back(item.first.first, item.second);
        }
    }

    // 按行索引排序
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    return result;
}

// 交换两个表格的内容
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::swap(da_hash_table& other) noexcept
{
    mData.swap(other.mData);
}

}  // namespace DA

#endif  // DA_HASH_TABLE_H
