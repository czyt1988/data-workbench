#ifndef DATABLE_H
#define DATABLE_H
// Qt
#include <QDebug>
// std
#include <memory>
#include <functional>
// DA
#include "da_algorithm.hpp"
#include "da_hash_table.hpp"
#include "da_vector_table.hpp"
#include "da_array_table.hpp"
namespace DA
{

/**
 * @brief 是一个支持稀疏和各种算法的表
 *
 * 其内部使用da_hash_table，但封装了shape，让获取尺寸更方便
 */
template< typename T >
class DATable
{
public:
    using Type               = T;
    using IndexType          = int;
    using IndexPair          = std::pair< IndexType, IndexType >;
    using TableType          = da_hash_table< T, IndexType >;
    using TableIterator      = typename TableType::iterator;
    using TableConstIterator = typename TableType::const_iterator;
    using value_type         = typename TableType::value_type;  // 兼容std
    using PredFun            = std::function< bool(const value_type&) >;

public:
    DATable() : mShape(0, 0)
    {
    }
    DATable(const DATable& other)
    {
        mData  = other.mData;
        mShape = other.mShape;
    }
    DATable(DATable&& other)
    {
        mData  = std::move(other.mData);
        mShape = std::move(other.mShape);
    }
    DATable< T >& operator=(const DATable& other)
    {
        mData  = other.mData;
        mShape = other.mShape;
        return *this;
    }
    DATable< T >& operator=(DATable&& other)
    {
        mData  = std::move(other.mData);
        mShape = std::move(other.mShape);
        return *this;
    }
    /**
     * @brief 单元格是否有内容
     * @param r
     * @param c
     * @return
     */
    bool contain(int r, int c) const
    {
        return (mData.cend() != mData.find({ r, c }));
    }
    bool contain(IndexPair i) const
    {
        return (mData.cend() != mData.find(i));
    }
    /**
     * @brief 获取表的shape，first：最大行，second：最大列
     * @return
     */
    IndexPair getShape() const
    {
        return mShape;
    }
    const T& at(int r, int c) const
    {
        return mData.at(r, c);
    }
    T& at(int r, int c)
    {
        return mData.at(r, c);
    }
    const T& at(const IndexPair& i) const
    {
        return mData.at(i);
    }
    T& at(const IndexPair& i)
    {
        return mData.at(i);
    }
    /**
     * @brief cell函数返回的是值，如果单元格没有，也返回一个默认构造的值
     * @param r
     * @param c
     * @return
     */
    T cell(int r, int c) const
    {
        return mData.value(r, c);
    }
    T cell(const IndexPair& i) const
    {
        return mData.value(i);
    }
    T& operator[](const IndexPair& i)
    {
        reflashShape(i);
        return mData[ i ];
    }
    const T& operator[](const IndexPair& i) const
    {
        return mData[ i ];
    }
    /**
     * @brief 插入内容
     * @param k
     * @param v
     */
    void set(const IndexPair& k, const T& v)
    {
        mData[ k ] = v;
        reflashShape(k);
    }
    void set(IndexType row, IndexType col, const T& v)
    {
        set(IndexPair(row, col), v);
    }

    /**
     * @brief 表的最大行数
     * @return
     */
    int rowCount() const
    {
        return mShape.first;
    }

    /**
     * @brief 表的最大列数
     * @return
     */
    int columnCount() const
    {
        return mShape.second;
    }

    /**
       @brief 获取表格的形状
       @return
     */
    IndexPair shape() const
    {
        return mShape;
    }

    /**
     * @brief 重新计算shape
     */
    void recalcShape()
    {
        mShape = mData.shape();
    }

    /**
     * @brief 表格单元的数量
     * @return
     */
    std::size_t size() const
    {
        return mData.size();
    }

    /**
     * @brief 移除一个cell
     * @param r
     * @param c
     * @return 成功移除返回true
     */
    bool removeCell(int r, int c)
    {
        auto i = mData.find({ r, c });
        if (i != mData.end()) {
            mData.erase(i);
            // 判断是否触发reshape
            if (r == (mShape.first - 1) || c == (mShape.second - 1)) {
                // 但凡有一边接触到边界，在删除后都有重新计算边界
                recalcShape();
            }
            return true;
        }
        return false;
    }
    /**
     * @brief 移除一个cell
     * @param i 索引
     * @return
     */
    bool removeCell(const IndexPair& i)
    {
        return removeCell(i.first, i.second);
    }

