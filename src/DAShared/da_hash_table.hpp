#ifndef DA_HASH_TABLE_H
#define DA_HASH_TABLE_H

#include <unordered_map>
#include <utility>
#include <functional>
#include <initializer_list>
#include <vector>
#include <algorithm>

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
    T& operator()(row_index_type r, col_index_type c);             // 函数调用运算符访问元素
    const T operator()(row_index_type r, col_index_type c) const;  // 函数调用运算符访问元素（常量版本）
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
    std::unordered_map< key_type, T, hasher > data_;
};

/**
 * @brief Hash function for std::pair to be used in unordered containers
 * @tparam T1 第一个元素的类型
 * @tparam T2 第二个元素的类型
 * @param p 要哈希的pair
 * @return 哈希值
 *
 * @code{.cpp}
 * DA::pair_hash hasher;
 * std::size_t h = hasher(std::make_pair(1, 2)); // 计算哈希值
 * @endcode
 */
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

/**
 * @brief 初始化列表构造函数
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param init 初始化列表
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 1},
 *     {{1, 1}, 2},
 *     {{2, 2}, 3}
 * };
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
da_hash_table< T, row_index_type, col_index_type, hasher >::da_hash_table(std::initializer_list< value_type > init)
    : data_(init)
{
}

/**
 * @brief 带边界检查的元素访问
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table.insert({{0, 0}, 42});
 * try {
 *     int& value = table.at(0, 0); // 返回42
 *     int& invalid = table.at(1, 1); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(row_index_type r, col_index_type c)
{
    return data_.at({ r, c });
}

/**
 * @brief 带边界检查的常量元素访问
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的常量引用
 * @throw std::out_of_range 如果元素不存在
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * try {
 *     const int& value = table.at(0, 0); // 返回42
 *     const int& invalid = table.at(1, 1); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T& da_hash_table< T, row_index_type, col_index_type, hasher >::at(row_index_type r, col_index_type c) const
{
    return data_.at({ r, c });
}

/**
 * @brief 函数调用运算符访问元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的引用
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table(0, 0) = 42; // 插入或修改元素
 * int value = table(0, 0); // 获取元素值
 * int missing = table(1, 1); // 返回默认构造的值（0）
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(row_index_type r, col_index_type c)
{
    return data_[ { r, c } ];
}

/**
 * @brief 函数调用运算符访问元素（常量版本）
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的值或默认值
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * int value = table(0, 0); // 返回42
 * int missing = table(1, 1); // 返回默认构造的值（0）
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
const T da_hash_table< T, row_index_type, col_index_type, hasher >::operator()(row_index_type r, col_index_type c) const
{
    auto it = data_.find({ r, c });
    return it != data_.end() ? it->second : T();
}

template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T& da_hash_table< T, row_index_type, col_index_type, hasher >::operator[](
    da_hash_table< T, row_index_type, col_index_type, hasher >::key_type k)
{
    return data_[ k ];
}

/**
 * @brief 安全的元素访问，返回默认值如果不存在
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素的值或默认构造的值
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * int value = table.value(0, 0); // 返回42
 * int missing = table.value(1, 1); // 返回0（默认构造的int）
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T da_hash_table< T, row_index_type, col_index_type, hasher >::value(row_index_type r, col_index_type c) const
{
    auto it = data_.find({ r, c });
    return it != data_.end() ? it->second : T();
}

/**
 * @brief 安全的元素访问，返回指定默认值如果不存在
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @param defaultValue 指定的默认值
 * @return 元素的值或指定的默认值
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * int value = table.value(0, 0, -1); // 返回42
 * int missing = table.value(1, 1, -1); // 返回-1
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
T da_hash_table< T, row_index_type, col_index_type, hasher >::value(row_index_type r, col_index_type c, const T& defaultValue) const
{
    auto it = data_.find({ r, c });
    return it != data_.end() ? it->second : defaultValue;
}

/**
 * @brief 检查表格是否为空
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 如果表格为空返回true，否则返回false
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * if (table.empty()) {
 *     std::cout << "表格为空" << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
bool da_hash_table< T, row_index_type, col_index_type, hasher >::empty() const noexcept
{
    return data_.empty();
}

/**
 * @brief 返回元素数量
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 表格中的元素数量
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * std::cout << "表格有 " << table.size() << " 个元素" << std::endl;
 * // 输出: 表格有 2 个元素
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::size()
    const noexcept
{
    return data_.size();
}

/**
 * @brief 返回最大可能元素数量
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 表格可容纳的最大元素数量
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * std::cout << "表格最大可容纳 " << table.max_size() << " 个元素" << std::endl;
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::max_size()
    const noexcept
{
    return data_.max_size();
}

/**
 * @brief 清空表格
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * table.clear(); // 清空表格
 * // 现在表格为空
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::clear() noexcept
{
    data_.clear();
}

/**
 * @brief 插入元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param value 要插入的元素
 * @return 包含迭代器和布尔值的pair，指示插入是否成功
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * auto result = table.insert({{0, 0}, 42});
 * if (result.second) {
 *     std::cout << "插入成功" << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::insert(const value_type& value)
{
    return data_.insert(value);
}

/**
 * @brief 插入元素（通用引用）
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @tparam P 元素类型
 * @param value 要插入的元素
 * @return 包含迭代器和布尔值的pair，指示插入是否成功
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * auto result = table.insert(std::make_pair(std::make_pair(0, 0), 42));
 * if (result.second) {
 *     std::cout << "插入成功" << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class P >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::insert(P&& value)
{
    return data_.insert(std::forward< P >(value));
}

/**
 * @brief 插入元素范围
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @tparam InputIt 输入迭代器类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * std::vector<std::pair<std::pair<int, int>, int>> values = {
 *     {{0, 0}, 42},
 *     {{1, 1}, 43}
 * };
 * table.insert(values.begin(), values.end());
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class InputIt >
void da_hash_table< T, row_index_type, col_index_type, hasher >::insert(InputIt first, InputIt last)
{
    data_.insert(first, last);
}

/**
 * @brief 插入初始化列表
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param ilist 初始化列表
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table.insert({
 *     {{0, 0}, 42},
 *     {{1, 1}, 43}
 * });
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::insert(std::initializer_list< value_type > ilist)
{
    data_.insert(ilist);
}

/**
 * @brief 原位构造元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @tparam Args 参数类型
 * @param args 构造参数
 * @return 包含迭代器和布尔值的pair，指示插入是否成功
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * auto result = table.emplace(std::make_pair(0, 0), 42);
 * if (result.second) {
 *     std::cout << "插入成功" << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
template< class... Args >
std::pair< typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator, bool > da_hash_table<
    T,
    row_index_type,
    col_index_type,
    hasher >::emplace(Args&&... args)
{
    return data_.emplace(std::forward< Args >(args)...);
}

/**
 * @brief 删除指定位置元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param pos 要删除的元素位置
 * @return 指向被删除元素之后位置的迭代器
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * auto it = table.find(0, 0);
 * if (it != table.end()) {
 *     table.erase(it); // 删除元素
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    const_iterator pos)
{
    return data_.erase(pos);
}

/**
 * @brief 删除元素范围
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @return 指向被删除元素之后位置的迭代器
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{1, 1}, 43},
 *     {{2, 2}, 44}
 * };
 * auto first = table.find(0, 0);
 * auto last = table.find(2, 2);
 * if (first != table.end() && last != table.end()) {
 *     table.erase(first, last); // 删除范围[first, last)内的元素
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    const_iterator first,
    const_iterator last)
{
    return data_.erase(first, last);
}

/**
 * @brief 删除指定行列元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 被删除的元素数量（0或1）
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * size_t count = table.erase(0, 0); // 返回1
 * count = table.erase(1, 1); // 返回0
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::erase(
    row_index_type r,
    col_index_type c)
{
    return data_.erase({ r, c });
}

/**
 * @brief 查找元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 指向元素的迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * auto it = table.find(0, 0);
 * if (it != table.end()) {
 *     std::cout << "找到元素: " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    row_index_type r,
    col_index_type c)
{
    return data_.find({ r, c });
}

/**
 * @brief 查找元素（常量版本）
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 指向元素的常量迭代器，如果未找到则返回end()
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * auto it = table.find(0, 0);
 * if (it != table.end()) {
 *     std::cout << "找到元素: " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T, row_index_type, col_index_type, hasher >::find(
    row_index_type r,
    col_index_type c) const
{
    return data_.find({ r, c });
}

/**
 * @brief 统计元素出现次数
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @param c 列索引
 * @return 元素出现次数（0或1）
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}};
 * size_t count = table.count(0, 0); // 返回1
 * count = table.count(1, 1); // 返回0
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::count(
    row_index_type r,
    col_index_type c) const
{
    return data_.count({ r, c });
}

/**
 * @brief 返回指向起始的迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格起始的迭代器
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << "(" << it->first.first << ", " << it->first.second
 *               << ") = " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::begin() noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向起始的常量迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格起始的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << "(" << it->first.first << ", " << it->first.second
 *               << ") = " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::begin() const noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向起始的常量迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格起始的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     std::cout << "(" << it->first.first << ", " << it->first.second
 *               << ") = " << it->second << std::endl;
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::cbegin() const noexcept
{
    return data_.cbegin();
}

/**
 * @brief 返回指向末尾的迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格末尾的迭代器
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::iterator da_hash_table< T, row_index_type, col_index_type, hasher >::end() noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向末尾的常量迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格末尾的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::end() const noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向末尾的常量迭代器
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 指向表格末尾的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_hash_table<int> table = {{{0, 0}, 42}, {{1, 1}, 43}};
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     // 处理元素
 * }
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::const_iterator da_hash_table< T,
                                                                                                   row_index_type,
                                                                                                   col_index_type,
                                                                                                   hasher >::cend() const noexcept
{
    return data_.cend();
}

/**
 * @brief 返回当前负载因子
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 当前负载因子
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * float lf = table.load_factor();
 * std::cout << "当前负载因子: " << lf << std::endl;
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
float da_hash_table< T, row_index_type, col_index_type, hasher >::load_factor() const noexcept
{
    return data_.load_factor();
}

/**
 * @brief 返回最大负载因子
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 最大负载因子
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * float mlf = table.max_load_factor();
 * std::cout << "最大负载因子: " << mlf << std::endl;
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
float da_hash_table< T, row_index_type, col_index_type, hasher >::max_load_factor() const noexcept
{
    return data_.max_load_factor();
}

/**
 * @brief 设置最大负载因子
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param ml 新的最大负载因子
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table.max_load_factor(0.75f); // 设置最大负载因子为0.75
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::max_load_factor(float ml)
{
    data_.max_load_factor(ml);
}

/**
 * @brief 设置桶数并重新哈希
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param count 新的桶数
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table.rehash(100); // 设置桶数为100并重新哈希
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::rehash(size_type count)
{
    data_.rehash(count);
}

/**
 * @brief 预留空间
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param count 要预留的元素数量
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * table.reserve(100); // 预留100个元素的空间
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::reserve(size_type count)
{
    data_.reserve(count);
}

/**
 * @brief 返回桶数
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 当前桶数
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * size_t bc = table.bucket_count();
 * std::cout << "当前桶数: " << bc << std::endl;
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::
    bucket_count() const noexcept
{
    return data_.bucket_count();
}

/**
 * @brief 返回最大桶数
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 最大桶数
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table;
 * size_t mbc = table.max_bucket_count();
 * std::cout << "最大桶数: " << mbc << std::endl;
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::size_type da_hash_table< T, row_index_type, col_index_type, hasher >::
    max_bucket_count() const noexcept
{
    return data_.max_bucket_count();
}

/**
 * @brief 计算表格的形状（最大行索引和最大列索引加1）
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 包含最大行索引+1和最大列索引+1的pair
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{2, 3}, 43},
 *     {{1, 5}, 44}
 * };
 * auto shape = table.shape(); // 返回(3, 6)
 * std::cout << "表格形状: " << shape.first << " x " << shape.second << std::endl;
 * // 输出: 表格形状: 3 x 6
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
typename da_hash_table< T, row_index_type, col_index_type, hasher >::table_index_type da_hash_table< T,
                                                                                                     row_index_type,
                                                                                                     col_index_type,
                                                                                                     hasher >::shape() const
{
    if (data_.empty()) {
        return { row_index_type(), col_index_type() };
    }

    auto it                = data_.cbegin();
    row_index_type max_row = it->first.first;
    col_index_type max_col = it->first.second;

    ++it;
    for (; it != data_.cend(); ++it) {
        if (it->first.first > max_row) {
            max_row = it->first.first;
        }
        if (it->first.second > max_col) {
            max_col = it->first.second;
        }
    }

    // 返回最大索引+1，表示形状
    return { max_row + 1, max_col + 1 };
}

/**
 * @brief 获取实际使用的行索引列表
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 包含所有实际使用的行索引的向量
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{2, 1}, 43},
 *     {{0, 2}, 44}
 * };
 * auto rows = table.row_indices(); // 返回[0, 2]
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< row_index_type > da_hash_table< T, row_index_type, col_index_type, hasher >::row_indices() const
{
    std::vector< row_index_type > rows;
    for (const auto& item : data_) {
        rows.push_back(item.first.first);
    }

    // 去重并排序
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    return rows;
}

/**
 * @brief 获取实际使用的列索引列表
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @return 包含所有实际使用的列索引的向量
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{2, 1}, 43},
 *     {{0, 2}, 44}
 * };
 * auto cols = table.column_indices(); // 返回[0, 1, 2]
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< col_index_type > da_hash_table< T, row_index_type, col_index_type, hasher >::column_indices() const
{
    std::vector< col_index_type > cols;
    for (const auto& item : data_) {
        cols.push_back(item.first.second);
    }

    // 去重并排序
    std::sort(cols.begin(), cols.end());
    cols.erase(std::unique(cols.begin(), cols.end()), cols.end());

    return cols;
}

/**
 * @brief 获取指定行的所有元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param r 行索引
 * @return 包含指定行所有元素的向量，每个元素是(列索引, 值)的pair
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{0, 2}, 43},
 *     {{1, 1}, 44}
 * };
 * auto row0 = table.row(0); // 返回[(0, 42), (2, 43)]
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< std::pair< col_index_type, T > > da_hash_table< T, row_index_type, col_index_type, hasher >::row(row_index_type r) const
{
    std::vector< std::pair< col_index_type, T > > result;

    for (const auto& item : data_) {
        if (item.first.first == r) {
            result.emplace_back(item.first.second, item.second);
        }
    }

    // 按列索引排序
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    return result;
}

/**
 * @brief 获取指定列的所有元素
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param c 列索引
 * @return 包含指定列所有元素的向量，每个元素是(行索引, 值)的pair
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table = {
 *     {{0, 0}, 42},
 *     {{2, 0}, 43},
 *     {{1, 1}, 44}
 * };
 * auto col0 = table.column(0); // 返回[(0, 42), (2, 43)]
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
std::vector< std::pair< row_index_type, T > > da_hash_table< T, row_index_type, col_index_type, hasher >::column(
    col_index_type c) const
{
    std::vector< std::pair< row_index_type, T > > result;

    for (const auto& item : data_) {
        if (item.first.second == c) {
            result.emplace_back(item.first.first, item.second);
        }
    }

    // 按行索引排序
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    return result;
}

/**
 * @brief 交换两个表格的内容
 * @tparam T 存储的值类型
 * @tparam row_index_type 行索引类型
 * @tparam col_index_type 列索引类型
 * @tparam hasher 哈希函数类型
 * @param other 要交换的另一个表格
 *
 * @code{.cpp}
 * DA::da_hash_table<int> table1 = {{{0, 0}, 42}};
 * DA::da_hash_table<int> table2 = {{{1, 1}, 43}};
 * table1.swap(table2);
 * // 现在table1包含{{1, 1}, 43}，table2包含{{0, 0}, 42}
 * @endcode
 */
template< typename T, typename row_index_type, typename col_index_type, typename hasher >
void da_hash_table< T, row_index_type, col_index_type, hasher >::swap(da_hash_table& other) noexcept
{
    data_.swap(other.data_);
}

}  // namespace DA

#endif  // DA_HASH_TABLE_H
