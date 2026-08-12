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
    std::array< row_type, RowN > mData;  // 数据存储
};

// ============================================================================
// 实现部分
// ============================================================================

// 默认构造函数
template< typename T, std::size_t RowN, std::size_t ColN >
da_array_table< T, RowN, ColN >::da_array_table() : mData()
{
}

// 函数调用运算符访问元素
template< typename T, std::size_t RowN, std::size_t ColN >
T& da_array_table< T, RowN, ColN >::operator()(std::size_t row, std::size_t col)
{
    return mData[ row ][ col ];
}

// 函数调用运算符访问元素（常量版本）
template< typename T, std::size_t RowN, std::size_t ColN >
const T& da_array_table< T, RowN, ColN >::operator()(std::size_t row, std::size_t col) const
{
    return mData[ row ][ col ];
}

// 带边界检查的元素访问
template< typename T, std::size_t RowN, std::size_t ColN >
T& da_array_table< T, RowN, ColN >::at(std::size_t row, std::size_t col)
{
    if (row >= RowN || col >= ColN) {
        throw std::out_of_range("Index out of range");
    }
    return mData[ row ][ col ];
}

// 带边界检查的元素访问（常量版本）
template< typename T, std::size_t RowN, std::size_t ColN >
const T& da_array_table< T, RowN, ColN >::at(std::size_t row, std::size_t col) const
{
    if (row >= RowN || col >= ColN) {
        throw std::out_of_range("Index out of range");
    }
    return mData[ row ][ col ];
}

// 返回指向起始的迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::iterator da_array_table< T, RowN, ColN >::begin() noexcept
{
    return mData.begin();
}

// 返回指向起始的常量迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::begin() const noexcept
{
    return mData.begin();
}

// 返回指向起始的常量迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::cbegin() const noexcept
{
    return mData.cbegin();
}

// 返回指向末尾的迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::iterator da_array_table< T, RowN, ColN >::end() noexcept
{
    return mData.end();
}

// 返回指向末尾的常量迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::end() const noexcept
{
    return mData.end();
}

// 返回指向末尾的常量迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_iterator da_array_table< T, RowN, ColN >::cend() const noexcept
{
    return mData.cend();
}

// 返回指向起始的反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::reverse_iterator da_array_table< T, RowN, ColN >::rbegin() noexcept
{
    return mData.rbegin();
}

// 返回指向起始的常量反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::rbegin() const noexcept
{
    return mData.rbegin();
}

// 返回指向起始的常量反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::crbegin() const noexcept
{
    return mData.crbegin();
}

// 返回指向末尾的反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::reverse_iterator da_array_table< T, RowN, ColN >::rend() noexcept
{
    return mData.rend();
}

// 返回指向末尾的常量反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::rend() const noexcept
{
    return mData.rend();
}

// 返回指向末尾的常量反向迭代器
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::const_reverse_iterator da_array_table< T, RowN, ColN >::crend() const noexcept
{
    return mData.crend();
}

// 检查表格是否为空
template< typename T, std::size_t RowN, std::size_t ColN >
bool da_array_table< T, RowN, ColN >::empty() const noexcept
{
    return mData.empty();
}

// 返回元素数量
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::size_type da_array_table< T, RowN, ColN >::size() const noexcept
{
    return mData.size() * ColN;
}

// 返回最大可能元素数量
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::size_type da_array_table< T, RowN, ColN >::max_size() const noexcept
{
    return mData.max_size() * ColN;
}

// 获取表格形状
template< typename T, std::size_t RowN, std::size_t ColN >
typename da_array_table< T, RowN, ColN >::table_index_type da_array_table< T, RowN, ColN >::shape() const
{
    return { RowN, ColN };
}

// 获取行数
template< typename T, std::size_t RowN, std::size_t ColN >
std::size_t da_array_table< T, RowN, ColN >::row_count() const
{
    return RowN;
}

// 获取列数
template< typename T, std::size_t RowN, std::size_t ColN >
std::size_t da_array_table< T, RowN, ColN >::column_count() const
{
    return ColN;
}

// 填充表格
template< typename T, std::size_t RowN, std::size_t ColN >
void da_array_table< T, RowN, ColN >::fill(const T& value)
{
    for (auto& row : mData) {
        row.fill(value);
    }
}

// 交换两个表格的内容
template< typename T, std::size_t RowN, std::size_t ColN >
void da_array_table< T, RowN, ColN >::swap(da_array_table& other) noexcept
{
    mData.swap(other.mData);
}

}  // namespace DA

#endif  // DA_ARRAY_TABLE_H