    /**
     * @brief 清空表格
     */
    void clear()
    {
        mData.clear();
        mShape = IndexPair(0, 0);
    }
    /**
     * @brief 获取内部的table
     * @note 此函数不安全,写操作完成后应该调用recalcShape
     * @return
     */
    TableType& rawData()
    {
        return mData;
    }
    const TableType& rawData() const
    {
        return mData;
    }
    /**
     * @brief 查找索引
     * @param i
     * @return
     */
    TableConstIterator find(const IndexPair& i) const
    {
        return mData.find(i);
    }
    TableConstIterator find(int r, int c) const
    {
        return find(IndexPair(r, c));
    }
    TableIterator find(const IndexPair& i)
    {
        return mData.find(i);
    }
    TableIterator find(int r, int c)
    {
        return find(IndexPair(r, c));
    }
    TableConstIterator end() const
    {
        return mData.end();
    }
    TableIterator end()
    {
        return mData.end();
    }
    TableConstIterator begin() const
    {
        return mData.begin();
    }
    TableIterator begin()
    {
        return mData.begin();
    }
    bool empty() const
    {
        return mData.empty();
    }
    /**
     * @brief 按条件删除元素
     * @param pred
     * @return 返回元素的个数
     */
    std::size_t erase_if(PredFun pred)
    {
        std::size_t r = erase_if__(pred);
        recalcShape();
        return r;
    }
    /**
     * @brief 移除一列，所有大于此索引的列向左移动
     * @param col
     */
    void dropColumn(IndexType col)
    {
        // 分两步，第一步删除，第二部移动
        // 先把列号等于col的移除
        std::vector< value_type > temp;
        std::ignore = erase_if__([ col, &temp ](const value_type& v) -> bool {
            if (v.first.second == col) {
                return true;
            } else if (v.first.second > col) {
                // 大于这个列的也要删除，但要把值缓存起来，并进行左移动
                temp.emplace_back(v);
                return true;
            }
            return false;
        });
        // 再把列号大于col的全部减去1
        std::for_each(temp.begin(), temp.end(), [ this ](value_type& v) {
            IndexPair k = v.first;
            --k.second;
            mData[ k ] = v.second;
        });
        recalcShape();
    }

    /**
     * @brief 按照trFun遍历所有元素，并返回一个新的table
     *
     * 这个函数一般做表格的格式转换用，例如吧DATable<double>转换为DATable<int>
     *
     * @code
     * DATable<double> doubleTable;
     * ...
     * DATable<int> intTable = doubleTable.transfered([](const double& v)->int{return v;});
     * @endcode
     * @param trFun
     * @return
     */
    template< typename OtherType >
    DATable< OtherType > transfered(std::function< OtherType(const T& v) > trFun) const
    {
        DATable< OtherType > other;
        for (auto i = mData.cbegin(), last = mData.cend(); i != last; ++i) {
            other[ i.first ] = trFun(*i);
        }
        return other;
    }

	/**
	 * @brief 按照trFun遍历一列
	 * @param trFun 如果返回true,继续迭代，如果返回false，就退出迭代
	 * @return
	 */
	void transferColumn(IndexType col, std::function< bool(const T& v) > trFun) const
	{
		auto rowCnt = rowCount();
		for (auto r = 0;r<rowCnt;++r) {
			auto ite = mData.find(IndexPair(r,col));
			if(ite!=mData.end()){
				if(!trFun(ite->second)){
					return;
				}
			}
		}
	}

