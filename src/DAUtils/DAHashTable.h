#ifndef DAHASHTABLE_H
#define DAHASHTABLE_H
#include <unordered_map>
namespace DA
{

struct pair_hash
{
    template< class T1, class T2 >
    std::size_t operator()(const std::pair< T1, T2 >& p) const
    {
        auto h1 = std::hash< T1 > {}(p.first);
        auto h2 = std::hash< T2 > {}(p.second);
        return h1 ^ h2;
    }
};

/**
 * @brief 基于std::unordered_map组成的松散table
 * key为std::pair< row_index_type, col_index_type >
 *
 * @code
 * DAHashTable table<double>;// 行列由std::size_t组成索引，值为double的table
 * DAHashTable table<double,int>;// 行列由int组成索引，值为double的table
 * DAHashTable table<double,std::size_t,std::string>;// 行由std::size_t组成索引,列由std::string组成索引（类似excel），值为double的table
 * @endcode
 */
template< typename T, typename row_index_type = std::size_t, typename col_index_type = row_index_type, typename hasher = pair_hash >
class DAHashTable : public std::unordered_map< std::pair< row_index_type, col_index_type >, T, hasher >
{
public:
    using table_index_type = std::pair< row_index_type, col_index_type >;

public:
    DAHashTable() : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >()
    {
    }
    DAHashTable(const DAHashTable& other)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(other)
    {
    }
    DAHashTable(DAHashTable&& other)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(other)
    {
    }
    DAHashTable(const std::initializer_list< T >& init)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(init)
    {
    }
    T& at(row_index_type r, col_index_type c)
    {
        return at(std::make_pair(r, c));
    }
    const T& at(row_index_type r, col_index_type c) const
    {
        return at(std::make_pair(r, c));
    }
    /**
     * @brief 计算表格的shape
     *
     * @note 此操作会遍历所有节点，时间复杂度为O(n)
     * @return
     */
    table_index_type shape() const
    {
        auto i = cbegin();
        if (i == cend()) {
            return table_index_type();
        }
        row_index_type minr = i->first.first;
        col_index_type minc = i->first.second;
        ++i;
        while (i != cend()) {
            if (i->first.first > minr) {
                minr = i->first.first;
            }
            if (i->first.second > minc) {
                minc = i->first.second;
            }
        }
        return table_index_type(minr, minc);
    }
};

}
#endif  // DAHASHTABLE_H
