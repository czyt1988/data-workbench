#ifndef DA_VECTOR_TABLE_H
#define DA_VECTOR_TABLE_H

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <initializer_list>
#include <type_traits>

namespace DA
{
/**
 * @brief 二维表格数据结构，使用一维数组存储以提高缓存友好性
 * @tparam T 表格中存储的元素类型
 *
 * 此类提供了一个二维表格数据结构，内部使用一维数组存储以提高内存局部性和缓存友好性。
 * 适合存放规则的结构化数据，并提供高效的随机访问性能。
 */
template< typename T >
class da_vector_table
{
public:
    using value_type       = T;                                      ///< 元素类型
    using table_index_type = std::pair< std::size_t, std::size_t >;  ///< 表格形状类型(行数, 列数)

    // 迭代器类型定义
    using iterator               = typename std::vector< T >::iterator;
    using const_iterator         = typename std::vector< T >::const_iterator;
    using reverse_iterator       = typename std::vector< T >::reverse_iterator;
    using const_reverse_iterator = typename std::vector< T >::const_reverse_iterator;

public:
    // 构造函数
    da_vector_table();
    explicit da_vector_table(std::size_t rows, std::size_t cols = 0, const T& value = T());
    da_vector_table(std::initializer_list< std::initializer_list< T > > init);

    // 拷贝和移动构造函数
    da_vector_table(const da_vector_table& other);
    da_vector_table(da_vector_table&& other) noexcept;

    // 赋值运算符
    da_vector_table& operator=(const da_vector_table& other);
    da_vector_table& operator=(da_vector_table&& other) noexcept;

    // 容量操作
    bool empty() const noexcept;
    std::size_t size() const noexcept;
    std::size_t max_size() const noexcept;
    void reserve(std::size_t new_cap);
    std::size_t capacity() const noexcept;
    void shrink_to_fit();

    // 迭代器访问
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;
    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

    // 元素访问
    T& operator()(std::size_t row, std::size_t col);
    const T& operator()(std::size_t row, std::size_t col) const;
    T& at(std::size_t row, std::size_t col);
    const T& at(std::size_t row, std::size_t col) const;
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;
    T* data() noexcept;
    const T* data() const noexcept;

    // 修改器
    void assign(std::size_t rows, std::size_t cols, const T& value);
    template< typename InputIt >
    void assign(InputIt first, InputIt last);
    void push_back(const T& value);
    void push_back(T&& value);
    template< typename... Args >
    void emplace_back(Args&&... args);
    void pop_back();
    void clear() noexcept;
    void swap(da_vector_table& other) noexcept;

    // 行操作
    void append_row(const std::vector< T >& row);
    void append_row(std::vector< T >&& row);
    template< typename InputIt >
    void append_row(InputIt first, InputIt last);
    void append_row(std::initializer_list< T > il);
    void insert_row(std::size_t pos, const std::vector< T >& row);
    void insert_row(std::size_t pos, std::vector< T >&& row);
    void erase_row(std::size_t pos);

    // 列操作
    void append_column(const std::vector< T >& col);
    void append_column(std::vector< T >&& col);
    template< typename InputIt >
    void append_column(InputIt first, InputIt last);
    void append_column(std::initializer_list< T > il);
    void insert_column(std::size_t pos, const std::vector< T >& col);
    void insert_column(std::size_t pos, std::vector< T >&& col);
    void erase_column(std::size_t pos);

    // 形状操作
    table_index_type shape() const noexcept;
    std::size_t row_count() const noexcept;
    std::size_t column_count() const noexcept;
    void resize(std::size_t rows, std::size_t cols, const T& value = T());
    void resize(table_index_type sh, const T& value = T());
    void reshape(std::size_t rows, std::size_t cols);

    // 辅助功能
    bool is_rectangular() const noexcept
    {
        return true;
    }  // 一维存储总是矩形的
    std::vector< T > get_row(std::size_t row) const;
    std::vector< T > get_column(std::size_t col) const;
    void set_row(std::size_t row, const std::vector< T >& values);
    void set_column(std::size_t col, const std::vector< T >& values);

private:
    std::vector< T > data_;  // 一维数据存储
    std::size_t rows_;       // 行数
    std::size_t cols_;       // 列数

