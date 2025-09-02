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
    const T& operator[](const IndexPair& i) const;  // 下标运算符访问元素（常量版本）

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

/**
 * @brief 默认构造函数
 * @tparam T 存储的值类型
 *
 * @code{.cpp}
 * DA::DATable<int> table; // 创建一个空的表格
 * @endcode
 */
template< typename T >
DATable< T >::DATable() : mShape(0, 0)
{
}

/**
 * @brief 拷贝构造函数
 * @tparam T 存储的值类型
 * @param other 要拷贝的表格
 *
 * @code{.cpp}
 * DA::DATable<int> table1;
 * table1.set(0, 0, 42);
 * DA::DATable<int> table2(table1); // 拷贝构造
 * @endcode
 */
template< typename T >
DATable< T >::DATable(const DATable& other)
{
    mData  = other.mData;
    mShape = other.mShape;
}

/**
 * @brief 移动构造函数
 * @tparam T 存储的值类型
 * @param other 要移动的表格
 *
 * @code{.cpp}
 * DA::DATable<int> createTable() {
 *     DA::DATable<int> table;
 *     table.set(0, 0, 42);
 *     return table; // 使用移动语义
 * }
 *
 * auto table = createTable(); // 高效，不会拷贝数据
 * @endcode
 */
template< typename T >
DATable< T >::DATable(DATable&& other) noexcept
{
    mData  = std::move(other.mData);
    mShape = std::move(other.mShape);
}

/**
 * @brief 拷贝赋值运算符
 * @tparam T 存储的值类型
 * @param other 要拷贝的表格
 * @return 当前表格的引用
 *
 * @code{.cpp}
 * DA::DATable<int> table1;
 * table1.set(0, 0, 42);
 * DA::DATable<int> table2;
 * table2 = table1; // 拷贝赋值
 * @endcode
 */
template< typename T >
DATable< T >& DATable< T >::operator=(const DATable& other)
{
    mData  = other.mData;
    mShape = other.mShape;
    return *this;
}

/**
 * @brief 移动赋值运算符
 * @tparam T 存储的值类型
 * @param other 要移动的表格
 * @return 当前表格的引用
 *
 * @code{.cpp}
 * DA::DATable<int> table1;
 * table1.set(0, 0, 42);
 * DA::DATable<int> table2;
 * table2 = std::move(table1); // 移动赋值，table1变为空
 * @endcode
 */
template< typename T >
DATable< T >& DATable< T >::operator=(DATable&& other) noexcept
{
    mData  = std::move(other.mData);
    mShape = std::move(other.mShape);
    return *this;
}

/**
 * @brief 检查单元格是否有内容
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 如果单元格有内容返回true，否则返回false
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * bool hasValue = table.contain(0, 0); // 返回true
 * bool noValue = table.contain(1, 1); // 返回false
 * @endcode
 */
template< typename T >
bool DATable< T >::contain(int r, int c) const
{
    return (mData.cend() != mData.find({ r, c }));
}

/**
 * @brief 检查单元格是否有内容（使用索引对）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 如果单元格有内容返回true，否则返回false
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * bool hasValue = table.contain({0, 0}); // 返回true
 * bool noValue = table.contain({1, 1}); // 返回false
 * @endcode
 */
template< typename T >
bool DATable< T >::contain(IndexPair i) const
{
    return (mData.cend() != mData.find(i));
}

/**
 * @brief 获取表格形状
 * @tparam T 存储的值类型
 * @return 包含最大行索引和最大列索引的pair
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(2, 3, 42);
 * auto shape = table.getShape(); // 返回(3, 4)
 * std::cout << "表格形状: " << shape.first << " x " << shape.second << std::endl;
 * // 输出: 表格形状: 3 x 4
 * @endcode
 */
template< typename T >
typename DATable< T >::IndexPair DATable< T >::getShape() const
{
    return mShape;
}

