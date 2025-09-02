#ifndef DA_ARRAY_TABLE_H
#define DA_ARRAY_TABLE_H

#include <array>
#include <utility>

namespace DA
{

/**
 * @brief 基于 std::array 的固定大小二维表格数据结构
 *
 * 此类提供了一个固定大小的二维表格数据结构，使用嵌套的 std::array 存储数据，
 * 适合存储固定大小的二维数据，提供高效的随机访问性能。
 *
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 */
template< typename T, std::size_t RowN, std::size_t ColN >
class da_array_table
{
public:
    using value_type       = T;                                      // 元素类型
    using row_type         = std::array< T, ColN >;                  // 行类型
    using table_index_type = std::pair< std::size_t, std::size_t >;  // 索引类型

    // 迭代器类型定义
    using iterator               = typename std::array< row_type, RowN >::iterator;
    using const_iterator         = typename std::array< row_type, RowN >::const_iterator;
    using reverse_iterator       = typename std::array< row_type, RowN >::reverse_iterator;
    using const_reverse_iterator = typename std::array< row_type, RowN >::const_reverse_iterator;
    using size_type              = typename std::array< row_type, RowN >::size_type;

public:
    // 构造函数
    da_array_table();  // 默认构造函数

    // 元素访问
    T& operator()(std::size_t row, std::size_t col);              // 函数调用运算符访问元素
    const T& operator()(std::size_t row, std::size_t col) const;  // 函数调用运算符访问元素（常量版本）
    T& at(std::size_t row, std::size_t col);                      // 带边界检查的元素访问
    const T& at(std::size_t row, std::size_t col) const;          // 带边界检查的元素访问（常量版本）

    // 迭代器访问
    iterator begin() noexcept;                        // 返回指向起始的迭代器
    const_iterator begin() const noexcept;            // 返回指向起始的常量迭代器
    const_iterator cbegin() const noexcept;           // 返回指向起始的常量迭代器
    iterator end() noexcept;                          // 返回指向末尾的迭代器
    const_iterator end() const noexcept;              // 返回指向末尾的常量迭代器
    const_iterator cend() const noexcept;             // 返回指向末尾的常量迭代器
    reverse_iterator rbegin() noexcept;               // 返回指向起始的反向迭代器
    const_reverse_iterator rbegin() const noexcept;   // 返回指向起始的常量反向迭代器
    const_reverse_iterator crbegin() const noexcept;  // 返回指向起始的常量反向迭代器
    reverse_iterator rend() noexcept;                 // 返回指向末尾的反向迭代器
    const_reverse_iterator rend() const noexcept;     // 返回指向末尾的常量反向迭代器
    const_reverse_iterator crend() const noexcept;    // 返回指向末尾的常量反向迭代器

    // 容量操作
    bool empty() const noexcept;          // 检查表格是否为空
    size_type size() const noexcept;      // 返回元素数量
    size_type max_size() const noexcept;  // 返回最大可能元素数量

    // 表格操作
    table_index_type shape() const;    // 获取表格形状
    std::size_t row_count() const;     // 获取行数
    std::size_t column_count() const;  // 获取列数