    // 辅助函数
    std::size_t index(std::size_t row, std::size_t col) const noexcept;
    void check_bounds(std::size_t row, std::size_t col) const;
    void check_row_bounds(std::size_t row) const;
    void check_column_bounds(std::size_t col) const;
};

// ============================================================================
// 实现部分
// ============================================================================

/**
 * @brief 默认构造函数
 * @tparam T 表格元素类型
 *
 * @code{.cpp}
 * // 创建一个空的表格
 * DA::da_vector_table<int> table;
 * @endcode
 */
template< typename T >
da_vector_table< T >::da_vector_table() : data_(), rows_(0), cols_(0)
{
}

/**
 * @brief 构造函数，创建指定大小的表格
 * @tparam T 表格元素类型
 * @param rows 行数
 * @param cols 列数
 * @param value 初始值
 *
 * @code{.cpp}
 * // 创建一个3行4列的表格，所有元素初始化为0
 * DA::da_vector_table<int> table(3, 4, 0);
 *
 * // 创建一个2行3列的表格，所有元素使用默认构造
 * DA::da_vector_table<std::string> table(2, 3);
 * @endcode
 */
template< typename T >
da_vector_table< T >::da_vector_table(std::size_t rows, std::size_t cols, const T& value)
    : data_(rows * cols, value), rows_(rows), cols_(cols)
{
}

/**
 * @brief 使用初始化列表构造表格
 * @tparam T 表格元素类型
 * @param init 初始化列表
 *
 * @code{.cpp}
 * // 使用初始化列表创建表格
 * DA::da_vector_table<int> table = {
 *     {1, 2, 3},
 *     {4, 5, 6},
 *     {7, 8, 9}
 * };
 * @endcode
 */
template< typename T >
da_vector_table< T >::da_vector_table(std::initializer_list< std::initializer_list< T > > init)
    : rows_(init.size()), cols_(0)
{
    // 确定最大列数
    for (const auto& row : init) {
        if (row.size() > cols_) {
            cols_ = row.size();
        }
    }

    // 分配内存
    data_.resize(rows_ * cols_);

    // 填充数据
    std::size_t row_idx = 0;
    for (const auto& row : init) {
        std::size_t col_idx = 0;
        for (const auto& value : row) {
            data_[ index(row_idx, col_idx) ] = value;
            ++col_idx;
        }
        // 填充剩余部分（如果有）
        for (; col_idx < cols_; ++col_idx) {
            data_[ index(row_idx, col_idx) ] = T();
        }
        ++row_idx;
    }
}

/**
 * @brief 拷贝构造函数
 * @tparam T 表格元素类型
 * @param other 要拷贝的表格
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table1(2, 3, 5);
 * // 创建table1的副本
 * DA::da_vector_table<int> table2(table1);
 * @endcode
 */
template< typename T >
da_vector_table< T >::da_vector_table(const da_vector_table& other)
    : data_(other.data_), rows_(other.rows_), cols_(other.cols_)
{
}

/**
 * @brief 移动构造函数
 * @tparam T 表格元素类型
 * @param other 要移动的表格
 *
 * @code{.cpp}
 * DA::da_vector_table<int> create_table() {
 *     DA::da_vector_table<int> table(3, 4, 1);
 *     return table; // 使用移动语义
 * }
 *
 * auto table = create_table(); // 高效，不会拷贝数据
 * @endcode
 */
template< typename T >
da_vector_table< T >::da_vector_table(da_vector_table&& other) noexcept
    : data_(std::move(other.data_)), rows_(other.rows_), cols_(other.cols_)
{
    other.rows_ = 0;
    other.cols_ = 0;
}

/**
 * @brief 拷贝赋值运算符
 * @tparam T 表格元素类型
 * @param other 要拷贝的表格
 * @return 当前表格的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table1(2, 3, 5);
 * DA::da_vector_table<int> table2;
 * table2 = table1; // 拷贝赋值
 * @endcode
 */
template< typename T >
da_vector_table< T >& da_vector_table< T >::operator=(const da_vector_table& other)
{
    if (this != &other) {
        data_ = other.data_;
        rows_ = other.rows_;
        cols_ = other.cols_;
    }
    return *this;
}

/**
 * @brief 移动赋值运算符
 * @tparam T 表格元素类型
 * @param other 要移动的表格
 * @return 当前表格的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table1(2, 3, 5);
 * DA::da_vector_table<int> table2;
 * table2 = std::move(table1); // 移动赋值，table1变为空
 * @endcode
 */
template< typename T >
da_vector_table< T >& da_vector_table< T >::operator=(da_vector_table&& other) noexcept
{
    if (this != &other) {
        data_       = std::move(other.data_);
        rows_       = other.rows_;
        cols_       = other.cols_;
        other.rows_ = 0;
        other.cols_ = 0;
    }
    return *this;
}

/**
 * @brief 检查表格是否为空
 * @tparam T 表格元素类型
 * @return 如果表格为空返回true，否则返回false
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * if (table.empty()) {
 *     std::cout << "表格为空" << std::endl;
 * }
 * @endcode
 */
template< typename T >
bool da_vector_table< T >::empty() const noexcept
{
    return data_.empty();
}

/**
 * @brief 返回表格中的元素总数
 * @tparam T 表格元素类型
 * @return 表格中的元素总数
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(3, 4);
 * std::cout << "表格有 " << table.size() << " 个元素" << std::endl;
 * @endcode
 */
template< typename T >
std::size_t da_vector_table< T >::size() const noexcept
{
    return data_.size();
}

/**
 * @brief 返回表格可容纳的最大元素数
 * @tparam T 表格元素类型
 * @return 表格可容纳的最大元素数
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * std::cout << "表格最大可容纳 " << table.max_size() << " 个元素" << std::endl;
 * @endcode
 */
template< typename T >
std::size_t da_vector_table< T >::max_size() const noexcept
{
    return data_.max_size();
}

/**
 * @brief 预留存储空间
 * @tparam T 表格元素类型
 * @param new_cap 新的容量值
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * // 预留空间以提高后续添加元素的性能
 * table.reserve(100);
 * @endcode
 */
template< typename T >
void da_vector_table< T >::reserve(std::size_t new_cap)
{
    data_.reserve(new_cap);
}

/**
 * @brief 返回当前分配的存储容量
 * @tparam T 表格元素类型
 * @return 当前分配的存储容量
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * table.reserve(50);
 * std::cout << "表格容量: " << table.capacity() << std::endl;
 * @endcode
 */
template< typename T >
std::size_t da_vector_table< T >::capacity() const noexcept
{
    return data_.capacity();
}

/**
 * @brief 请求移除未使用的容量
 * @tparam T 表格元素类型
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * // 添加一些元素
 * table.resize(3, 4, 1);
 * // 释放多余内存
 * table.shrink_to_fit();
 * @endcode
 */
template< typename T >
void da_vector_table< T >::shrink_to_fit()
{
    data_.shrink_to_fit();
}

/**
 * @brief 返回指向表格第一个元素的迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素的迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用迭代器遍历所有元素
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::iterator da_vector_table< T >::begin() noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向表格第一个元素的常量迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 使用常量迭代器遍历所有元素
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::begin() const noexcept
{
    return data_.begin();
}

/**
 * @brief 返回指向表格第一个元素的常量迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 使用常量迭代器遍历所有元素
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::cbegin() const noexcept
{
    return data_.cbegin();
}

/**
 * @brief 返回指向表格尾后位置的迭代器
 * @tparam T 表格元素类型
 * @return 指向表格尾后位置的迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用迭代器遍历所有元素
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::iterator da_vector_table< T >::end() noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向表格尾后位置的常量迭代器
 * @tparam T 表格元素类型
 * @return 指向表格尾后位置的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 使用常量迭代器遍历所有元素
 * for (auto it = table.begin(); it != table.end(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::end() const noexcept
{
    return data_.end();
}

/**
 * @brief 返回指向表格尾后位置的常量迭代器
 * @tparam T 表格元素类型
 * @return 指向表格尾后位置的常量迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 使用常量迭代器遍历所有元素
 * for (auto it = table.cbegin(); it != table.cend(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::cend() const noexcept
{
    return data_.cend();
}

/**
 * @brief 返回指向表格最后一个元素的反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格最后一个元素的反向迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用反向迭代器逆序遍历所有元素
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::reverse_iterator da_vector_table< T >::rbegin() noexcept
{
    return data_.rbegin();
}

/**
 * @brief 返回指向表格最后一个元素的常量反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格最后一个元素的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用常量反向迭代器逆序遍历所有元素
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::rbegin() const noexcept
{
    return data_.rbegin();
}

/**
 * @brief 返回指向表格最后一个元素的常量反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格最后一个元素的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用常量反向迭代器逆序遍历所有元素
 * for (auto it = table.crbegin(); it != table.crend(); ++it) {
 *     std::cout << *it << " ";
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::crbegin() const noexcept
{
    return data_.crbegin();
}

/**
 * @brief 返回指向表格第一个元素前一个位置的反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素前一个位置的反向迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用反向迭代器逆序遍历所有元素
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::reverse_iterator da_vector_table< T >::rend() noexcept
{
    return data_.rend();
}

/**
 * @brief 返回指向表格第一个元素前一个位置的常量反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素前一个位置的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用常量反向迭代器逆序遍历所有元素
 * for (auto it = table.rbegin(); it != table.rend(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::rend() const noexcept
{
    return data_.rend();
}

/**
 * @brief 返回指向表格第一个元素前一个位置的常量反向迭代器
 * @tparam T 表格元素类型
 * @return 指向表格第一个元素前一个位置的常量反向迭代器
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 使用常量反向迭代器逆序遍历所有元素
 * for (auto it = table.crbegin(); it != table.crend(); ++it) {
 *     // 处理每个元素
 * }
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::crend() const noexcept
{
    return data_.crend();
}

/**
 * @brief 访问指定位置的元素（函数调用运算符）
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @return 指定位置元素的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 访问第一行第二列的元素
 * int& value = table(0, 1);
 * value = 10; // 修改为10
 * @endcode
 */
template< typename T >
T& da_vector_table< T >::operator()(std::size_t row, std::size_t col)
{
    return data_[ index(row, col) ];
}

/**
 * @brief 访问指定位置的元素（函数调用运算符，常量版本）
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @return 指定位置元素的常量引用
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 访问第一行第二列的元素
 * const int& value = table(0, 1);
 * std::cout << "值: " << value << std::endl;
 * @endcode
 */
template< typename T >
const T& da_vector_table< T >::operator()(std::size_t row, std::size_t col) const
{
    return data_[ index(row, col) ];
}

/**
 * @brief 访问指定位置的元素，带边界检查
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @return 指定位置元素的引用
 * @throw std::out_of_range 如果row或col超出范围
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * try {
 *     int& value = table.at(0, 1); // 获取第一行第二列的元素
 *     value = 10; // 修改为10
 *
 *     // 尝试访问不存在的元素
 *     int& invalid = table.at(5, 5); // 抛出异常
 * } catch (const std::out_of_range& e) {
 *     std::cerr << "错误: " << e.what() << std::endl;
 * }
 * @endcode
 */
template< typename T >
T& da_vector_table< T >::at(std::size_t row, std::size_t col)
{
    check_bounds(row, col);
    return data_[ index(row, col) ];
}

/**
 * @brief 访问指定位置的元素，带边界检查（常量版本）
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @return 指定位置元素的常量引用
 * @throw std::out_of_range 如果row或col超出范围
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * try {
 *     const int& value = table.at(0, 1); // 获取第一行第二列的元素
 *     std::cout << "值: " << value << std::endl;
 *
 *     // 尝试访问不存在的元素
 *     const int& invalid = table.at(5, 5); // 抛出异常
 * } catch (const std::out_of_range& e) {
 *     std::cerr << "错误: " << e.what() << std::endl;
 * }
 * @endcode
 */
template< typename T >
const T& da_vector_table< T >::at(std::size_t row, std::size_t col) const
{
    check_bounds(row, col);
    return data_[ index(row, col) ];
}

/**
 * @brief 访问表格的第一个元素
 * @tparam T 表格元素类型
 * @return 第一个元素的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * T& first = table.front();
 * first = 10; // 修改第一个元素
 * @endcode
 */
template< typename T >
T& da_vector_table< T >::front()
{
    return data_.front();
}

/**
 * @brief 访问表格的第一个元素（常量版本）
 * @tparam T 表格元素类型
 * @return 第一个元素的常量引用
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * const T& first = table.front();
 * std::cout << "第一个元素: " << first << std::endl;
 * @endcode
 */
template< typename T >
const T& da_vector_table< T >::front() const
{
    return data_.front();
}

/**
 * @brief 访问表格的最后一个元素
 * @tparam T 表格元素类型
 * @return 最后一个元素的引用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * T& last = table.back();
 * last = 10; // 修改最后一个元素
 * @endcode
 */
template< typename T >
T& da_vector_table< T >::back()
{
    return data_.back();
}

/**
 * @brief 访问表格的最后一个元素（常量版本）
 * @tparam T 表格元素类型
 * @return 最后一个元素的常量引用
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * const T& last = table.back();
 * std::cout << "最后一个元素: " << last << std::endl;
 * @endcode
 */
template< typename T >
const T& da_vector_table< T >::back() const
{
    return data_.back();
}

/**
 * @brief 返回指向底层数组的指针
 * @tparam T 表格元素类型
 * @return 指向底层数组的指针
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * int* ptr = table.data();
 * // 可以直接操作底层数组
 * for (std::size_t i = 0; i < table.size(); ++i) {
 *     ptr[i] += 1;
 * }
 * @endcode
 */
template< typename T >
T* da_vector_table< T >::data() noexcept
{
    return data_.data();
}

/**
 * @brief 返回指向底层数组的指针（常量版本）
 * @tparam T 表格元素类型
 * @return 指向底层数组的常量指针
 *
 * @code{.cpp}
 * const DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * const int* ptr = table.data();
 * // 可以读取底层数组
 * for (std::size_t i = 0; i < table.size(); ++i) {
 *     std::cout << ptr[i] << " ";
 * }
 * @endcode
 */
template< typename T >
const T* da_vector_table< T >::data() const noexcept
{
    return data_.data();
}

/**
 * @brief 分配新内容替换当前内容
 * @tparam T 表格元素类型
 * @param rows 新行数
 * @param cols 新列数
 * @param value 填充值
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * // 分配3行4列，所有元素初始化为5
 * table.assign(3, 4, 5);
 * @endcode
 */
template< typename T >
void da_vector_table< T >::assign(std::size_t rows, std::size_t cols, const T& value)
{
    data_.assign(rows * cols, value);
    rows_ = rows;
    cols_ = cols;
}

/**
 * @brief 使用迭代器范围分配新内容替换当前内容
 * @tparam T 表格元素类型
 * @tparam InputIt 输入迭代器类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 *
 * @note 此方法假设输入数据是按行优先顺序排列的，并且行数和列数需要预先知道
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table;
 * std::vector<int> values = {1, 2, 3, 4, 5, 6};
 * // 分配2行3列，使用前6个元素
 * table.assign(values.begin(), values.end());
 * // 需要手动设置行列数
 * table.reshape(2, 3);
 * @endcode
 */
template< typename T >
template< typename InputIt >
void da_vector_table< T >::assign(InputIt first, InputIt last)
{
    data_.assign(first, last);
    // 注意：调用此方法后需要手动设置rows_和cols_，或者调用reshape
}

/**
 * @brief 在表格末尾添加一个元素
 * @tparam T 表格元素类型
 * @param value 要添加的元素
 *
 * @note 此操作会破坏表格的矩形结构，慎用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(2, 2, 1); // 2x2表格
 * table.push_back(5); // 添加一个元素，表格变为非矩形
 * @endcode
 */
template< typename T >
void da_vector_table< T >::push_back(const T& value)
{
    data_.push_back(value);
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

/**
 * @brief 在表格末尾添加一个元素（移动语义）
 * @tparam T 表格元素类型
 * @param value 要添加的元素
 *
 * @note 此操作会破坏表格的矩形结构，慎用
 *
 * @code{.cpp}
 * DA::da_vector_table<std::string> table(2, 2, "hello");
 * std::string s = "world";
 * table.push_back(std::move(s)); // 移动添加一个元素
 * @endcode
 */
template< typename T >
void da_vector_table< T >::push_back(T&& value)
{
    data_.push_back(std::move(value));
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

/**
 * @brief 在表格末尾原位构造一个元素
 * @tparam T 表格元素类型
 * @tparam Args 参数类型
 * @param args 构造参数
 *
 * @note 此操作会破坏表格的矩形结构，慎用
 *
 * @code{.cpp}
 * DA::da_vector_table<std::pair<int, std::string>> table;
 * // 原位构造一个元素
 * table.emplace_back(1, "one");
 * @endcode
 */
template< typename T >
template< typename... Args >
void da_vector_table< T >::emplace_back(Args&&... args)
{
    data_.emplace_back(std::forward< Args >(args)...);
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

/**
 * @brief 移除表格的最后一个元素
 * @tparam T 表格元素类型
 *
 * @note 此操作会破坏表格的矩形结构，慎用
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * table.pop_back(); // 移除最后一个元素4
 * @endcode
 */
template< typename T >
void da_vector_table< T >::pop_back()
{
    data_.pop_back();
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

/**
 * @brief 清空表格中的所有元素
 * @tparam T 表格元素类型
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * table.clear(); // 清空表格
 * // 现在表格为空
 * @endcode
 */
template< typename T >
void da_vector_table< T >::clear() noexcept
{
    data_.clear();
    rows_ = 0;
    cols_ = 0;
}

/**
 * @brief 交换两个表格的内容
 * @tparam T 表格元素类型
 * @param other 要交换的另一个表格
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table1 = {{1, 2}, {3, 4}};
 * DA::da_vector_table<int> table2 = {{5, 6}};
 * table1.swap(table2);
 * // 现在table1包含{{5, 6}}，table2包含{{1, 2}, {3, 4}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::swap(da_vector_table& other) noexcept
{
    data_.swap(other.data_);
    std::swap(rows_, other.rows_);
    std::swap(cols_, other.cols_);
}

/**
 * @brief 在表格末尾添加一行
 * @tparam T 表格元素类型
 * @param row 要添加的行
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(2, 2, 1); // 2x2表格
 * std::vector<int> new_row = {2, 3};
 * table.append_row(new_row); // 添加一行
 * // 现在表格为3x2
 * @endcode
 */
template< typename T >
void da_vector_table< T >::append_row(const std::vector< T >& row)
{
    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = row.size();
    } else if (row.size() != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    data_.insert(data_.end(), row.begin(), row.end());
    ++rows_;
}

/**
 * @brief 在表格末尾添加一行（移动语义）
 * @tparam T 表格元素类型
 * @param row 要添加的行
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(2, 2, 1); // 2x2表格
 * std::vector<int> new_row = {2, 3};
 * table.append_row(std::move(new_row)); // 移动添加一行
 * // 现在表格为3x2，new_row变为空
 * @endcode
 */
template< typename T >
void da_vector_table< T >::append_row(std::vector< T >&& row)
{
    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = row.size();
    } else if (row.size() != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    data_.insert(data_.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
    ++rows_;
}

/**
 * @brief 使用迭代器范围在表格末尾添加一行
 * @tparam T 表格元素类型
 * @tparam InputIt 输入迭代器类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(2, 2, 1); // 2x2表格
 * std::vector<int> values = {2, 3};
 * // 添加前2个元素作为一行
 * table.append_row(values.begin(), values.end());
 * // 现在表格为3x2
 * @endcode
 */
template< typename T >
template< typename InputIt >
void da_vector_table< T >::append_row(InputIt first, InputIt last)
{
    const std::size_t count = std::distance(first, last);
    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = count;
    } else if (count != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    data_.insert(data_.end(), first, last);
    ++rows_;
}

/**
 * @brief 使用初始化列表在表格末尾添加一行
 * @tparam T 表格元素类型
 * @param il 初始化列表
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(2, 2, 1); // 2x2表格
 * // 使用初始化列表添加一行
 * table.append_row({2, 3});
 * // 现在表格为3x2
 * @endcode
 */
template< typename T >
void da_vector_table< T >::append_row(std::initializer_list< T > il)
{
    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = il.size();
    } else if (il.size() != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    data_.insert(data_.end(), il.begin(), il.end());
    ++rows_;
}

/**
 * @brief 在指定位置插入一行
 * @tparam T 表格元素类型
 * @param pos 插入位置
 * @param row 要插入的行
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {4, 5}};
 * std::vector<int> new_row = {7, 8};
 * // 在第一行后插入新行
 * table.insert_row(1, new_row);
 * // 现在表格为 {{1, 2}, {7, 8}, {4, 5}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::insert_row(std::size_t pos, const std::vector< T >& row)
{
    check_row_bounds(pos);  // 允许在末尾插入(pos == rows_)

    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = row.size();
    } else if (row.size() != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    // 计算插入位置
    const std::size_t insert_pos = pos * cols_;
    data_.insert(data_.begin() + insert_pos, row.begin(), row.end());
    ++rows_;
}

/**
 * @brief 在指定位置插入一行（移动语义）
 * @tparam T 表格元素类型
 * @param pos 插入位置
 * @param row 要插入的行
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {4, 5}};
 * std::vector<int> new_row = {7, 8};
 * // 在第一行后插入新行
 * table.insert_row(1, std::move(new_row));
 * // 现在表格为 {{1, 2}, {7, 8}, {4, 5}}，new_row变为空
 * @endcode
 */
template< typename T >
void da_vector_table< T >::insert_row(std::size_t pos, std::vector< T >&& row)
{
    check_row_bounds(pos);  // 允许在末尾插入(pos == rows_)

    if (cols_ == 0) {
        // 空表格，设置列数
        cols_ = row.size();
    } else if (row.size() != cols_) {
        throw std::invalid_argument("Row size must match table column count");
    }

    // 预留足够空间避免重新分配
    if (data_.capacity() < data_.size() + cols_) {
        data_.reserve(data_.capacity() + std::max(data_.capacity(), cols_ * 2));
    }

    // 计算插入位置
    const std::size_t insert_pos = pos * cols_;
    data_.insert(data_.begin() + insert_pos, std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
    ++rows_;
}

/**
 * @brief 移除指定位置的行
 * @tparam T 表格元素类型
 * @param pos 要移除的行位置
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}, {5, 6}};
 * // 移除第二行
 * table.erase_row(1);
 * // 现在表格为 {{1, 2}, {5, 6}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::erase_row(std::size_t pos)
{
    check_row_bounds(pos);

    // 计算要删除的范围
    const std::size_t start_pos = pos * cols_;
    const std::size_t end_pos   = start_pos + cols_;

    data_.erase(data_.begin() + start_pos, data_.begin() + end_pos);
    --rows_;
}

/**
 * @brief 使用迭代器范围在表格末尾添加一列
 * @tparam T 表格元素类型
 * @tparam InputIt 输入迭代器类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * std::vector<int> new_col = {5, 6};
 * // 添加新列
 * table.append_column(new_col.begin(), new_col.end());
 * // 现在表格为 {{1, 2, 5}, {3, 4, 6}}
 * @endcode
 */
template<typename T>
template<typename InputIt>
void da_vector_table<T>::append_column(InputIt first, InputIt last)
{
    // 计算元素数量
    const std::size_t count = std::distance(first, last);
    if (count != rows_) {
        throw std::invalid_argument("Column size must match table row count");
    }
    
    // 创建新向量，容量为添加一列后的大小
    std::vector<T> new_data;
    new_data.reserve(rows_ * (cols_ + 1));
    
    // 复制现有数据并添加新列
    auto it = first;
    for (std::size_t i = 0; i < rows_; ++i) {
        const std::size_t row_start = i * cols_;
        
        // 复制当前行的所有元素
        for (std::size_t j = 0; j < cols_; ++j) {
            new_data.push_back(std::move(data_[row_start + j]));
        }
        
        // 添加新列的元素
        new_data.push_back(*it);
        ++it;
    }
    
    // 更新数据
    data_ = std::move(new_data);
    ++cols_;
}

/**
 * @brief 在表格末尾添加一列
 * @tparam T 表格元素类型
 * @param col 要添加的列
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * std::vector<int> new_col = {5, 6};
 * table.append_column(new_col); // 添加一列
 * // 现在表格为 {{1, 2, 5}, {3, 4, 6}}
 * @endcode
 */
template<typename T>
void da_vector_table<T>::append_column(const std::vector<T>& col)
{
    // 使用迭代器版本
    append_column(col.begin(), col.end());
}

/**
 * @brief 在表格末尾添加一列（移动语义）
 * @tparam T 表格元素类型
 * @param col 要添加的列
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * std::vector<int> new_col = {5, 6};
 * table.append_column(std::move(new_col)); // 移动添加一列
 * // 现在表格为 {{1, 2, 5}, {3, 4, 6}}，new_col变为空
 * @endcode
 */
template<typename T>
void da_vector_table<T>::append_column(std::vector<T>&& col)
{
    // 使用迭代器版本
    append_column(std::make_move_iterator(col.begin()), 
                  std::make_move_iterator(col.end()));
}

/**
 * @brief 使用初始化列表在表格末尾添加一列
 * @tparam T 表格元素类型
 * @param il 初始化列表
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 使用初始化列表添加新列
 * table.append_column({5, 6});
 * // 现在表格为 {{1, 2, 5}, {3, 4, 6}}
 * @endcode
 */
template<typename T>
void da_vector_table<T>::append_column(std::initializer_list<T> il)
{
    // 使用迭代器版本
    append_column(il.begin(), il.end());
}

/**
 * @brief 在指定位置插入一列
 * @tparam T 表格元素类型
 * @param pos 插入位置
 * @param col 要插入的列
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 3}, {4, 6}};
 * std::vector<int> new_col = {2, 5};
 * // 在第一列后插入新列
 * table.insert_column(1, new_col);
 * // 现在表格为 {{1, 2, 3}, {4, 5, 6}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::insert_column(std::size_t pos, const std::vector< T >& col)
{
    check_column_bounds(pos);  // 允许在末尾插入(pos == cols_)

    if (col.size() != rows_) {
        throw std::invalid_argument("Column size must match table row count");
    }

    // 创建新向量，容量为插入一列后的大小
    std::vector< T > new_data;
    new_data.reserve(rows_ * (cols_ + 1));

    // 复制数据并插入新列
    for (std::size_t i = 0; i < rows_; ++i) {
        const std::size_t row_start = i * cols_;

        // 复制当前行中插入位置之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(data_[ row_start + j ]);
        }

        // 插入新列的元素
        new_data.push_back(col[ i ]);

        // 复制当前行中插入位置之后的元素
        for (std::size_t j = pos; j < cols_; ++j) {
            new_data.push_back(data_[ row_start + j ]);
        }
    }

    // 更新数据
    data_ = std::move(new_data);
    ++cols_;
}

/**
 * @brief 在指定位置插入一列（移动语义）
 * @tparam T 表格元素类型
 * @param pos 插入位置
 * @param col 要插入的列
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 3}, {4, 6}};
 * std::vector<int> new_col = {2, 5};
 * // 在第一列后插入新列
 * table.insert_column(1, std::move(new_col));
 * // 现在表格为 {{1, 2, 3}, {4, 5, 6}}，new_col变为空
 * @endcode
 */
template<typename T>
void da_vector_table<T>::insert_column(std::size_t pos, std::vector<T>&& col)
{
    check_column_bounds(pos); // 允许在末尾插入(pos == cols_)
    
    if (col.size() != rows_) {
        throw std::invalid_argument("Column size must match table row count");
    }
    
    // 创建新向量，容量为插入一列后的大小
    std::vector<T> new_data;
    new_data.reserve(rows_ * (cols_ + 1));
    
    // 复制数据并插入新列
    for (std::size_t i = 0; i < rows_; ++i) {
        const std::size_t row_start = i * cols_;
        
        // 复制当前行中插入位置之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(std::move(data_[row_start + j]));
        }
        
        // 插入新列的元素（使用移动语义）
        new_data.push_back(std::move(col[i]));
        
        // 复制当前行中插入位置之后的元素
        for (std::size_t j = pos; j < cols_; ++j) {
            new_data.push_back(std::move(data_[row_start + j]));
        }
    }
    
    // 更新数据
    data_ = std::move(new_data);
    ++cols_;
}

/**
 * @brief 移除指定位置的列（高效版本）
 * @tparam T 表格元素类型
 * @param pos 要移除的列位置
 *
 * 此版本通过创建新向量来避免多次数据移动，提高大型表格的操作效率。
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3}, {4, 5, 6}};
 * // 移除第二列
 * table.erase_column(1);
 * // 现在表格为 {{1, 3}, {4, 6}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::erase_column(std::size_t pos)
{
    check_column_bounds(pos);

    // 创建新向量，容量为移除一列后的大小
    std::vector< T > new_data;
    new_data.reserve(rows_ * (cols_ - 1));

    // 复制除了指定列之外的所有元素
    for (std::size_t i = 0; i < rows_; ++i) {
        const std::size_t row_start = i * cols_;

        // 复制当前行中指定列之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(data_[ row_start + j ]);
        }

        // 跳过指定列，复制之后的元素
        for (std::size_t j = pos + 1; j < cols_; ++j) {
            new_data.push_back(data_[ row_start + j ]);
        }
    }

    // 更新数据
    data_ = std::move(new_data);
    --cols_;
}

/**
 * @brief 获取表格的形状（行数和列数）
 * @tparam T 表格元素类型
 * @return 包含行数和列数的pair
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(3, 4, 1);
 * auto shape = table.shape();
 * std::cout << "表格形状: " << shape.first << " 行, "
 *           << shape.second << " 列" << std::endl;
 * // 输出: 表格形状: 3 行, 4 列
 * @endcode
 */
template< typename T >
typename da_vector_table< T >::table_index_type da_vector_table< T >::shape() const noexcept
{
    return { rows_, cols_ };
}

/**
 * @brief 获取表格的行数
 * @tparam T 表格元素类型
 * @return 表格的行数
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(3, 4);
 * std::cout << "表格有 " << table.row_count() << " 行" << std::endl;
 * // 输出: 表格有 3 行
 * @endcode
 */
template< typename T >
std::size_t da_vector_table< T >::row_count() const noexcept
{
    return rows_;
}

/**
 * @brief 获取表格的列数
 * @tparam T 表格元素类型
 * @return 表格的列数
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table(3, 4);
 * std::cout << "表格有 " << table.column_count() << " 列" << std::endl;
 * // 输出: 表格有 4 列
 * @endcode
 */
template< typename T >
std::size_t da_vector_table< T >::column_count() const noexcept
{
    return cols_;
}

/**
 * @brief 调整表格大小
 * @tparam T 表格元素类型
 * @param rows 新的行数
 * @param cols 新的列数
 * @param value 新元素的初始值
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 调整大小为3行4列，新元素初始化为0
 * table.resize(3, 4, 0);
 * // 现在表格为3行4列，新增元素为0
 * @endcode
 */
template< typename T >
void da_vector_table< T >::resize(std::size_t rows, std::size_t cols, const T& value)
{
    if (rows == rows_ && cols == cols_) {
        return;
    }

    // 如果新大小小于等于当前容量，且列数不变，可以原地调整
    if (cols == cols_ && rows * cols <= data_.capacity()) {
        if (rows > rows_) {
            // 增加行数
            data_.resize(rows * cols, value);
        } else {
            // 减少行数
            data_.resize(rows * cols);
        }
        rows_ = rows;
        return;
    }

    // 否则需要重新移动
    std::vector< T > new_data(rows * cols, value);

    const std::size_t copy_rows = std::min(rows, rows_);
    const std::size_t copy_cols = std::min(cols, cols_);

    for (std::size_t i = 0; i < copy_rows; ++i) {
        for (std::size_t j = 0; j < copy_cols; ++j) {
            new_data[ i * cols + j ] = data_[ i * cols_ + j ];
        }
    }

    data_ = std::move(new_data);
    rows_ = rows;
    cols_ = cols;
}

/**
 * @brief 使用形状对象调整表格大小
 * @tparam T 表格元素类型
 * @param sh 包含新行数和列数的形状对象
 * @param value 新元素的初始值
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2}, {3, 4}};
 * // 调整大小为3行4列，新元素初始化为0
 * table.resize({3, 4}, 0);
 * // 现在表格为3行4列，新增元素为0
 * @endcode
 */
template< typename T >
void da_vector_table< T >::resize(table_index_type sh, const T& value)
{
    resize(sh.first, sh.second, value);
}

/**
 * @brief 改变表格形状而不改变数据总量
 * @tparam T 表格元素类型
 * @param rows 新的行数
 * @param cols 新的列数
 * @throw std::invalid_argument 如果新形状的元素总数与当前不同
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3, 4}};
 * // 改变形状为2x2
 * table.reshape(2, 2);
 * // 现在表格为 {{1, 2}, {3, 4}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::reshape(std::size_t rows, std::size_t cols)
{
    if (rows * cols != data_.size()) {
        throw std::invalid_argument("New shape must have the same number of elements");
    }

    rows_ = rows;
    cols_ = cols;
}

/**
 * @brief 获取指定行的数据
 * @tparam T 表格元素类型
 * @param row 行索引
 * @return 包含指定行数据的向量
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3}, {4, 5, 6}};
 * // 获取第一行
 * std::vector<int> row0 = table.get_row(0);
 * // row0 包含 {1, 2, 3}
 * @endcode
 */
template< typename T >
std::vector< T > da_vector_table< T >::get_row(std::size_t row) const
{
    check_row_bounds(row);

    std::vector< T > result;
    result.reserve(cols_);

    const std::size_t start = row * cols_;
    for (std::size_t i = 0; i < cols_; ++i) {
        result.push_back(data_[ start + i ]);
    }

    return result;
}

/**
 * @brief 获取指定列的数据
 * @tparam T 表格元素类型
 * @param col 列索引
 * @return 包含指定列数据的向量
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3}, {4, 5, 6}};
 * // 获取第二列
 * std::vector<int> col1 = table.get_column(1);
 * // col1 包含 {2, 5}
 * @endcode
 */
template< typename T >
std::vector< T > da_vector_table< T >::get_column(std::size_t col) const
{
    check_column_bounds(col);

    std::vector< T > result;
    result.reserve(rows_);

    for (std::size_t i = 0; i < rows_; ++i) {
        result.push_back(data_[ i * cols_ + col ]);
    }

    return result;
}

/**
 * @brief 设置指定行的数据
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param values 要设置的值
 * @throw std::invalid_argument 如果值数量与列数不匹配
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3}, {4, 5, 6}};
 * // 设置第一行
 * table.set_row(0, {7, 8, 9});
 * // 现在表格为 {{7, 8, 9}, {4, 5, 6}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::set_row(std::size_t row, const std::vector< T >& values)
{
    check_row_bounds(row);

    if (values.size() != cols_) {
        throw std::invalid_argument("Number of values must match column count");
    }

    const std::size_t start = row * cols_;
    for (std::size_t i = 0; i < cols_; ++i) {
        data_[ start + i ] = values[ i ];
    }
}

/**
 * @brief 设置指定列的数据
 * @tparam T 表格元素类型
 * @param col 列索引
 * @param values 要设置的值
 * @throw std::invalid_argument 如果值数量与行数不匹配
 *
 * @code{.cpp}
 * DA::da_vector_table<int> table = {{1, 2, 3}, {4, 5, 6}};
 * // 设置第二列
 * table.set_column(1, {7, 8});
 * // 现在表格为 {{1, 7, 3}, {4, 8, 6}}
 * @endcode
 */
template< typename T >
void da_vector_table< T >::set_column(std::size_t col, const std::vector< T >& values)
{
    check_column_bounds(col);

    if (values.size() != rows_) {
        throw std::invalid_argument("Number of values must match row count");
    }

    for (std::size_t i = 0; i < rows_; ++i) {
        data_[ i * cols_ + col ] = values[ i ];
    }
}

// ============================================================================
// 私有辅助函数实现
// ============================================================================

/**
 * @brief 计算二维索引对应的一维索引
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @return 一维索引
 */
template< typename T >
std::size_t da_vector_table< T >::index(std::size_t row, std::size_t col) const noexcept
{
    return row * cols_ + col;
}

/**
 * @brief 检查行列索引是否在有效范围内
 * @tparam T 表格元素类型
 * @param row 行索引
 * @param col 列索引
 * @throw std::out_of_range 如果索引超出范围
 */
template< typename T >
void da_vector_table< T >::check_bounds(std::size_t row, std::size_t col) const
{
    if (row >= rows_) {
        throw std::out_of_range("Row index out of range");
    }
    if (col >= cols_) {
        throw std::out_of_range("Column index out of range");
    }
}

/**
 * @brief 检查行索引是否在有效范围内
 * @tparam T 表格元素类型
 * @param row 行索引
 * @throw std::out_of_range 如果索引超出范围
 */
template< typename T >
void da_vector_table< T >::check_row_bounds(std::size_t row) const
{
    if (row > rows_) {  // 允许在末尾插入，所以可以等于rows_
        throw std::out_of_range("Row index out of range");
    }
}

/**
 * @brief 检查列索引是否在有效范围内
 * @tparam T 表格元素类型
 * @param col 列索引
 * @throw std::out_of_range 如果索引超出范围
 */
template< typename T >
void da_vector_table< T >::check_column_bounds(std::size_t col) const
{
    if (col > cols_) {  // 允许在末尾插入，所以可以等于cols_
        throw std::out_of_range("Column index out of range");
    }
}

}  // namespace DA

#endif  // DA_VECTOR_TABLE_H
