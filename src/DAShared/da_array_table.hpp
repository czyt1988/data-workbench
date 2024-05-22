#ifndef DA_ARRAY_TABLE_H
#define DA_ARRAY_TABLE_H
#include <array>
namespace DA
{

template< typename T, std::size_t RowN, std::size_t ColN >
class da_array_table : public std::array< std::array< T, ColN >, RowN >
{
public:
    using column_type      = std::array< T, ColN >;                      ///< 列类型
    using table_index_type = std::pair< std::size_t, std::size_t >;      ///< 索引类型
    using super_class      = std::array< std::array< T, ColN >, RowN >;  ///< 父类
public:
    da_array_table() : super_class()
    {
    }
    /**
       @brief 表格形状
       @return
     */
    table_index_type shape() const
    {
        return table_index_type(RowN, ColN);
    }

    /**
       @brief 行数
       @return
     */
    std::size_t row_count() const
    {
        return RowN;
    }

    /**
       @brief 列数
       @return
     */
    std::size_t column_count() const
    {
        return ColN;
    }
};

}
#endif  // DA_ARRAY_TABLE_H
