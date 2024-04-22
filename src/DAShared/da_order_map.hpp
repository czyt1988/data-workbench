#ifndef DA_ORDER_MAP_H
#define DA_ORDER_MAP_H
#include <vector>
#ifdef QT_VERSION //说明是qt应用
#include <QList>
#include <QMap>
#endif

/**
 * @brief 一种简化的map，通过两个连续容器来保存key和value，而不是用二叉树，对于少量的map数据有更好的性能
 *
 * CONTAINTER_VALUE必须是Qt容器类
 */
template<typename KEY, typename VALUE, typename CONTAINTER_KEY = QList<KEY>, typename CONTAINTER_VALUE = QList<VALUE> >
class da_order_map
{
public:
    da_order_map();
    ~da_order_map();
    typedef KEY								key_type;
    typedef VALUE								value_type;
    typedef da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>	_Myt;///< 学stl

    //等同QMap::clear
    void clear();

    //等同QMap::contains
    bool contains(const KEY& key) const;

    //等同QMap::count
    int count(const KEY& key) const;

    //等同QMap::count
    int count() const;

    //等同QMap::empty
    bool empty() const;

    //等同QMap::first
    VALUE& first();

    //等同QMap::first
    const VALUE& first() const;

    //等同QMap::firstKey
    const KEY& firstKey() const;

    //等同QMap::isEmpty
    bool isEmpty() const;

    //等同QMap::size
    int size() const;

    //等同QMap::operator[]
    VALUE& operator[](const KEY& key);

    class const_iterator;

    /**
     * @brief 迭代器
     */
    class iterator
    {
        friend class const_iterator;
        friend class da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>;
        typename CONTAINTER_KEY::iterator mIteKey;
        typename CONTAINTER_VALUE::iterator mIteValue;
public:
        typedef std::bidirectional_iterator_tag		iterator_category;
        typedef qptrdiff				difference_type;
        typedef VALUE					value_type;
        typedef VALUE *					pointer;
        typedef VALUE&					reference;
        inline iterator()
        {
        }


        inline iterator(typename CONTAINTER_KEY::iterator ki, typename CONTAINTER_VALUE::iterator vi)
            : mIteKey(ki), mIteValue(vi)
        {
        }


        inline const KEY& key() const
        {
            return (*mIteKey);
        }


        inline VALUE& value() const
        {
            return (*mIteValue);
        }


        inline VALUE& operator*() const
        {
            return (*mIteValue);
        }


        inline VALUE *operator->() const
        {
            return (&(*mIteValue));
        }


        inline bool operator==(const iterator& o) const
        {
            return ((mIteKey == o.mIteKey) && (mIteValue == o.mIteValue));
        }


        inline bool operator!=(const iterator& o) const
        {
            return ((mIteKey != o.mIteKey) || (mIteValue != o.mIteValue));
        }


        /**
         * @brief ++i
         * @return
         */
        inline iterator& operator++()
        {
            mIteKey = (++mIteKey);
            mIteValue = (++mIteValue);
            return (*this);
        }


        /**
         * @brief i++
         * @return
         */
        inline iterator operator++(int)
        {
            iterator r = *this;

            mIteKey = (++mIteKey);
            mIteValue = (++mIteValue);
            return (r);
        }


        inline iterator& operator--()
        {
            mIteKey = (--mIteKey);
            mIteValue = (--mIteValue);
            return (*this);
        }


        inline iterator operator--(int)
        {
            iterator r = *this;

            mIteKey = (--mIteKey);
            mIteValue = (--mIteValue);
            return (r);
        }


        inline iterator operator+(int j) const
        {
            iterator r = *this;

            if (j > 0) {
                while (j--)
                {
                    ++r;
                }
            }else{
                while (j++)
                {
                    --r;
                }
            }
            return (r);
        }


        inline iterator operator-(int j) const
        {
            return (operator+(-j));
        }


        inline iterator& operator+=(int j)
        {
            return (*this = *this + j);
        }


        inline iterator& operator-=(int j)
        {
            return (*this = *this - j);
        }


        inline bool operator==(const const_iterator& o) const
        {
            return ((mIteKey == o.mIteKey) && (mIteValue == o.mIteValue));
        }


        inline bool operator!=(const const_iterator& o) const
        {
            return ((mIteKey != o.mIteKey) || (mIteValue != o.mIteValue));
        }
    };
    friend class iterator;
    class const_iterator
    {
        friend class iterator;
        friend class da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>;
        typename CONTAINTER_KEY::const_iterator mIteKey;
        typename CONTAINTER_VALUE::const_iterator mIteValue;
public:
        typedef std::bidirectional_iterator_tag		iterator_category;
        typedef qptrdiff				difference_type;
        typedef VALUE					value_type;
        typedef const VALUE *				pointer;
        typedef const VALUE&				reference;

        inline const_iterator()
        {
        }


        inline const_iterator(typename CONTAINTER_KEY::const_iterator ki, typename CONTAINTER_VALUE::const_iterator vi)
            : mIteKey(ki), mIteValue(vi)
        {
        }


        inline const_iterator(const iterator& o)
        {
            mIteKey = o.mIteKey;
            mIteValue = o.mIteValue;
        }


        inline const KEY& key() const
        {
            return (*mIteKey);
        }


        inline const VALUE& value() const
        {
            return (*mIteValue);
        }


        inline const VALUE& operator*() const
        {
            return (*mIteValue);
        }


        inline const VALUE *operator->() const
        {
            return (&(*mIteValue));
        }


        inline bool operator==(const const_iterator& o) const
        {
            return ((mIteKey == o.mIteKey) && (mIteValue == o.mIteValue));
        }


