#ifndef DATABLE_H
#define DATABLE_H

#include <memory>
#include <functional>
#include <vector>
#include <algorithm>
#include <utility>
#include <iterator>
#include "da_hash_table.hpp"
#include "da_vector_table.hpp"

namespace DA
{

/**
 * @brief 支持稀疏存储和各种算法的表格类
 *
 * 此类提供了一个稀疏表格数据结构，内部使用 da_hash_table 存储数据，
 * 同时维护表格的形状信息，提供方便的表格操作接口。
 *
 * @tparam T 存储的值类型
 */
template< typename T >
class DATable
{
public:
    using Type               = T;                                         ///< 值类型
    using IndexType          = int;                                       ///< 索引类型
    using IndexPair          = std::pair< IndexType, IndexType >;         ///< 索引对类型
    using TableType          = da_hash_table< T, IndexType >;             ///< 内部表格类型
    using TableIterator      = typename TableType::iterator;              ///< 表格迭代器
    using TableConstIterator = typename TableType::const_iterator;        ///< 表格常量迭代器
    using value_type         = typename TableType::value_type;            ///< 兼容std的值类型
    using PredFun            = std::function< bool(const value_type&) >;  ///< 谓词函数类型

public:
    // 构造函数和赋值运算符
    DATable();                                          // 默认构造函数
    DATable(const DATable& other);                      // 拷贝构造函数
    DATable(DATable&& other) noexcept;                  // 移动构造函数
    DATable< T >& operator=(const DATable& other);      // 拷贝赋值运算符
    DATable< T >& operator=(DATable&& other) noexcept;  // 移动赋值运算符

    // 元素访问
    bool contain(int r, int c) const;       // 检查单元格是否有内容
    bool contain(IndexPair i) const;        // 检查单元格是否有内容（使用索引对）
    const T& at(int r, int c) const;        // 带边界检查的元素访问
    T& at(int r, int c);                    // 带边界检查的元素访问
    const T& at(const IndexPair& i) const;  // 带边界检查的元素访问（使用索引对）
    T& at(const IndexPair& i);              // 带边界检查的元素访问（使用索引对）
    T cell(int r, int c) const;             // 安全的元素访问，返回默认值如果不存在
    T cell(const IndexPair& i) const;   // 安全的元素访问，返回默认值如果不存在（使用索引对）
    T& operator[](const IndexPair& i);  // 下标运算符访问元素
    T& operator()(IndexType r, IndexType c);             // 函数调用运算符访问元素
    const T operator()(IndexType r, IndexType c) const;  // 函数调用运算符访问元素（常量版本）
    // 修改器
    void set(const IndexPair& k, const T& v);            // 设置单元格值
    void set(IndexType row, IndexType col, const T& v);  // 设置单元格值

    // 容量操作
    int rowCount() const;      // 获取表格行数
    int columnCount() const;   // 获取表格列数
    IndexPair shape() const;   // 获取表格形状
    std::size_t size() const;  // 获取表格元素数量
    bool empty() const;        // 检查表格是否为空

    // 表格操作
    void recalcShape();                   // 重新计算表格形状
    bool removeCell(int r, int c);        // 移除指定单元格
    bool removeCell(const IndexPair& i);  // 移除指定单元格（使用索引对）
    void clear();                         // 清空表格
    std::size_t erase_if(PredFun pred);   // 按条件删除元素
    void dropColumn(IndexType col);       // 移除指定列

    // 迭代器访问
    TableConstIterator find(const IndexPair& i) const;  // 查找元素
    TableConstIterator find(int r, int c) const;        // 查找元素
    TableIterator find(const IndexPair& i);             // 查找元素
    TableIterator find(int r, int c);                   // 查找元素
    TableConstIterator end() const;                     // 返回末尾迭代器
    TableIterator end();                                // 返回末尾迭代器
    TableConstIterator begin() const;                   // 返回起始迭代器
    TableIterator begin();                              // 返回起始迭代器

    // 转换操作
    template< typename OtherType >
    DATable< OtherType > transfered(std::function< OtherType(const T& v) > trFun) const;  // 转换表格数据类型

    void transferColumn(IndexType col, std::function< bool(const T& v) > trFun) const;  // 遍历指定列

    // 赋值操作
    template< typename OtherType >
    DATable< T >& operator=(const da_vector_table< OtherType >& other);  // 从 da_vector_table 赋值

    // 内部数据访问
    TableType& rawData();              // 获取内部表格数据（不安全）
    const TableType& rawData() const;  // 获取内部表格数据（常量版本，不安全）

