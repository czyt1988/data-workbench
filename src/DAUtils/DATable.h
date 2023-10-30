#ifndef DATABLE_H
#define DATABLE_H
// Qt
#include <QDebug>
// std
#include <memory>
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
    using value_type         = T;  //兼容std
    using IndexType          = std::pair< int, int >;
    using TableType          = DAHashTable< T, int >;
    using TableIterator      = typename DAHashTable< T, int >::iterator;
    using TableConstIterator = typename DAHashTable< T, int >::const_iterator;

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
    /**
     * @brief 获取表的shape，first：最大行，second：最大列
     * @return
     */
    IndexType getShape() const
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
    const T& at(const IndexType& i) const
    {
        return mData.at(i);
    }
    T& at(const IndexType& i)
    {
        return mData.at(i);
    }
    T cell(int r, int c) const
    {
        return at(r, c);
    }
    T cell(const IndexType& i) const
    {
        return at(i);
    }
    T& operator[](const IndexType& i)
    {
        if (i.first > mShape.first) {
            mShape.first = i.first;
        }
        if (i.second > mShape.second) {
            mShape.second = i.second;
        }
        return mData[ i ];
    }
    const T& operator[](const IndexType& i) const
    {
        return mData[ i ];
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
            //判断是否触发reshape
            if (r == mShape.first || c == mShape.second) {
                //但凡有一边接触到边界，在删除后都有重新计算边界
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
    bool removeCell(const IndexType& i)
    {
        return removeCell(i.first, i.second);
    }

    /**
     * @brief 清空表格
     */
    void clear()
    {
        mData.clear();
        mShape = IndexType(0, 0);
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
    TableConstIterator find(const IndexType& i) const
    {
        return mData.find(i);
    }
    TableConstIterator find(int r, int c) const
    {
        return find(IndexType(r, c));
    }
    TableIterator find(const IndexType& i)
    {
        return mData.find(i);
    }
    TableIterator find(int r, int c)
    {
        return find(IndexType(r, c));
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

private:
    DAHashTable< T, int > mData;
    IndexType mShape;
};

}  // end DA

#endif  // DATABLE_H
