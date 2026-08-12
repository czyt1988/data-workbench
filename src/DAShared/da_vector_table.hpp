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
    std::vector< T > mData;  // 一维数据存储
    std::size_t mRows;       // 行数
    std::size_t mCols;       // 列数

    // 辅助函数
    std::size_t index(std::size_t row, std::size_t col) const noexcept;
    void check_bounds(std::size_t row, std::size_t col) const;
    void check_row_bounds(std::size_t row) const;
    void check_column_bounds(std::size_t col) const;
};

// ============================================================================
// 实现部分
// ============================================================================

// 默认构造函数
template< typename T >
da_vector_table< T >::da_vector_table() : mData(), mRows(0), mCols(0)
{
}

// 构造函数，创建指定大小的表格
template< typename T >
da_vector_table< T >::da_vector_table(std::size_t rows, std::size_t cols, const T& value)
    : mData(rows * cols, value), mRows(rows), mCols(cols)
{
}

// 使用初始化列表构造表格
template< typename T >
da_vector_table< T >::da_vector_table(std::initializer_list< std::initializer_list< T > > init)
    : mRows(init.size()), mCols(0)
{
    // 确定最大列数
    for (const auto& row : init) {
        if (row.size() > mCols) {
            mCols = row.size();
        }
    }

    // 分配内存
    mData.resize(mRows * mCols);

    // 填充数据
    std::size_t row_idx = 0;
    for (const auto& row : init) {
        std::size_t col_idx = 0;
        for (const auto& value : row) {
            mData[ index(row_idx, col_idx) ] = value;
            ++col_idx;
        }
        // 填充剩余部分（如果有）
        for (; col_idx < mCols; ++col_idx) {
            mData[ index(row_idx, col_idx) ] = T();
        }
        ++row_idx;
    }
}

// 拷贝构造函数
template< typename T >
da_vector_table< T >::da_vector_table(const da_vector_table& other)
    : mData(other.mData), mRows(other.mRows), mCols(other.mCols)
{
}

// 移动构造函数
template< typename T >
da_vector_table< T >::da_vector_table(da_vector_table&& other) noexcept
    : mData(std::move(other.mData)), mRows(other.mRows), mCols(other.mCols)
{
    other.mRows = 0;
    other.mCols = 0;
}

// 拷贝赋值运算符
template< typename T >
da_vector_table< T >& da_vector_table< T >::operator=(const da_vector_table& other)
{
    if (this != &other) {
        mData = other.mData;
        mRows = other.mRows;
        mCols = other.mCols;
    }
    return *this;
}

// 移动赋值运算符
template< typename T >
da_vector_table< T >& da_vector_table< T >::operator=(da_vector_table&& other) noexcept
{
    if (this != &other) {
        mData       = std::move(other.mData);
        mRows       = other.mRows;
        mCols       = other.mCols;
        other.mRows = 0;
        other.mCols = 0;
    }
    return *this;
}

// 检查表格是否为空
template< typename T >
bool da_vector_table< T >::empty() const noexcept
{
    return mData.empty();
}

// 返回表格中的元素总数
template< typename T >
std::size_t da_vector_table< T >::size() const noexcept
{
    return mData.size();
}

// 返回表格可容纳的最大元素数
template< typename T >
std::size_t da_vector_table< T >::max_size() const noexcept
{
    return mData.max_size();
}

// 预留存储空间
template< typename T >
void da_vector_table< T >::reserve(std::size_t new_cap)
{
    mData.reserve(new_cap);
}

// 返回当前分配的存储容量
template< typename T >
std::size_t da_vector_table< T >::capacity() const noexcept
{
    return mData.capacity();
}

// 请求移除未使用的容量
template< typename T >
void da_vector_table< T >::shrink_to_fit()
{
    mData.shrink_to_fit();
}

// 返回指向表格第一个元素的迭代器
template< typename T >
typename da_vector_table< T >::iterator da_vector_table< T >::begin() noexcept
{
    return mData.begin();
}

// 返回指向表格第一个元素的常量迭代器
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::begin() const noexcept
{
    return mData.begin();
}

// 返回指向表格第一个元素的常量迭代器
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::cbegin() const noexcept
{
    return mData.cbegin();
}

// 返回指向表格尾后位置的迭代器
template< typename T >
typename da_vector_table< T >::iterator da_vector_table< T >::end() noexcept
{
    return mData.end();
}