    IndexPair getShape() const;

private:
    void reflashShape(const IndexPair& k);  // 刷新形状信息
    std::size_t erase_if__(PredFun pred);   // 内部条件删除实现

private:
    TableType mData;   // 内部表格数据
    IndexPair mShape;  // 表格形状信息
};

// ============================================================================
// 实现部分
// ============================================================================

// 默认构造函数
template< typename T >
DATable< T >::DATable() : mShape(0, 0)
{
}

// 拷贝构造函数
template< typename T >
DATable< T >::DATable(const DATable& other)
{
    mData  = other.mData;
    mShape = other.mShape;
}

// 移动构造函数
template< typename T >
DATable< T >::DATable(DATable&& other) noexcept
{
    mData  = std::move(other.mData);
    mShape = std::move(other.mShape);
}

// 拷贝赋值运算符
template< typename T >
DATable< T >& DATable< T >::operator=(const DATable& other)
{
    mData  = other.mData;
    mShape = other.mShape;
    return *this;
}

// 移动赋值运算符
template< typename T >
DATable< T >& DATable< T >::operator=(DATable&& other) noexcept
{
    mData  = std::move(other.mData);
    mShape = std::move(other.mShape);
    return *this;
}

// 检查单元格是否有内容
template< typename T >
bool DATable< T >::contain(int r, int c) const
{
    return (mData.cend() != mData.find(r, c));
}

// 检查单元格是否有内容（使用索引对）
template< typename T >
bool DATable< T >::contain(IndexPair i) const
{
    return (mData.cend() != mData.find(i));
}

// 获取表格形状
template< typename T >
typename DATable< T >::IndexPair DATable< T >::getShape() const
{
    return mShape;
}

// 带边界检查的元素访问
template< typename T >
const T& DATable< T >::at(int r, int c) const
{
    return mData.at(r, c);
}

// 带边界检查的元素访问
template< typename T >
T& DATable< T >::at(int r, int c)
{
    return mData.at(r, c);
}

// 带边界检查的元素访问（使用索引对）
template< typename T >
const T& DATable< T >::at(const IndexPair& i) const
{
    return mData.at(i);
}

// 带边界检查的元素访问（使用索引对）
template< typename T >
T& DATable< T >::at(const IndexPair& i)
{
    return mData.at(i);
}

// 安全的元素访问，返回默认值如果不存在
template< typename T >
T DATable< T >::cell(int r, int c) const
{
    return mData.value(r, c);
}

// 安全的元素访问，返回默认值如果不存在（使用索引对）
template< typename T >
T DATable< T >::cell(const IndexPair& i) const
{
    return mData.value(i);
}

// 下标运算符访问元素
template< typename T >
T& DATable< T >::operator[](const IndexPair& i)
{
    reflashShape(i);
    return mData[ i ];
}

// 函数调用运算符访问元素
template< typename T >
T& DATable< T >::operator()(IndexType r, IndexType c)
{
    reflashShape(std::make_pair(r, c));
    return mData(r, c);
}

// 函数调用运算符访问元素(常量版本)
template< typename T >
const T DATable< T >::operator()(IndexType r, IndexType c) const
{
    return mData(r, c);
}

// 设置单元格值
template< typename T >
void DATable< T >::set(const IndexPair& k, const T& v)
{
    mData[ k ] = v;
    reflashShape(k);
}

// 设置单元格值
template< typename T >
void DATable< T >::set(IndexType row, IndexType col, const T& v)
{
    set(IndexPair(row, col), v);
}

// 获取表格行数
template< typename T >
int DATable< T >::rowCount() const
{
    return mShape.first;
}

// 获取表格列数
template< typename T >
int DATable< T >::columnCount() const
{
    return mShape.second;
}

// 获取表格形状
template< typename T >
typename DATable< T >::IndexPair DATable< T >::shape() const
{
    return mShape;
}

// 重新计算表格形状
template< typename T >
void DATable< T >::recalcShape()
{
    mShape = mData.shape();
}

// 获取表格元素数量
template< typename T >
std::size_t DATable< T >::size() const
{
    return mData.size();
}

// 移除指定单元格
template< typename T >
bool DATable< T >::removeCell(int r, int c)
{
    auto i = mData.find(r, c);
    if (i != mData.end()) {
        mData.erase(i);
        // 判断是否触发reshape
        if (r == (mShape.first - 1) || c == (mShape.second - 1)) {
            // 但凡有一边接触到边界，在删除后都有重新计算边界
            recalcShape();
        }
        return true;
    }
    return false;
}

// 移除指定单元格（使用索引对）
template< typename T >
bool DATable< T >::removeCell(const IndexPair& i)
{
    return removeCell(i.first, i.second);
}

// 清空表格
template< typename T >
void DATable< T >::clear()
{
    mData.clear();
    mShape = IndexPair(0, 0);
}

// 获取内部表格数据
template< typename T >
typename DATable< T >::TableType& DATable< T >::rawData()
{
    return mData;
}

// 获取内部表格数据（常量版本）
template< typename T >
const typename DATable< T >::TableType& DATable< T >::rawData() const
{
    return mData;
}

// 查找元素
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::find(const IndexPair& i) const
{
    return mData.find(i);
}

// 查找元素
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::find(int r, int c) const
{
    return find(IndexPair(r, c));
}

// 查找元素
template< typename T >
typename DATable< T >::TableIterator DATable< T >::find(const IndexPair& i)
{
    return mData.find(i.first, i.second);
}

// 查找元素
template< typename T >
typename DATable< T >::TableIterator DATable< T >::find(int r, int c)
{
    return find(IndexPair(r, c));
}

// 返回末尾迭代器
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::end() const
{
    return mData.end();
}

// 返回末尾迭代器
template< typename T >
typename DATable< T >::TableIterator DATable< T >::end()
{
    return mData.end();
}

// 返回起始迭代器
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::begin() const
{
    return mData.begin();
}

// 返回起始迭代器
template< typename T >
typename DATable< T >::TableIterator DATable< T >::begin()
{
    return mData.begin();
}

// 检查表格是否为空
template< typename T >
bool DATable< T >::empty() const
{
    return mData.empty();
}

// 按条件删除元素
template< typename T >
std::size_t DATable< T >::erase_if(PredFun pred)
{
    std::size_t r = erase_if__(pred);
    recalcShape();
    return r;
}

// 移除指定列
template< typename T >
void DATable< T >::dropColumn(IndexType col)
{
    // 分两步，第一步删除，第二部移动
    // 先把列号等于col的移除
    // 使用非const key的pair类型，避免const key导致的无法移动
    std::vector< std::pair< IndexPair, T > > temp;
    std::ignore = erase_if__([ col, &temp ](const value_type& v) -> bool {
        if (v.first.second == col) {
            return true;
        } else if (v.first.second > col) {
            // 大于这个列的也要删除，但要把值缓存起来，并进行左移动
            // 预计算新key，避免后续再次拷贝
            IndexPair k = v.first;
            --k.second;
            temp.emplace_back(std::move(k), T(v.second));
            return true;
        }
        return false;
    });
    // 再把缓存的值插入到新位置
    for (auto& [ k, val ] : temp) {
        mData[ k ] = std::move(val);
    }
    recalcShape();
}

// 转换表格数据类型
template< typename T >
template< typename OtherType >
DATable< OtherType > DATable< T >::transfered(std::function< OtherType(const T& v) > trFun) const
{
    DATable< OtherType > other;
    for (auto i = mData.cbegin(), last = mData.cend(); i != last; ++i) {
        other[ i->first ] = trFun(i->second);
    }
    return other;
}

// 遍历指定列
template< typename T >
void DATable< T >::transferColumn(IndexType col, std::function< bool(const T& v) > trFun) const
{
    auto rowCnt = rowCount();
    for (auto r = 0; r < rowCnt; ++r) {
        auto ite = mData.find(IndexPair(r, col));
        if (ite != mData.end()) {
            if (!trFun(ite->second)) {
                return;
            }
        }
    }
}

// 从 da_vector_table 赋值
template< typename T >
template< typename OtherType >
DATable< T >& DATable< T >::operator=(const da_vector_table< OtherType >& other)
{
    clear();
    auto rc = other.row_count();
    auto cc = other.column_count();
    for (auto r = 0; r < rc; ++r) {
        for (auto c = 0; c < cc; ++c) {
            set(r, c, other(r, c));
        }
    }
    return *this;
}

// 刷新形状信息
template< typename T >
void DATable< T >::reflashShape(const IndexPair& k)
{
    if (k.first >= mShape.first) {
        mShape.first = k.first + 1;
    }
    if (k.second >= mShape.second) {
        mShape.second = k.second + 1;
    }
}

// 内部条件删除实现
template< typename T >
std::size_t DATable< T >::erase_if__(PredFun pred)
{
    auto old_size = size();
    for (auto i = mData.begin(), last = mData.end(); i != last;) {
        if (pred(*i)) {
            i = mData.erase(i);
        } else {
            ++i;
        }
    }
    return old_size - size();
}

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

// 实现DATable向da_vector_table的转换
template< typename T1, typename T2 >
da_vector_table< T1 > table_transfered(const DATable< T2 >& table, std::function< T1(const T2&) > tr_fun)
{
    da_vector_table< T1 > res;
    const typename DATable< T2 >::IndexPair sh = table.shape();
    res.resize(sh.first, sh.second);
    for (auto c = table.begin(); c != table.end(); ++c) {
        res(c->first.first, c->first.second) = tr_fun(c->second);
    }
    return res;
}
template< typename T1, typename T2 >
da_vector_table< T1 > table_transfered(const DATable< T2 >& table)
{
    da_vector_table< T1 > res;
    const typename DATable< T2 >::IndexPair sh = table.shape();
    res.resize(sh.first, sh.second);
    for (auto c = table.begin(); c != table.end(); ++c) {
        res(c->first.first, c->first.second) = c->second;
    }
    return res;
}

// 实现da_vector_table向DATable的转换
template< typename T1, typename T2 >
DATable< T1 > table_transfered(const da_vector_table< T2 >& table, std::function< T1(const T2&) > tr_fun)
{
    DATable< T1 > res;
    auto rc = table.row_count();
    auto cc = table.column_count();
    for (std::size_t r = 0; r < rc; ++r) {
        for (std::size_t c = 0; c < cc; ++c) {
            res.set(r, c, tr_fun(table(r, c)));
        }
    }
    return res;
}

}  // end DA

#endif  // DATABLE_H