/**
 * @brief 带边界检查的元素访问
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的常量引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * try {
 *     const int& value = table.at(0, 0); // 返回42
 *     const int& invalid = table.at(1, 1); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T >
const T& DATable< T >::at(int r, int c) const
{
    return mData.at(r, c);
}

/**
 * @brief 带边界检查的元素访问
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * try {
 *     int& value = table.at(0, 0); // 返回42
 *     value = 43; // 修改元素值
 *     int& invalid = table.at(1, 1); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T >
T& DATable< T >::at(int r, int c)
{
    return mData.at(r, c);
}

/**
 * @brief 带边界检查的元素访问（使用索引对）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 元素的常量引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * try {
 *     const int& value = table.at({0, 0}); // 返回42
 *     const int& invalid = table.at({1, 1}); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T >
const T& DATable< T >::at(const IndexPair& i) const
{
    return mData.at(i);
}

/**
 * @brief 带边界检查的元素访问（使用索引对）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 元素的引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * try {
 *     int& value = table.at({0, 0}); // 返回42
 *     value = 43; // 修改元素值
 *     int& invalid = table.at({1, 1}); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T >
T& DATable< T >::at(const IndexPair& i)
{
    return mData.at(i);
}

/**
 * @brief 安全的元素访问，返回默认值如果不存在
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的值或默认构造的值
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * int value = table.cell(0, 0); // 返回42
 * int missing = table.cell(1, 1); // 返回0（默认构造的int）
 * @endcode
 */
template< typename T >
T DATable< T >::cell(int r, int c) const
{
    return mData.value(r, c);
}

/**
 * @brief 安全的元素访问，返回默认值如果不存在（使用索引对）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 元素的值或默认构造的值
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * int value = table.cell({0, 0}); // 返回42
 * int missing = table.cell({1, 1}); // 返回0（默认构造的int）
 * @endcode
 */
template< typename T >
T DATable< T >::cell(const IndexPair& i) const
{
    return mData.value(i);
}

/**
 * @brief 下标运算符访问元素
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 元素的引用
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table[{0, 0}] = 42; // 设置元素值
 * int value = table[{0, 0}]; // 获取元素值
 * @endcode
 */
template< typename T >
T& DATable< T >::operator[](const IndexPair& i)
{
    reflashShape(i);
    return mData[ i ];
}

/**
 * @brief 下标运算符访问元素（常量版本）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 元素的常量引用
 *
 * @code{.cpp}
 * const DA::DATable<int> table;
 * // 假设表格已有数据
 * int value = table[{0, 0}]; // 获取元素值
 * @endcode
 */
template< typename T >
const T& DATable< T >::operator[](const IndexPair& i) const
{
    return mData[ i ];
}

/**
 * @brief 设置单元格值
 * @tparam T 存储的值类型
 * @param k 索引对
 * @param v 要设置的值
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set({0, 0}, 42); // 设置单元格值
 * @endcode
 */
template< typename T >
void DATable< T >::set(const IndexPair& k, const T& v)
{
    mData[ k ] = v;
    reflashShape(k);
}

/**
 * @brief 设置单元格值
 * @tparam T 存储的值类型
 * @param row 行索引
 * @param col 列索引
 * @param v 要设置的值
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42); // 设置单元格值
 * @endcode
 */
template< typename T >
void DATable< T >::set(IndexType row, IndexType col, const T& v)
{
    set(IndexPair(row, col), v);
}

/**
 * @brief 获取表格行数
 * @tparam T 存储的值类型
 * @return 表格的行数
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(2, 3, 42);
 * int rows = table.rowCount(); // 返回3
 * @endcode
 */
template< typename T >
int DATable< T >::rowCount() const
{
    return mShape.first;
}

/**
 * @brief 获取表格列数
 * @tparam T 存储的值类型
 * @return 表格的列数
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(2, 3, 42);
 * int cols = table.columnCount(); // 返回4
 * @endcode
 */
template< typename T >
int DATable< T >::columnCount() const
{
    return mShape.second;
}

/**
 * @brief 获取表格形状
 * @tparam T 存储的值类型
 * @return 包含行数和列数的pair
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(2, 3, 42);
 * auto shape = table.shape(); // 返回(3, 4)
 * @endcode
 */
template< typename T >
typename DATable< T >::IndexPair DATable< T >::shape() const
{
    return mShape;
}

/**
 * @brief 重新计算表格形状
 * @tparam T 存储的值类型
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * // 直接操作内部数据后
 * table.recalcShape(); // 重新计算形状
 * @endcode
 */
template< typename T >
void DATable< T >::recalcShape()
{
    mShape = mData.shape();
}