    // 其他操作
    void fill(const T& value);                  // 填充表格
    void swap(da_array_table& other) noexcept;  // 交换两个表格的内容

private:
    std::array< row_type, RowN > data_;  // 数据存储
};

// ============================================================================
// 实现部分
// ============================================================================

/**
 * @brief 默认构造函数
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table; // 创建3行4列的表格
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
da_array_table< T, RowN, ColN >::da_array_table() : data_()
{
}

/**
 * @brief 函数调用运算符访问元素
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param row 行索引
 * @param col 列索引
 * @return 元素的引用
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * table(0, 0) = 42; // 设置元素值
 * int value = table(0, 0); // 获取元素值
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
T& da_array_table< T, RowN, ColN >::operator()(std::size_t row, std::size_t col)
{
    return data_[ row ][ col ];
}

/**
 * @brief 函数调用运算符访问元素（常量版本）
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param row 行索引
 * @param col 列索引
 * @return 元素的常量引用
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * int value = table(0, 0); // 获取元素值
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
const T& da_array_table< T, RowN, ColN >::operator()(std::size_t row, std::size_t col) const
{
    return data_[ row ][ col ];
}

/**
 * @brief 带边界检查的元素访问
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param row 行索引
 * @param col 列索引
 * @return 元素的引用
 * @throw std::out_of_range 如果索引超出范围
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * try {
 *     T& value = table.at(0, 0); // 正常访问
 *     T& invalid = table.at(5, 5); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
T& da_array_table< T, RowN, ColN >::at(std::size_t row, std::size_t col)
{
    if (row >= RowN || col >= ColN) {
        throw std::out_of_range("Index out of range");
    }
    return data_[ row ][ col ];
}

/**
 * @brief 带边界检查的元素访问（常量版本）
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param row 行索引
 * @param col 列索引
 * @return 元素的常量引用
 * @throw std::out_of_range 如果索引超出范围
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * try {
 *     const T& value = table.at(0, 0); // 正常访问
 *     const T& invalid = table.at(5, 5); // 抛出std::out_of_range
 * } catch (const std::out_of_range& e) {
 *     // 处理异常
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
const T& da_array_table< T, RowN, ColN >::at(std::size_t row, std::size_t col) const
{
    if (row >= RowN || col >= ColN) {
        throw std::out_of_range("Index out of range");
    }
    return data_[ row ][ col ];
}

/**
 * @brief 返回指向起始的迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的迭代器
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     for (auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::iterator da_array_table< T, RowN, ColN >::begin() noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向起始的常量迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     for (const auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::begin() const noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向起始的常量迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     for (const auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::cbegin() const noexcept
{
    return data_.cbegin();
}

/**
 * @brief 返回指向末尾的迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的迭代器
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::iterator da_array_table< T, RowN, ColN >::end() noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向末尾的常量迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::end() const noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向末尾的常量迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::cend() const noexcept
{
    return data_.cend();
}

/**
 * @brief 返回指向起始的反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的反向迭代器
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     for (auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::reverse_iterator da_array_table< T, RowN, ColN >::rbegin() noexcept
{
    return data_.rbegin();
}

/**
 * @brief 返回指向起始的常量反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     for (const auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::rbegin() const noexcept
{
    return data_.rbegin();
}

/**
 * @brief 返回指向起始的常量反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格起始的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.crbegin(); it != table.crend(); ++it) {
 *     for (const auto& element : *it) {
 *         // 处理元素
 *     }
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::crbegin() const noexcept
{
    return data_.crbegin();
}

/**
 * @brief 返回指向末尾的反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的反向迭代器
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::reverse_iterator da_array_table< T, RowN, ColN >::rend() noexcept
{
    return data_.rend();
}

/**
 * @brief 返回指向末尾的常量反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::rend() const noexcept
{
    return data_.rend();
}

/**
 * @brief 返回指向末尾的常量反向迭代器
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 指向表格末尾的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_array_table<int, 3, 4> table;
 * for (auto it = table.crbegin(); it != table.crend(); ++it) {
 *     // 处理行
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::crend() const noexcept
{
    return data_.crend();
}

/**
 * @brief 检查表格是否为空
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 如果表格为空返回true，否则返回false
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * if (table.empty()) {
 *     std::cout << "表格为空" << std::endl;
 * } else {
 *     std::cout << "表格不为空" << std::endl;
 * }
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
bool da_array_table< T, RowN, ColN >::empty() const noexcept
{
    return data_.empty();
}

/**
 * @brief 返回元素数量
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 表格中的元素数量
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * std::cout << "表格有 " << table.size() << " 个元素" << std::endl;
 * // 输出: 表格有 12 个元素
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::size_type da_array_table< T, RowN, ColN >::size() const noexcept
{
    return data_.size() * ColN;
}

/**
 * @brief 返回最大可能元素数量
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 表格可容纳的最大元素数量
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * std::cout << "表格最大可容纳 " << table.max_size() << " 个元素" << std::endl;
 * // 输出: 表格最大可容纳 12 个元素
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::size_type da_array_table< T, RowN, ColN >::max_size() const noexcept
{
    return data_.max_size() * ColN;
}

/**
 * @brief 获取表格形状
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 包含行数和列数的pair
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * auto shape = table.shape(); // 返回(3, 4)
 * std::cout << "表格形状: " << shape.first << " x " << shape.second << std::endl;
 * // 输出: 表格形状: 3 x 4
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::table_index_type da_array_table< T, RowN, ColN >::shape() const
{
    return { RowN, ColN };
}

/**
 * @brief 获取行数
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 表格的行数
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * std::cout << "表格有 " << table.row_count() << " 行" << std::endl;
 * // 输出: 表格有 3 行
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
std::size_t da_array_table< T, RowN, ColN >::row_count() const
{
    return RowN;
}

/**
 * @brief 获取列数
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @return 表格的列数
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * std::cout << "表格有 " << table.column_count() << " 列" << std::endl;
 * // 输出: 表格有 4 列
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
std::size_t da_array_table< T, RowN, ColN >::column_count() const
{
    return ColN;
}

/**
 * @brief 填充表格
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param value 要填充的值
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table;
 * table.fill(42); // 将所有元素设置为42
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
void da_array_table< T, RowN, ColN >::fill(const T& value)
{
    for (auto& row : data_) {
        row.fill(value);
    }
}

/**
 * @brief 交换两个表格的内容
 * @tparam T 存储的值类型
 * @tparam RowN 行数
 * @tparam ColN 列数
 * @param other 要交换的另一个表格
 *
 * @code{.cpp}
 * DA::da_array_table<int, 3, 4> table1;
 * DA::da_array_table<int, 3, 4> table2;
 * table1.swap(table2); // 交换两个表格的内容
 * @endcode
 */
template< typename T, std::size_t RowN, std::size_t ColN >
void da_array_table< T, RowN, ColN >::swap(da_array_table& other) noexcept
{
    data_.swap(other.data_);
}

}  // namespace DA

#endif  // DA_ARRAY_TABLE_H
