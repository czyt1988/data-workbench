#ifndef DA_VECTOR_TABLE_H
#define DA_VECTOR_TABLE_H
#include <vector>
namespace DA
{
/**
 * @brief 由vector<vector>组成的表
 */
template< typename T >
class da_vector_table : public std::vector< std::vector< T > >
{
public:
    using row_type         = std::vector< T >;                       ///< 行类型
    using column_type      = std::vector< T >;                       ///< 列类型
    using table_index_type = std::pair< std::size_t, std::size_t >;  ///< 索引类型
    using super_class      = std::vector< std::vector< T > >;        ///< 父类

    using value_type             = typename super_class::value_type;
    using allocator_type         = typename super_class::allocator_type;
    using pointer                = typename super_class::pointer;
    using const_pointer          = typename super_class::const_pointer;
    using reference              = typename super_class::reference;
    using const_reference        = typename super_class::const_reference;
    using size_type              = typename super_class::size_type;
    using difference_type        = typename super_class::difference_type;
    using iterator               = typename super_class::iterator;
    using const_iterator         = typename super_class::const_iterator;
    using reverse_iterator       = typename super_class::reverse_iterator;
    using const_reverse_iterator = typename super_class::const_reverse_iterator;

public:
    da_vector_table() : super_class()
    {
    }
    /**
     * @brief 生成一个rowcnt行，colcnt列的T()填充的表格
     * @param rowcnt 行数
     * @param colcnt 列数
     */
    da_vector_table(std::size_t rowcnt, std::size_t colcnt) : super_class(rowcnt, row_type(colcnt))
    {
    }
    da_vector_table(std::size_t rowcnt, std::size_t colcnt, const T& v) : super_class(rowcnt, row_type(colcnt, v))
    {
    }
    // 插入一行
    void append_row(const row_type& row)
    {
        emplace_back(row);
    }
    void append_row(row_type&& row)
    {
        emplace_back(row);
    }
    template< typename InputIt >
    void append_row(InputIt first, InputIt last)
    {
        emplace_back(row_type(first, last));
    }
    void append_row(const std::initializer_list< T >& il)
    {
        emplace_back(row_type(il));
    }
    // 插入列
    template< typename InputIt >
    void append_column(InputIt first, InputIt last)
    {
        auto i  = begin();
        auto ci = first;
        while (i != end() && ci != last) {
            i->emplace_back(*ci);
            ++i;
            ++ci;
        }
    }
    void append_column(const column_type& col)
    {
        append_column(col.begin(), col.end());
    }

    void append_column(const std::initializer_list< T >& il)
    {
        append_column(il.begin(), il.end());
    }

    reference at(std::size_t r, std::size_t c)
    {
        return at(r).at(c);
    }
    const_reference at(std::size_t r, std::size_t c) const
    {
        return at(r).at(c);
    }
    /**
     * @brief 计算最大的形状
     * @return 此函数会遍历所有的行，时间复杂度为O(n)
     */
    table_index_type shape() const
    {
        table_index_type sh;
        auto i = cbegin();
        if (i == cend()) {
            return table_index_type();
        }
        sh.second = (*i).size();
        while (i != cend()) {
            if (sh.second < (*i).size()) {
                sh.second = (*i).size();
            }
            ++i;
        }
        sh.first = size();
        return sh;
    }
    /**
       @brief 行数
       @return
     */
    std::size_t row_count() const
    {
        return super_class::size();
    }

    /**
       @brief 列数
       @note 由于有可能每行的列不一样，因此列数可指定行，默认为第一行
       @note 如果要求最大的形状，使用@ref shape
       @return
     */
    std::size_t column_count(std::size_t row = 0) const
    {
        return at(row).size();
    }

    /**
       @brief 基于表格的resize
       @param row 行数
       @param col 列数
     */
    void resize(std::size_t row, std::size_t col)
    {
        super_class::resize(row);
        for (auto i = begin(); i != end(); ++i) {
            i->resize(col);
        }
    }
    /**
       @brief 基于表格的resize
       @param sh
     */
    void resize(table_index_type sh)
    {
        resize(sh.first, sh.second);
    }
};
}
#endif  // DA_VECTOR_TABLE_H