// 返回指向表格尾后位置的常量迭代器
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::end() const noexcept
{
    return mData.end();
}

// 返回指向表格尾后位置的常量迭代器
template< typename T >
typename da_vector_table< T >::const_iterator da_vector_table< T >::cend() const noexcept
{
    return mData.cend();
}

// 返回指向表格最后一个元素的反向迭代器
template< typename T >
typename da_vector_table< T >::reverse_iterator da_vector_table< T >::rbegin() noexcept
{
    return mData.rbegin();
}

// 返回指向表格最后一个元素的常量反向迭代器
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::rbegin() const noexcept
{
    return mData.rbegin();
}

// 返回指向表格最后一个元素的常量反向迭代器
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::crbegin() const noexcept
{
    return mData.crbegin();
}

// 返回指向表格第一个元素前一个位置的反向迭代器
template< typename T >
typename da_vector_table< T >::reverse_iterator da_vector_table< T >::rend() noexcept
{
    return mData.rend();
}

// 返回指向表格第一个元素前一个位置的常量反向迭代器
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::rend() const noexcept
{
    return mData.rend();
}

// 返回指向表格第一个元素前一个位置的常量反向迭代器
template< typename T >
typename da_vector_table< T >::const_reverse_iterator da_vector_table< T >::crend() const noexcept
{
    return mData.crend();
}

// 访问指定位置的元素（函数调用运算符）
template< typename T >
T& da_vector_table< T >::operator()(std::size_t row, std::size_t col)
{
    return mData[ index(row, col) ];
}

// 访问指定位置的元素（函数调用运算符，常量版本）
template< typename T >
const T& da_vector_table< T >::operator()(std::size_t row, std::size_t col) const
{
    return mData[ index(row, col) ];
}

// 访问指定位置的元素，带边界检查
template< typename T >
T& da_vector_table< T >::at(std::size_t row, std::size_t col)
{
    check_bounds(row, col);
    return mData[ index(row, col) ];
}

// 访问指定位置的元素，带边界检查（常量版本）
template< typename T >
const T& da_vector_table< T >::at(std::size_t row, std::size_t col) const
{
    check_bounds(row, col);
    return mData[ index(row, col) ];
}

// 访问表格的第一个元素
template< typename T >
T& da_vector_table< T >::front()
{
    return mData.front();
}

// 访问表格的第一个元素（常量版本）
template< typename T >
const T& da_vector_table< T >::front() const
{
    return mData.front();
}

// 访问表格的最后一个元素
template< typename T >
T& da_vector_table< T >::back()
{
    return mData.back();
}

// 访问表格的最后一个元素（常量版本）
template< typename T >
const T& da_vector_table< T >::back() const
{
    return mData.back();
}

// 返回指向底层数组的指针
template< typename T >
T* da_vector_table< T >::data() noexcept
{
    return mData.data();
}

// 返回指向底层数组的指针（常量版本）
template< typename T >
const T* da_vector_table< T >::data() const noexcept
{
    return mData.data();
}

// 分配新内容替换当前内容
template< typename T >
void da_vector_table< T >::assign(std::size_t rows, std::size_t cols, const T& value)
{
    mData.assign(rows * cols, value);
    mRows = rows;
    mCols = cols;
}

// 使用迭代器范围分配新内容替换当前内容
template< typename T >
template< typename InputIt >
void da_vector_table< T >::assign(InputIt first, InputIt last)
{
    mData.assign(first, last);
    // 注意：调用此方法后需要手动设置mRows和mCols，或者调用reshape
}