        inline bool operator!=(const const_iterator& o) const
        {
            return ((mIteKey != o.mIteKey) || (mIteValue != o.mIteValue));
        }


        /**
         * @brief ++i
         * @return
         */
        inline const_iterator& operator++()
        {
            mIteKey = (++mIteKey);
            mIteValue = (++mIteValue);
            return (*this);
        }


        /**
         * @brief i++
         * @return
         */
        inline const_iterator operator++(int)
        {
            const_iterator r = *this;

            mIteKey = (++mIteKey);
            mIteValue = (++mIteValue);
            return (r);
        }


        inline const_iterator& operator--()
        {
            mIteKey = (--mIteKey);
            mIteValue = (--mIteValue);
            return (*this);
        }


        inline const_iterator operator--(int)
        {
            const_iterator r = *this;

            mIteKey = (--mIteKey);
            mIteValue = (--mIteValue);
            return (r);
        }


        inline const_iterator operator+(int j) const
        {
            const_iterator r = *this;

            if (j > 0) {
                while (j--)
                {
                    ++r;
                }
            } else{
                while (j++)
                {
                    --r;
                }
            }
            return (r);
        }


        inline const_iterator operator-(int j) const
        {
            return (operator+(-j));
        }


        inline const_iterator& operator+=(int j)
        {
            return (*this = *this + j);
        }


        inline const_iterator& operator-=(int j)
        {
            return (*this = *this - j);
        }
    };

    // STL style
    inline iterator begin() { return (iterator(mKeys.begin(), mValues.begin())); }
    inline const_iterator begin() const { return (const_iterator(mKeys.begin(), mValues.begin())); }
    inline const_iterator constBegin() const { return (const_iterator(mKeys.begin(), mValues.begin())); }
    inline const_iterator cbegin() const { return (const_iterator(mKeys.begin(), mValues.begin())); }
    inline iterator end() { return (iterator(mKeys.end(), mValues.end())); }
    inline const_iterator end() const { return (const_iterator(mKeys.end(), mValues.end())); }
    inline const_iterator constEnd() const { return (const_iterator(mKeys.end(), mValues.end())); }
    inline const_iterator cend() const { return (const_iterator(mKeys.end(), mValues.end())); }

    iterator erase(iterator it);

    iterator find(const KEY& key);
    const_iterator find(const KEY& key) const;

    inline VALUE& orderValue(int order) { return (mValues[order]); }
    inline const VALUE& orderValue(int order) const { return (mValues[order]); }
    inline KEY& orderKey(int order) { return (mKeys[order]); }
    inline const KEY& orderKey(int order) const { return (mKeys[order]); }
    //遍历k,v
    const std::pair<KEY, VALUE> orderPair(int order) const { return (std::make_pair<KEY, VALUE>(orderKey(order), orderValue(order))); }

    const VALUE value(const KEY& key, const VALUE& defaultValue = VALUE()) const;
#ifdef QT_VERSION
    //转换为qmap
    QMap<KEY, VALUE> toMap() const;
#endif
private:
    CONTAINTER_KEY mKeys;
    CONTAINTER_VALUE mValues;
};

template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::da_order_map()
{
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::~da_order_map()
{
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
void da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::clear()
{
    mKeys.clear();
    mValues.clear();
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
bool da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::contains(const KEY& key) const
{
    return (mKeys.contains(key));
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
int da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::count(const KEY& key) const
{
    return (mKeys.count(key));
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
int da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::count() const
{
    return (size());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
bool da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::empty() const
{
    return (mKeys.empty());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
VALUE& da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::first()
{
    return (mValues.first());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
const VALUE& da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::first() const
{
    return (mValues.first());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
const KEY& da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::firstKey() const
{
    return (mKeys.firstKey());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
bool da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::isEmpty() const
{
    return (mKeys.isEmpty());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
int da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::size() const
{
    return (mKeys.size());
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
VALUE& da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::operator[](const KEY& key)
{
    int index = mKeys.indexOf(key);

    if (index < 0) {
        mKeys.append(key);
        mValues.append(VALUE());
        return (mValues.last());
    }
    return (mValues[index]);
}


#include <memory>
template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
typename da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::iterator
da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::erase(iterator it)
{
    typename CONTAINTER_KEY::iterator ki = mKeys.erase(it.mIteKey);
    typename CONTAINTER_VALUE::iterator vi = mValues.erase(it.mIteValue);
    return (iterator(ki, vi));
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
typename da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::iterator
da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::find(const KEY& key)
{
    iterator i = begin();
    const_iterator e = end();

    while (i != e)
    {
        if (i.key() == key) {
            return (i);
        }
        ++i;
    }
    return (e);
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
typename da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::const_iterator
da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::find(const KEY& key) const
{
    const_iterator i = begin();
    const_iterator e = end();

    while (i != e)
    {
        if (i.key() == key) {
            return (i);
        }
        ++i;
    }
    return (e);
}


template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
const VALUE da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::value(const KEY& key, const VALUE& defaultValue) const
{
    const_iterator i = this->find(key);

    return ((i == this->end()) ? defaultValue : i.value());
}

#ifdef QT_VERSION
template<typename KEY, typename VALUE, typename CONTAINTER_KEY, typename CONTAINTER_VALUE>
QMap<KEY, VALUE> da_order_map<KEY, VALUE, CONTAINTER_KEY, CONTAINTER_VALUE>::toMap() const
{
    QMap<KEY, VALUE> r;
    int s = size();

    for (int i = 0; i < s; ++i)
    {
        r.insert(orderKey(i), orderValue(i));
    }
    return (r);
}
#endif

#endif // SASHORTMAP_H
