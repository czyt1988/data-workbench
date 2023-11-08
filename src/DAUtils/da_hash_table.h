#ifndef DA_HASH_TABLE_H
#define DA_HASH_TABLE_H
#include <unordered_map>
#include "DAUtilsAPI.h"
namespace DA
{

struct DAUTILS_API pair_hash
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
 * @brief 鍩轰簬std::unordered_map缁勬垚鐨勬澗鏁able
 * key涓簊td::pair< row_index_type, col_index_type >
 *
 * @code
 * DAHashTable table<double>;// 琛屽垪鐢眘td::size_t缁勬垚绱㈠紩锛屽€间负double鐨則able
 * DAHashTable table<double,int>;// 琛屽垪鐢眎nt缁勬垚绱㈠紩锛屽€间负double鐨則able
 * DAHashTable table<double,std::size_t,std::string>;// 琛岀敱std::size_t缁勬垚绱㈠紩,鍒楃敱std::string缁勬垚绱㈠紩锛堢被浼糴xcel锛夛紝鍊间负double鐨則able
 * @endcode
 */
template< typename T, typename row_index_type = std::size_t, typename col_index_type = row_index_type, typename hasher = pair_hash >
class da_hash_table : public std::unordered_map< std::pair< row_index_type, col_index_type >, T, hasher >
{
public:
    using table_index_type = std::pair< row_index_type, col_index_type >;
    using super_class      = std::unordered_map< std::pair< row_index_type, col_index_type >, T, hasher >;

public:
    da_hash_table() : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >()
    {
    }
    da_hash_table(const da_hash_table& other)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(other)
    {
    }
    da_hash_table(da_hash_table&& other)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(other)
    {
    }
    da_hash_table(const std::initializer_list< T >& init)
        : std::unordered_map< std::pair< row_index_type, col_index_type >, T, pair_hash >(init)
    {
    }
    da_hash_table< T, row_index_type, col_index_type, hasher >& operator=(const da_hash_table< T, row_index_type, col_index_type, hasher >& other)
    {
        super_class::operator=(other);
        return *this;
    }
    da_hash_table< T, row_index_type, col_index_type, hasher >& operator=(da_hash_table< T, row_index_type, col_index_type, hasher >&& other)
    {
        super_class::operator=(other);
        return *this;
    }
    T& at(row_index_type r, col_index_type c)
    {
        return super_class::at(std::make_pair(r, c));
    }
    const T& at(row_index_type r, col_index_type c) const
    {
        return super_class::at(std::make_pair(r, c));
    }
    /**
     * @brief 计算最大的形状
     *
     * @note 此函数会遍历所有的元素，时间复杂度为O(n)
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
            ++i;
        }
        return table_index_type(minr + 1, minc + 1);
    }
};

}
#endif  // DA_HASH_TABLE_H
