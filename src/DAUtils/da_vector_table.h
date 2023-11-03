#ifndef DA_VECTOR_TABLE_H
#define DA_VECTOR_TABLE_H
#include <vector>
namespace DA
{
template< typename T >
class da_vector_table : public std::vector< std::vector< T > >
{
};
}
#endif  // DA_VECTOR_TABLE_H