    /**
     * @brief operator =
     * @param other
     * @return
     */
    template< typename OtherType >
    DATable< T >& operator=(const da_vector_table< OtherType >& other)
    {
        clear();
        auto rc = other.row_count();
        auto cc = other.column_count();
        for (auto r = 0; r < rc; ++r) {
            for (auto c = 0; c < cc; ++c) {
                set(r, c, other[ r ][ c ]);
            }
        }
        return *this;
    }

private:
    void reflashShape(const IndexPair& k)
    {
        if (k.first >= mShape.first) {
            mShape.first = k.first + 1;
        }
        if (k.second >= mShape.second) {
            mShape.second = k.second + 1;
        }
    }
    std::size_t erase_if__(PredFun pred)
    {
        auto old_size = size();
        for (auto i = mData.begin(), last = mData.end(); i != last;) {
            if (pred(*i)) {
                i = mData.erase(i);
            } else {
                ++i;
            }
        }
        return old_size - size();
    }

private:
    TableType mData;
    IndexPair mShape;
};

/**
   @brief 实现DATable向da_vector_table的转换
   @param table
   @param tr_fun 转换函数指针，如果不指定，默认为nullptr，直接进行赋值转换
   @example 下面演示把一个DATable< QVariant >转换为da_vector_table< qreal >：
   @code
   DA::DATable< QVariant > table;
   ...
   DA::da_vector_table< qreal > res =
            DA::table_transfered< qreal, QVariant >(table,[](const QVariant& v) -> qreal { return v.toDouble(); });
   @return 转换后的结果
 */
template< typename T1, typename T2 >
da_vector_table< T1 > table_transfered(const DATable< T2 >& table, std::function< T1(const T2&) > tr_fun)
{
    da_vector_table< T1 > res;
    const typename DATable< T2 >::IndexPair sh = table.shape();
    res.resize(sh.first, sh.second);
    for (auto c = table.begin(); c != table.end(); ++c) {
        res[ c->first.first ][ c->first.second ] = tr_fun(c->second);
    }
    return res;
}
template< typename T1, typename T2 >
da_vector_table< T1 > table_transfered(const DATable< T2 >& table)
{
    da_vector_table< T1 > res;
    const typename DATable< T2 >::IndexPair sh = table.shape();
    res.resize(sh.first, sh.second);
    for (auto c = table.begin(); c != table.end(); ++c) {
        res[ c->first.first ][ c->first.second ] = c->second;
    }
    return res;
}

/**
   @brief 实现da_vector_table向DATable的转换
   @param table
   @param tr_fun 转换函数指针，如果不指定，默认为nullptr，直接进行赋值转换
   @example 下面演示把一个da_vector_table< qreal >转换为DATable< QVariant >：
   @code
   DA::da_vector_table< qreal > table;
   ...
   DA::DATable< QVariant > res = DA::table_transfered< QVariant, qreal >(table);
   @endcode
   @return
 */
template< typename T1, typename T2 >
DATable< T1 > table_transfered(const da_vector_table< T2 >& table, std::function< T1(const T2&) > tr_fun)
{
    DATable< T1 > res;
    auto r_cnt = table.row_count();
    for (std::size_t r = 0; r < r_cnt; ++r) {
        for (std::size_t c = 0; c < table[ r ].size(); ++c) {
            res[ { r, c } ] = tr_fun(table[ r ][ c ]);
        }
    }
    return res;
}
template< typename T1, typename T2 >
DATable< T1 > table_transfered(const da_vector_table< T2 >& table)
{
    DATable< T1 > res;
    std::size_t r_cnt = table.row_count();
    for (std::size_t r = 0; r < r_cnt; ++r) {
        for (std::size_t c = 0; c < table[ r ].size(); ++c) {
            res[ { r, c } ] = table[ r ][ c ];
        }
    }
    return res;
}

}  // end DA

template< typename T >
QDebug operator<<(QDebug debug, const DA::DATable< T >& t)
{
    QDebugStateSaver saver(debug);
    int rs = t.rowCount();
    int cs = t.columnCount();
    for (int r = 0; r < rs; ++r) {
        if (r != 0) {
            debug << "\n";
        }
        for (int c = 0; c < cs; ++c) {
            if (c != 0) {
                debug << "\t";
            }
            if (t.contain(r, c)) {
                debug.noquote() << t.at(r, c);
            }
        }
    }
    return (debug);
}


#endif  // DATABLE_H