// 在表格末尾添加一个元素
template< typename T >
void da_vector_table< T >::push_back(const T& value)
{
    mData.push_back(value);
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

// 在表格末尾添加一个元素（移动语义）
template< typename T >
void da_vector_table< T >::push_back(T&& value)
{
    mData.push_back(std::move(value));
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

// 在表格末尾原位构造一个元素
template< typename T >
template< typename... Args >
void da_vector_table< T >::emplace_back(Args&&... args)
{
    mData.emplace_back(std::forward< Args >(args)...);
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

// 移除表格的最后一个元素
template< typename T >
void da_vector_table< T >::pop_back()
{
    mData.pop_back();
    // 注意：此操作会破坏表格结构，需要谨慎使用
}

// 清空表格中的所有元素
template< typename T >
void da_vector_table< T >::clear() noexcept
{
    mData.clear();
    mRows = 0;
    mCols = 0;
}

// 交换两个表格的内容
template< typename T >
void da_vector_table< T >::swap(da_vector_table& other) noexcept
{
    mData.swap(other.mData);
    std::swap(mRows, other.mRows);
    std::swap(mCols, other.mCols);
}

// 在表格末尾添加一行
template< typename T >
void da_vector_table< T >::append_row(const std::vector< T >& row)
{
    if (mCols == 0) {
        // 空表格，设置列数
        mCols = row.size();
    } else if (row.size() != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    mData.insert(mData.end(), row.begin(), row.end());
    ++mRows;
}

// 在表格末尾添加一行（移动语义）
template< typename T >
void da_vector_table< T >::append_row(std::vector< T >&& row)
{
    if (mCols == 0) {
        // 空表格，设置列数
        mCols = row.size();
    } else if (row.size() != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    mData.insert(mData.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
    ++mRows;
}

// 使用迭代器范围在表格末尾添加一行
template< typename T >
template< typename InputIt >
void da_vector_table< T >::append_row(InputIt first, InputIt last)
{
    const std::size_t count = std::distance(first, last);
    if (mCols == 0) {
        // 空表格，设置列数
        mCols = count;
    } else if (count != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    mData.insert(mData.end(), first, last);
    ++mRows;
}

// 使用初始化列表在表格末尾添加一行
template< typename T >
void da_vector_table< T >::append_row(std::initializer_list< T > il)
{
    if (mCols == 0) {
        // 空表格，设置列数
        mCols = il.size();
    } else if (il.size() != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    mData.insert(mData.end(), il.begin(), il.end());
    ++mRows;
}

// 在指定位置插入一行
template< typename T >
void da_vector_table< T >::insert_row(std::size_t pos, const std::vector< T >& row)
{
    check_row_bounds(pos);  // 允许在末尾插入(pos == mRows)

    if (mCols == 0) {
        // 空表格，设置列数
        mCols = row.size();
    } else if (row.size() != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    // 计算插入位置
    const std::size_t insert_pos = pos * mCols;
    mData.insert(mData.begin() + insert_pos, row.begin(), row.end());
    ++mRows;
}

// 在指定位置插入一行（移动语义）
template< typename T >
void da_vector_table< T >::insert_row(std::size_t pos, std::vector< T >&& row)
{
    check_row_bounds(pos);  // 允许在末尾插入(pos == mRows)

    if (mCols == 0) {
        // 空表格，设置列数
        mCols = row.size();
    } else if (row.size() != mCols) {
        throw std::invalid_argument("Row size must match table column count");
    }

    // 预留足够空间避免重新分配
    if (mData.capacity() < mData.size() + mCols) {
        mData.reserve(mData.capacity() + std::max(mData.capacity(), mCols * 2));
    }

    // 计算插入位置
    const std::size_t insert_pos = pos * mCols;
    mData.insert(mData.begin() + insert_pos, std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
    ++mRows;
}

// 移除指定位置的行
template< typename T >
void da_vector_table< T >::erase_row(std::size_t pos)
{
    check_row_bounds(pos);

    // 计算要删除的范围
    const std::size_t start_pos = pos * mCols;
    const std::size_t end_pos   = start_pos + mCols;

    mData.erase(mData.begin() + start_pos, mData.begin() + end_pos);
    --mRows;
}

// 使用迭代器范围在表格末尾添加一列
template<typename T>
template<typename InputIt>
void da_vector_table<T>::append_column(InputIt first, InputIt last)
{
    // 计算元素数量
    const std::size_t count = std::distance(first, last);
    if (count != mRows) {
        throw std::invalid_argument("Column size must match table row count");
    }
    
    // 创建新向量，容量为添加一列后的大小
    std::vector<T> new_data;
    new_data.reserve(mRows * (mCols + 1));
    
    // 复制现有数据并添加新列
    auto it = first;
    for (std::size_t i = 0; i < mRows; ++i) {
        const std::size_t row_start = i * mCols;
        
        // 复制当前行的所有元素
        for (std::size_t j = 0; j < mCols; ++j) {
            new_data.push_back(std::move(mData[row_start + j]));
        }
        
        // 添加新列的元素
        new_data.push_back(*it);
        ++it;
    }
    
    // 更新数据
    mData = std::move(new_data);
    ++mCols;
}

// 在表格末尾添加一列
template<typename T>
void da_vector_table<T>::append_column(const std::vector<T>& col)
{
    // 使用迭代器版本
    append_column(col.begin(), col.end());
}

// 在表格末尾添加一列（移动语义）
template<typename T>
void da_vector_table<T>::append_column(std::vector<T>&& col)
{
    // 使用迭代器版本
    append_column(std::make_move_iterator(col.begin()), 
                  std::make_move_iterator(col.end()));
}

// 使用初始化列表在表格末尾添加一列
template<typename T>
void da_vector_table<T>::append_column(std::initializer_list<T> il)
{
    // 使用迭代器版本
    append_column(il.begin(), il.end());
}

// 在指定位置插入一列
template< typename T >
void da_vector_table< T >::insert_column(std::size_t pos, const std::vector< T >& col)
{
    check_column_bounds(pos);  // 允许在末尾插入(pos == mCols)

    if (col.size() != mRows) {
        throw std::invalid_argument("Column size must match table row count");
    }

    // 创建新向量，容量为插入一列后的大小
    std::vector< T > new_data;
    new_data.reserve(mRows * (mCols + 1));

    // 复制数据并插入新列
    for (std::size_t i = 0; i < mRows; ++i) {
        const std::size_t row_start = i * mCols;

        // 复制当前行中插入位置之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(mData[ row_start + j ]);
        }

        // 插入新列的元素
        new_data.push_back(col[ i ]);

        // 复制当前行中插入位置之后的元素
        for (std::size_t j = pos; j < mCols; ++j) {
            new_data.push_back(mData[ row_start + j ]);
        }
    }

    // 更新数据
    mData = std::move(new_data);
    ++mCols;
}

// 在指定位置插入一列（移动语义）
template<typename T>
void da_vector_table<T>::insert_column(std::size_t pos, std::vector<T>&& col)
{
    check_column_bounds(pos); // 允许在末尾插入(pos == mCols)
    
    if (col.size() != mRows) {
        throw std::invalid_argument("Column size must match table row count");
    }
    
    // 创建新向量，容量为插入一列后的大小
    std::vector<T> new_data;
    new_data.reserve(mRows * (mCols + 1));
    
    // 复制数据并插入新列
    for (std::size_t i = 0; i < mRows; ++i) {
        const std::size_t row_start = i * mCols;
        
        // 复制当前行中插入位置之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(std::move(mData[row_start + j]));
        }
        
        // 插入新列的元素（使用移动语义）
        new_data.push_back(std::move(col[i]));
        
        // 复制当前行中插入位置之后的元素
        for (std::size_t j = pos; j < mCols; ++j) {
            new_data.push_back(std::move(mData[row_start + j]));
        }
    }
    
    // 更新数据
    mData = std::move(new_data);
    ++mCols;
}

// 移除指定位置的列（高效版本）
template< typename T >
void da_vector_table< T >::erase_column(std::size_t pos)
{
    check_column_bounds(pos);

    // 创建新向量，容量为移除一列后的大小
    std::vector< T > new_data;
    new_data.reserve(mRows * (mCols - 1));

    // 复制除了指定列之外的所有元素
    for (std::size_t i = 0; i < mRows; ++i) {
        const std::size_t row_start = i * mCols;

        // 复制当前行中指定列之前的元素
        for (std::size_t j = 0; j < pos; ++j) {
            new_data.push_back(mData[ row_start + j ]);
        }

        // 跳过指定列，复制之后的元素
        for (std::size_t j = pos + 1; j < mCols; ++j) {
            new_data.push_back(mData[ row_start + j ]);
        }
    }

    // 更新数据
    mData = std::move(new_data);
    --mCols;
}

// 获取表格的形状（行数和列数）
template< typename T >
typename da_vector_table< T >::table_index_type da_vector_table< T >::shape() const noexcept
{
    return { mRows, mCols };
}

// 获取表格的行数
template< typename T >
std::size_t da_vector_table< T >::row_count() const noexcept
{
    return mRows;
}

// 获取表格的列数
template< typename T >
std::size_t da_vector_table< T >::column_count() const noexcept
{
    return mCols;
}

// 调整表格大小
template< typename T >
void da_vector_table< T >::resize(std::size_t rows, std::size_t cols, const T& value)
{
    if (rows == mRows && cols == mCols) {
        return;
    }

    // 如果新大小小于等于当前容量，且列数不变，可以原地调整
    if (cols == mCols && rows * cols <= mData.capacity()) {
        if (rows > mRows) {
            // 增加行数
            mData.resize(rows * cols, value);
        } else {
            // 减少行数
            mData.resize(rows * cols);
        }
        mRows = rows;
        return;
    }

    // 否则需要重新移动
    std::vector< T > new_data(rows * cols, value);

    const std::size_t copy_rows = std::min(rows, mRows);
    const std::size_t copy_cols = std::min(cols, mCols);

    for (std::size_t i = 0; i < copy_rows; ++i) {
        for (std::size_t j = 0; j < copy_cols; ++j) {
            new_data[ i * cols + j ] = mData[ i * mCols + j ];
        }
    }

    mData = std::move(new_data);
    mRows = rows;
    mCols = cols;
}

// 使用形状对象调整表格大小
template< typename T >
void da_vector_table< T >::resize(table_index_type sh, const T& value)
{
    resize(sh.first, sh.second, value);
}

// 改变表格形状而不改变数据总量
template< typename T >
void da_vector_table< T >::reshape(std::size_t rows, std::size_t cols)
{
    if (rows * cols != mData.size()) {
        throw std::invalid_argument("New shape must have the same number of elements");
    }

    mRows = rows;
    mCols = cols;
}

// 获取指定行的数据
template< typename T >
std::vector< T > da_vector_table< T >::get_row(std::size_t row) const
{
    check_row_bounds(row);

    std::vector< T > result;
    result.reserve(mCols);

    const std::size_t start = row * mCols;
    for (std::size_t i = 0; i < mCols; ++i) {
        result.push_back(mData[ start + i ]);
    }

    return result;
}

// 获取指定列的数据
template< typename T >
std::vector< T > da_vector_table< T >::get_column(std::size_t col) const
{
    check_column_bounds(col);

    std::vector< T > result;
    result.reserve(mRows);

    for (std::size_t i = 0; i < mRows; ++i) {
        result.push_back(mData[ i * mCols + col ]);
    }

    return result;
}

// 设置指定行的数据
template< typename T >
void da_vector_table< T >::set_row(std::size_t row, const std::vector< T >& values)
{
    check_row_bounds(row);

    if (values.size() != mCols) {
        throw std::invalid_argument("Number of values must match column count");
    }

    const std::size_t start = row * mCols;
    for (std::size_t i = 0; i < mCols; ++i) {
        mData[ start + i ] = values[ i ];
    }
}

// 设置指定列的数据
template< typename T >
void da_vector_table< T >::set_column(std::size_t col, const std::vector< T >& values)
{
    check_column_bounds(col);

    if (values.size() != mRows) {
        throw std::invalid_argument("Number of values must match row count");
    }

    for (std::size_t i = 0; i < mRows; ++i) {
        mData[ i * mCols + col ] = values[ i ];
    }
}

// ============================================================================
// 私有辅助函数实现
// ============================================================================

// 计算二维索引对应的一维索引
template< typename T >
std::size_t da_vector_table< T >::index(std::size_t row, std::size_t col) const noexcept
{
    return row * mCols + col;
}

// 检查行列索引是否在有效范围内
template< typename T >
void da_vector_table< T >::check_bounds(std::size_t row, std::size_t col) const
{
    if (row >= mRows) {
        throw std::out_of_range("Row index out of range");
    }
    if (col >= mCols) {
        throw std::out_of_range("Column index out of range");
    }
}

// 检查行索引是否在有效范围内
template< typename T >
void da_vector_table< T >::check_row_bounds(std::size_t row) const
{
    if (row > mRows) {  // 允许在末尾插入，所以可以等于mRows
        throw std::out_of_range("Row index out of range");
    }
}

// 检查列索引是否在有效范围内
template< typename T >
void da_vector_table< T >::check_column_bounds(std::size_t col) const
{
    if (col > mCols) {  // 允许在末尾插入，所以可以等于mCols
        throw std::out_of_range("Column index out of range");
    }
}

}  // namespace DA

#endif  // DA_VECTOR_TABLE_H