/**
 * @brief 获取表格元素数量
 * @tparam T 存储的值类型
 * @return 表格中的元素数量
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * table.set(1, 1, 43);
 * std::size_t count = table.size(); // 返回2
 * @endcode
 */
template< typename T >
std::size_t DATable< T >::size() const
{
    return mData.size();
}

/**
 * @brief 移除指定单元格
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 如果成功移除返回true，否则返回false
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * bool removed = table.removeCell(0, 0); // 返回true
 * bool notRemoved = table.removeCell(1, 1); // 返回false
 * @endcode
 */
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

/**
 * @brief 移除指定单元格（使用索引对）
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 如果成功移除返回true，否则返回false
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * bool removed = table.removeCell({0, 0}); // 返回true
 * bool notRemoved = table.removeCell({1, 1}); // 返回false
 * @endcode
 */
template< typename T >
bool DATable< T >::removeCell(const IndexPair& i)
{
    return removeCell(i.first, i.second);
}

/**
 * @brief 清空表格
 * @tparam T 存储的值类型
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * table.clear(); // 清空表格
 * // 现在表格为空
 * @endcode
 */
template< typename T >
void DATable< T >::clear()
{
    mData.clear();
    mShape = IndexPair(0, 0);
}

/**
 * @brief 获取内部表格数据
 * @tparam T 存储的值类型
 * @return 内部表格数据的引用
 * @note 此函数不安全，写操作完成后应该调用recalcShape
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * auto& data = table.rawData();
 * // 直接操作内部数据
 * data[{0, 0}] = 42;
 * // 操作完成后需要重新计算形状
 * table.recalcShape();
 * @endcode
 */
template< typename T >
typename DATable< T >::TableType& DATable< T >::rawData()
{
    return mData;
}

/**
 * @brief 获取内部表格数据（常量版本）
 * @tparam T 存储的值类型
 * @return 内部表格数据的常量引用
 *
 * @code{.cpp}
 * const DA::DATable<int> table;
 * const auto& data = table.rawData();
 * // 只能读取内部数据
 * @endcode
 */
template< typename T >
const typename DATable< T >::TableType& DATable< T >::rawData() const
{
    return mData;
}

/**
 * @brief 查找元素
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 指向元素的常量迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * auto it = table.find({0, 0});
 * if (it != table.end()) {
 *     std::cout << "找到元素: " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::find(const IndexPair& i) const
{
    return mData.find(i);
}

/**
 * @brief 查找元素
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 指向元素的常量迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * auto it = table.find(0, 0);
 * if (it != table.end()) {
 *     std::cout << "找到元素: " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::find(int r, int c) const
{
    return find(IndexPair(r, c));
}

/**
 * @brief 查找元素
 * @tparam T 存储的值类型
 * @param i 索引对
 * @return 指向元素的迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * auto it = table.find({0, 0});
 * if (it != table.end()) {
 *     it->second = 43; // 修改元素值
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableIterator DATable< T >::find(const IndexPair& i)
{
    return mData.find(i.first, i.second);
}

/**
 * @brief 查找元素
 * @tparam T 存储的值类型
 * @param r 行索引
 * @param c 列索引
 * @return 指向元素的迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * auto it = table.find(0, 0);
 * if (it != table.end()) {
 *     it->second = 43; // 修改元素值
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableIterator DATable< T >::find(int r, int c)
{
    return find(IndexPair(r, c));
}

/**
 * @brief 返回末尾迭代器
 * @tparam T 存储的值类型
 * @return 指向表格末尾的常量迭代器
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::end() const
{
    return mData.end();
}

/**
 * @brief 返回末尾迭代器
 * @tparam T 存储的值类型
 * @return 指向表格末尾的迭代器
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableIterator DATable< T >::end()
{
    return mData.end();
}

/**
 * @brief 返回起始迭代器
 * @tparam T 存储的值类型
 * @return 指向表格起始的常量迭代器
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableConstIterator DATable< T >::begin() const
{
    return mData.begin();
}

/**
 * @brief 返回起始迭代器
 * @tparam T 存储的值类型
 * @return 指向表格起始的迭代器
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T >
typename DATable< T >::TableIterator DATable< T >::begin()
{
    return mData.begin();
}

/**
 * @brief 检查表格是否为空
 * @tparam T 存储的值类型
 * @return 如果表格为空返回true，否则返回false
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * if (table.empty()) {
 *     std::cout << "表格为空" << std::endl;
 * }
 * @endcode
 */
