﻿#ifndef DATABLE_H
#define DATABLE_H
// Qt
#include <QDebug>
// std
#include <memory>
#include <functional>
// DA
#include "DAAlgorithm.h"
#include "DAHashTable.h"

namespace DA
{

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
    T cell(int r, int c) const
    {
        return at(r, c);
    }
    T cell(const IndexPair& i) const
    {
        return at(i);
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
            if (r == mShape.first || c == mShape.second) {
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
        auto old_size = size();
        for (auto i = mData.begin(), last = mData.end(); i != last;) {
            if (pred(*i)) {
                i = mData.erase(i);
            } else {
                ++i;
            }
        }
        recalcShape();
        return old_size - size();
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
        std::ignore = erase_if([ col, &temp ](const value_type& v) -> bool {
            if (v.first.second == col) {
                return true;
            } else if (v.first.second > col) {
                // 大于这个列的也要删除，但要缓存起来
                temp.emplace_back(v);
                return true;
            }
            return false;
        });
        // 再把列号大于col的全部减去1
        std::for_each(temp.begin(), temp.end(), [ this ](const value_type& v) {
            IndexPair k = v.first;
            k.second -= 1;
            this->operator[](k) = v.second;
        });
        recalcShape();
    }

    /**
     * @brief 按照trFun遍历所有元素，并返回一个新的table
     * @param trFun
     * @return
     */
    template< typename OtherType >
    DATable< OtherType > transfered(std::function< OtherType(const value_type& v) > trFun) const
    {
        DATable< OtherType > other;
        for (auto i = mData.cbegin(), last = mData.cend(); i != last; ++i) {
            other[ i.first ] = trFun(*i);
        }
        return other;
    }

private:
    void reflashShape(const IndexPair& k)
    {
        if (k.first > mShape.first) {
            mShape.first = k.first;
        }
        if (k.second > mShape.second) {
            mShape.second = k.second;
        }
    }

private:
    TableType mData;
    IndexPair mShape;
};

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
            debug.noquote() << t.at(r, c);
        }
    }
    return (debug);
}

#endif  // DATABLE_H