template< typename T >
bool DATable< T >::empty() const
{
    return mData.empty();
}

/**
 * @brief 按条件删除元素
 * @tparam T 存储的值类型
 * @param pred 谓词函数，返回true表示删除元素
 * @return 删除的元素数量
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * table.set(1, 1, 43);
 * // 删除值等于42的元素
 * std::size_t count = table.erase_if([](const auto& v) { return v.second == 42; });
 * // count = 1
 * @endcode
 */
template< typename T >
std::size_t DATable< T >::erase_if(PredFun pred)
{
    std::size_t r = erase_if__(pred);
    recalcShape();
    return r;
}

/**
 * @brief 移除指定列
 * @tparam T 存储的值类型
 * @param col 要移除的列索引
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * table.set(0, 1, 43);
 * table.set(0, 2, 44);
 * table.dropColumn(1); // 移除第1列
 * // 现在第2列变为第1列
 * @endcode
 */
template< typename T >
void DATable< T >::dropColumn(IndexType col)
{
    // 分两步，第一步删除，第二部移动
    // 先把列号等于col的移除
    std::vector< value_type > temp;
    std::ignore = erase_if__([ col, &temp ](const value_type& v) -> bool {
        if (v.first.second == col) {
            return true;
        } else if (v.first.second > col) {
            // 大于这个列的也要删除，但要把值缓存起来，并进行左移动
            temp.emplace_back(v);
            return true;
        }
        return false;
    });
    // 再把列号大于col的全部减去1
    std::for_each(temp.begin(), temp.end(), [ this ](value_type& v) {
        IndexPair k = v.first;
        --k.second;
        mData[ k ] = v.second;
    });
    recalcShape();
}

/**
 * @brief 转换表格数据类型
 * @tparam T 存储的值类型
 * @tparam OtherType 目标值类型
 * @param trFun 转换函数
 * @return 转换后的表格
 *
 * @code{.cpp}
 * DA::DATable<double> doubleTable;
 * doubleTable.set(0, 0, 42.5);
 * // 转换为int类型
 * DA::DATable<int> intTable = doubleTable.transfered<int>([](const double& v) { return static_cast<int>(v); });
 * // intTable[0, 0] = 42
 * @endcode
 */
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

/**
 * @brief 遍历指定列
 * @tparam T 存储的值类型
 * @param col 列索引
 * @param trFun 遍历函数，如果返回false则停止遍历
 *
 * @code{.cpp}
 * DA::DATable<int> table;
 * table.set(0, 0, 42);
 * table.set(1, 0, 43);
 * table.set(2, 0, 44);
 * // 遍历第0列
 * table.transferColumn(0, [](const int& v) {
 *     std::cout << v << " ";
 *     return true; // 继续遍历
 * });
 * // 输出: 42 43 44
 * @endcode
 */
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

/**
 * @brief 从 da_vector_table 赋值
 * @tparam T 存储的值类型
 * @tparam OtherType 源值类型
 * @param other 源表格
 * @return 当前表格的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> vectorTable(3, 4, 42);
 * DA::DATable<int> table;
 * table = vectorTable; // 从vectorTable赋值
 * @endcode
 */
template< typename T >
template< typename OtherType >
DATable< T >& DATable< T >::operator=(const da_vector_table< OtherType >& other)
{
    clear();
    auto rc = other.row_count();
    auto cc = other.column_count();
    for (auto r = 0; r < rc; ++r) {
        for (auto c = 0; c < cc; ++c) {
            set(r, c, other[ r ][ c ]);
        }
    }
    return *this;
}

/**
 * @brief 刷新形状信息
 * @tparam T 存储的值类型
 * @param k 索引对
 *
 * @code{.cpp}
 * // 内部使用，无需示例
 * @endcode
 */
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

/**
 * @brief 内部条件删除实现
 * @tparam T 存储的值类型
 * @param pred 谓词函数
 * @return 删除的元素数量
 *
 * @code{.cpp}
 * // 内部使用，无需示例
 * @endcode
 */
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

}  // end DA

#endif  // DATABLE_H
