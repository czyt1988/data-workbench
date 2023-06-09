#ifndef DATABLE_H
#define DATABLE_H
// Qt
#include <QVector>
#include <QHash>
#include <QDebug>
// std
#include <memory>
// SA
#include "DAAlgorithm.h"
namespace DA
{
//==============================================================
// DAVector
//==============================================================
/**
 * @brief 系列数据,系列允许设置名字
 */
template< typename T >
class DAVector : public QVector< T >
{
public:
    DAVector() : QVector< T >()
    {
    }

    DAVector(int size) : QVector< T >(size)
    {
    }

    DAVector(int size, const T& t) : QVector< T >(size, t)
    {
    }

    DAVector(std::initializer_list< T > args) : QVector< T >(args)
    {
    }

    DAVector(const QString& n) : QVector< T >(), mName(n)
    {
    }

    const QString& name() const;
    QString& name();
    void setName(const QString& n);
    QString getName() const;

private:
    QString mName;  ///< 系列名，参考pandas.Series.name
};

template< typename T >
const QString& DAVector< T >::name() const
{
    return (mName);
}

template< typename T >
QString& DAVector< T >::name()
{
    return (mName);
}

template< typename T >
void DAVector< T >::setName(const QString& n)
{
    mName = n;
}

template< typename T >
QString DAVector< T >::getName() const
{
    return (mName);
}
//==============================================================
// DARowTable
//==============================================================
/**
 * @brief 以行为基础的数据表
 *
 * 表格形如：
 *
 * _______________________________
 * |name| 0 | 1 | 2 | 3 | …… | n |
 * 行名字  -> 列索引
 * 表内容：
 * -------------------------------
 * |row1| 1 | 2 | 1 | 0 | …… | 6 |
 * |row2| 4 | 5 | 6 | 7 | …… | 8 |
 * ………………
 * |rown| 9 | 8 | 7 | 6 | …… | 0 |
 * -------------------------------
 *
 * @code
 * SARowTable<int> table;
 * table.setRowNames({"row1","row2",……,"rown"});
 * table.appendColumn({1,4,……,9});
 * table.appendColumn({2,5,……,8});
 * table.appendColumn({1,6,……,7});
 * table.appendColumn({0,7,……,6});
 * ……
 * table.appendColumn({6,8,……,0});
 * @endcode
 *
 * 可以通过行序号进行索引
 * table[1] //row2
 * 可以索引到单元格
 * table.cell(1,2) //row2[2]:6 等同table[1][2]
 * 可以通过行名索引到单元格
 * table["row2"][2] //row2[2]:6 等同table[1][2]
 */
template< typename T >
class DARowTable
{
public:
    enum Mode
    {
        FixedMode,  ///< 固定模式，表的列不会随着行的变化而变化(默认)
        ExpandMode  ///< 扩展模式，表的列是浮动的，如果插入一行比原来要宽，会自动扩充表的列数
    };
    enum CaseSensitivity
    {
        CaseInsensitive,  ///< 大小写不敏感
        CaseSensitive     ///< 大小写敏感
    };
    typedef T Type;
    typedef DAVector< T > SeriesType;
    typedef std::shared_ptr< SeriesType > SeriesPtr;
    typedef typename DAVector< T >::iterator SeriesIterator;
    typedef typename DAVector< T >::const_iterator SeriesConstIterator;
    typedef DARowTable< T > TableType;
    typedef std::shared_ptr< TableType > TablePtr;

    static SeriesPtr makeSeries();
    static SeriesPtr makeSeries(const QString& n);
    static SeriesPtr makeSeries(std::initializer_list< T > args);
    static TablePtr makeTable();

    DARowTable();
    DARowTable(int rows, int columns);
    void resize(int r, int c);
    void reserve(int size);
    //判断是否存在field
    bool haveFieldid(const QString& field) const;

    /**
     * @brief 填充元素
     * @param v
     */
    void fill(const T& v);
    const T& at(int r, int c) const;
    T& at(int r, int c);

    T cell(int r, int c) const;
    SeriesPtr appendRow(const QString& name);
    SeriesPtr appendRow(const QString& name, int size);
    void appendRow(SeriesPtr row);
    void appendRow(std::initializer_list< T > args, const QString& n);

    template< typename Ite1, typename Ite2 >
    void appendColumn(Ite1 b, Ite2 e);
    void appendColumn(std::initializer_list< T > args);
    int nameToIndex(const QString& n) const;
    SeriesPtr& row(int r);
    const SeriesPtr& row(int r) const;
    SeriesPtr& row(const QString& n);
    const SeriesPtr& row(const QString& n) const;

    //返回一列数据，返回一个SeriesPtr，这个seriesPtr的写操作不会影响table
    SeriesPtr colunm(int c) const;

    SeriesType& operator[](int r);
    const SeriesType& operator[](int r) const;
    SeriesType& operator[](const QString& rowname);
    const SeriesType& operator[](const QString& rowname) const;

    /**
     * @brief 以最大列数进行列数修正，保证所有行同列
     */
    void fixSize();

    /**
     * @brief 表的行数
     * @return
     */
    int rowCount() const;

    /**
     * @brief 表的列数
     * @return
     */
    int columnCount() const;

    /**
     * @brief 设置表格的模式
     * @param m 模式 @ref Mode
     */
    void setMode(Mode m);

    /**
     * @brief 获取模式
     * @return
     */
    Mode getMode() const;

    /**
     * @brief 清空
     */
    void clear();

    void setName(const QString& n);
    QString getName() const;

    /**
     * @brief rowNames
     * @return
     */
    QStringList rowNames() const;

    /**
     * @brief 设置行名，如果是个空的表会生成一个默认行
     * @param ns
     */
    void setRowNames(const QStringList& ns);

    /**
     * @brief 提取某个值等于value作为新表
     * @param field
     * @param value
     * @param cs
     * @return
     */
    TablePtr takeByValue(const QString& field, T value) const;

    /**
     * @brief groupby
     * @param field
     * @param cs
     * @return
     */
    QPair< QList< TablePtr >, QList< T > > groupBy(const QString& field) const;

    /**
     * @brief orderBy
     * @param sn
     */
    void orderBy(const QString& sn);
    void orderBy(int rindex);

    /**
     * @brief 查找第一个大于或等于某个元素的位置
     * @note 需要确保已经进行过排序
     * @param v
     * @return
     */
    QPair< T, int > lowerBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > lowerBound(const T& v, int r) const;
    QPair< T, int > upperBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > upperBound(const T& v, int r) const;

    /**
     * @brief 设置名字查询时是否对大小写敏感
     * @param cs
     */
    void setCaseSensitivity(CaseSensitivity cs);

    /**
     * @brief 判断是否大小写敏感
     * @return
     */
    bool isCaseSensitivity() const;
    //移除
    void remove(const QString& name);
    void remove(int rindex);

private:
    DAVector< SeriesPtr > m_d;
    size_t m_columns;
    Mode m_mode;
    SeriesPtr m_nullseries;
    CaseSensitivity m_caseSensitivity;
};

//==============================================================
// 全局函数
//==============================================================

template< typename T >
typename DARowTable< T >::TablePtr takeByValue(const DARowTable< T >& table, const QString& field, T value)
{
    const int r = table.nameToIndex(field);

    typename DARowTable< T >::TablePtr res = DARowTable< T >::makeTable();

    res->setName(table.getName());
    res->setRowNames(table.rowNames());
    int csize = table.columnCount();

    for (int i = 0; i < csize; ++i) {
        if (table.cell(r, i) == value) {
            typename DARowTable< T >::SeriesPtr col = table.colunm(i);
            res->appendColumn(col->begin(), col->end());
        }
    }
    return (res);
}

template< typename T >
typename DARowTable< T >::TablePtr takeByValue(const DARowTable< T >& table, int r, T value)
{
    typename DARowTable< T >::TablePtr res = DARowTable< T >::makeTable();

    res->setName(table.getName());
    res->setRowNames(table.rowNames());
    int csize = table.columnCount();

    for (int i = 0; i < csize; ++i) {
        if (table.cell(r, i) == value) {
            typename DARowTable< T >::SeriesPtr col = table.colunm(i);
            res->appendColumn(col->begin(), col->end());
        }
    }
    return (res);
}

/**
 * @brief groupby 对某个字段执行group by操作
 * @param table
 * @param field
 * @return 返回一个pair，first：group by后的结构表，second，group by的结果
 */
template< typename T >
QPair< QList< typename DARowTable< T >::TablePtr >, QList< T > > groupby(const DARowTable< T >& table, const QString& field)
{
    QList< typename DARowTable< T >::TablePtr > restables;
    QList< T > gr;

    int rindex = table.nameToIndex(field);

    Q_ASSERT_X(rindex >= 0, "groupby", "unknow field");
    typename DARowTable< T >::SeriesPtr r = table.row(rindex);
    if (r == nullptr) {
        return (qMakePair(restables, gr));
    }
    gr = r->toList().toSet().toList();
    std::sort(gr.begin(), gr.end());
    for (T v : gr) {
        restables.append(takeByValue(table, rindex, v));
    }
    return (qMakePair(restables, gr));
}

//==============================================================
// ValueWithIndex
//==============================================================

template< typename T >
class ValueWithIndex
{
public:
    ValueWithIndex() : index(-1)
    {
    }

    ValueWithIndex(const T& v, int i) : value(v), index(i)
    {
    }

    T value;
    int index;
};

template< typename T >
bool operator<(const ValueWithIndex< T >& a, const ValueWithIndex< T >& b)
{
    return (a.value < b.value);
}

/**
 * @brief 把序列转换为带序号的序列
 * @param p
 * @return 用于带序号的排序用
 */
template< typename T >
std::shared_ptr< DAVector< ValueWithIndex< T > > > makeIndexSeries(const typename DARowTable< T >::SeriesPtr& p)
{
    std::shared_ptr< DAVector< ValueWithIndex< T > > > res = std::make_shared< DAVector< ValueWithIndex< T > > >();

    res->setName(p->getName());
    const int s = p->size();

    res->reserve(s);
    for (int i = 0; i < s; ++i) {
        res->push_back(ValueWithIndex< T >(p->at(i), i));
    }
    return (res);
}

template< typename T >
void orderBy(DARowTable< T >& table, const QString& field)
{
    const int r = table.nameToIndex(field);

    orderBy(table, r);
    //    typename SARowTable<T>::SeriesPtr row = table.row(r);
    //    Q_ASSERT_X(row != nullptr, "orderBy", "unknow field");

    //    auto ordser = makeIndexSeries<T>(row);

    //    std::sort(ordser->begin(), ordser->end());
    //    int rowcount = table.rowCount();

    //    //开始逐一转换
    //    for (int rc = 0; rc < rowcount; ++rc)
    //    {
    //        typename SARowTable<T>::SeriesPtr series = table.row(rc);
    //        typename SARowTable<T>::SeriesPtr ns = SARowTable<T>::makeSeries(series->getName());
    //        ns->reserve(series->size());
    //        for (auto i = ordser->begin(); i != ordser->end(); ++i)
    //        {
    //            ns->push_back(series->at((*i).index));
    //        }
    //        table[rc].swap(ns);
    //    }
}

template< typename T >
void orderBy(DARowTable< T >& table, int r)
{
    typename DARowTable< T >::SeriesPtr row = table.row(r);
    Q_ASSERT_X(row != nullptr, "orderBy", "unknow field");
    auto ordser = makeIndexSeries< T >(row);

    std::sort(ordser->begin(), ordser->end());
    int rowcount = table.rowCount();

    //开始逐一转换
    for (int rc = 0; rc < rowcount; ++rc) {
        typename DARowTable< T >::SeriesPtr series = table.row(rc);
        typename DARowTable< T >::SeriesPtr ns     = DARowTable< T >::makeSeries(series->getName());
        ns->reserve(series->size());
        for (auto i = ordser->begin(); i != ordser->end(); ++i) {
            ns->push_back(series->at((*i).index));
        }
        table.row(rc).swap(ns);
    }
}

//==============================================================
// DARowTable
//==============================================================

template< typename T >
DARowTable< T >::DARowTable() : m_columns(0), m_mode(FixedMode), m_caseSensitivity(CaseInsensitive)
{
}

template< typename T >
DARowTable< T >::DARowTable(int rows, int columns) : m_mode(FixedMode), m_caseSensitivity(CaseInsensitive)
{
    m_d.clear();
    m_d.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        m_d.push_back(SeriesType(columns));
    }
    m_columns = columns;
}

/**
 * @brief 改变table 的大小
 * @param r
 * @param c
 */
template< typename T >
void DARowTable< T >::resize(int r, int c)
{
    m_d.resize(r);
    for (SeriesPtr& row : m_d) {
        if (row == nullptr) {
            row = makeSeries();
        }
        row->resize(c);
    }
    m_columns = c;
}

/**
 * @brief 判断是否存在field
 * @param field
 * @return
 */
template< typename T >
bool DARowTable< T >::haveFieldid(const QString& field) const
{
    int r                  = rowCount();
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int i = 0; i < r; ++i) {
        if (row(i)->name().compare(field, cs) == 0) {
            return (true);
        }
    }
    return (false);
}

template< typename T >
void DARowTable< T >::fill(const T& v)
{
    for (SeriesPtr r : m_d) {
        r->fill(v);
    }
}

template< typename T >
const T& DARowTable< T >::at(int r, int c) const
{
    return (m_d.at(r)->at(c));
}

template< typename T >
T& DARowTable< T >::at(int r, int c)
{
    return (m_d[ r ]->operator[](c));
}

/**
 * @brief 获取单元格
 * @param r
 * @param c
 * @return 如果没有或超范围，返回默认构造
 */
template< typename T >
T DARowTable< T >::cell(int r, int c) const
{
    if (r < m_d.size()) {
        const SeriesPtr& rr = row(r);
        if (c < rr->size()) {
            return (rr->at(c));
        }
    }
    return (T());
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::appendRow(const QString& name)
{
    return appendRow(name, columnCount());
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::appendRow(const QString& name, int size)
{
    SeriesPtr s = makeSeries(name);
    s->resize(size);
    appendRow(s);
    return s;
}

template< typename T >
void DARowTable< T >::appendRow(SeriesPtr row)
{
    size_t s = row->size();

    if ((s == m_columns) || (0 == m_columns)) {
        m_d.push_back(row);
        m_columns = s;
    } else if (s < m_columns) {  //在结尾补充
        row->resize(m_columns);
        m_d.push_back(row);
    } else {  // s>m_columns
        if (getMode() == ExpandMode) {
            m_d.push_back(row);
            fixSize();
        } else {
            //固定模式的插入
            row->resize(m_columns);
            m_d.push_back(row);
        }
    }
}

template< typename T >
void DARowTable< T >::appendRow(std::initializer_list< T > args, const QString& n)
{
    SeriesPtr r = makeSeries(args);

    r->setName(n);
    appendRow(r);
}

template< typename T >
template< typename Ite1, typename Ite2 >
void DARowTable< T >::appendColumn(Ite1 b, Ite2 e)
{
    const int rc = rowCount();
    auto it      = b;

    for (int i = 0; i < rc; ++i) {
        it = b + i;
        if (it < e) {
            row(i)->push_back(*it);
        } else {
            row(i)->push_back(T());
        }
    }
    ++m_columns;
}

template< typename T >
void DARowTable< T >::appendColumn(std::initializer_list< T > args)
{
    const int rc = rowCount();

    for (int i = 0; i < rc; ++i) {
        if (i < args.size()) {
            row(i)->push_back(args[ i ]);
        } else {
            row(i)->push_back(T());
        }
    }
    ++m_columns;
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::makeSeries(const QString& n)
{
    return (std::make_shared< SeriesType >(n));
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::makeSeries(std::initializer_list< T > args)
{
    return (std::make_shared< SeriesType >(args));
}

template< typename T >
typename DARowTable< T >::TablePtr DARowTable< T >::makeTable()
{
    return (std::make_shared< typename DARowTable< T >::TableType >());
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::makeSeries()
{
    return (std::make_shared< SeriesType >());
}

template< typename T >
int DARowTable< T >::nameToIndex(const QString& n) const
{
    int r                  = rowCount();
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int i = 0; i < r; ++i) {
        if (row(i)->name().compare(n, cs) == 0) {
            return (i);
        }
    }
    return (-1);
}

/**
 * @brief 获取行引用
 * @param r
 * @return
 */
template< typename T >
typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(int r)
{
    return (m_d[ r ]);
}

/**
 * @brief 获取行引用
 * @param r
 * @return
 */
template< typename T >
const typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(int r) const
{
    return (m_d[ r ]);
}

template< typename T >
typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(const QString& n)
{
    int r = nameToIndex(n);

    if ((r < 0) || (r > rowCount())) {
        return (m_nullseries);
    }
    return (row(r));
}

template< typename T >
const typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(const QString& n) const
{
    int r = nameToIndex(n);

    if ((r < 0) || (r > rowCount())) {
        return (m_nullseries);
    }
    return (row(r));
}

template< typename T >
typename DARowTable< T >::SeriesPtr DARowTable< T >::colunm(int c) const
{
    int rsize     = rowCount();
    SeriesPtr col = std::make_shared< SeriesType >(rsize);

    for (int r = 0; r < rsize; ++r) {
        col->operator[](r) = cell(r, c);
    }
    return (col);
}

template< typename T >
typename DARowTable< T >::SeriesType& DARowTable< T >::operator[](int r)
{
    return (*(row(r)));
}

template< typename T >
const typename DARowTable< T >::SeriesType& DARowTable< T >::operator[](int r) const
{
    return (*(row(r)));
}

template< typename T >
typename DARowTable< T >::SeriesType& DARowTable< T >::operator[](const QString& rowname)
{
    return (*(row(rowname)));
}

template< typename T >
const typename DARowTable< T >::SeriesType& DARowTable< T >::operator[](const QString& rowname) const
{
    return (*(row(rowname)));
}

template< typename T >
void DARowTable< T >::reserve(int size)
{
    for (SeriesPtr p : m_d) {
        p->reserve(size);
    }
}

template< typename T >
void DARowTable< T >::fixSize()
{
    std::vector< int > ss;

    for (const SeriesPtr& r : m_d) {
        ss.push_back(r->size());
    }
    int maxsize = *(std::max_element(ss.begin(), ss.end()));

    for (SeriesPtr& r : m_d) {
        if (r->size() < maxsize) {
            r->resize(maxsize);
        }
    }
    m_columns = maxsize;
}

template< typename T >
int DARowTable< T >::rowCount() const
{
    return (m_d.size());
}

template< typename T >
int DARowTable< T >::columnCount() const
{
    return (m_columns);
}

template< typename T >
void DARowTable< T >::setMode(typename DARowTable< T >::Mode m)
{
    m_mode = m;
}

template< typename T >
typename DARowTable< T >::Mode DARowTable< T >::getMode() const
{
    return (m_mode);
}

template< typename T >
void DARowTable< T >::clear()
{
    m_d.clear();
    m_columns = 0;
}

template< typename T >
void DARowTable< T >::setName(const QString& n)
{
    m_d.setName(n);
}

template< typename T >
QString DARowTable< T >::getName() const
{
    return (m_d.getName());
}

template< typename T >
QStringList DARowTable< T >::rowNames() const
{
    QStringList r;

    for (SeriesPtr p : m_d) {
        r.append(p->getName());
    }
    return (r);
}

template< typename T >
void DARowTable< T >::setRowNames(const QStringList& ns)
{
    const int s = ns.size();

    for (int i = 0; i < s; ++i) {
        if (i < rowCount()) {
            row(i)->setName(ns[ i ]);
        } else {
            SeriesPtr r = makeSeries(ns[ i ]);
            appendRow(r);
        }
    }
}

template< typename T >
typename DARowTable< T >::TablePtr DARowTable< T >::takeByValue(const QString& field, T value) const
{
    return (SA::takeByValue(*this, field, value));
}

template< typename T >
QPair< QList< typename DARowTable< T >::TablePtr >, QList< T > > DARowTable< T >::groupBy(const QString& field) const
{
    return (SA::groupby(*this, field));
}

template< typename T >
void DARowTable< T >::orderBy(const QString& sn)
{
    SA::orderBy(*this, sn);
}

template< typename T >
void DARowTable< T >::orderBy(int rindex)
{
    SA::orderBy(*this, rindex);
}

template< typename T >
QPair< T, int > DARowTable< T >::lowerBound(const T& v, const QString& sortedfield) const
{
    const int r = nameToIndex(sortedfield);

    return (lowerBound(v, r));
}

template< typename T >
QPair< T, int > DARowTable< T >::lowerBound(const T& v, int r) const
{
    typename DARowTable< T >::SeriesPtr prow = row(r);
    SeriesIterator ite                       = std::lower_bound(prow->begin(), prow->end(), v);

    if (ite == prow->end()) {
        return (qMakePair< T, int >(prow->back(), prow->size() - 1));
    }
    size_t dis = std::distance(prow->begin(), ite);

    return (qMakePair< T, int >(*ite, dis));
}

template< typename T >
QPair< T, int > DARowTable< T >::upperBound(const T& v, const QString& sortedfield) const
{
    const int r = nameToIndex(sortedfield);

    return (upperBound(v, r));
}

template< typename T >
QPair< T, int > DARowTable< T >::upperBound(const T& v, int r) const
{
    typename DARowTable< T >::SeriesPtr prow = row(r);
    SeriesIterator ite                       = std::upper_bound(prow->begin(), prow->end(), v);

    if (ite == prow->end()) {
        return (qMakePair< T, int >(prow->back(), prow->size() - 1));
    }
    size_t dis = std::distance(prow->begin(), ite);

    return (qMakePair< T, int >(*ite, dis));
}

template< typename T >
void DARowTable< T >::setCaseSensitivity(typename DARowTable< T >::CaseSensitivity cs)
{
    m_caseSensitivity = cs;
}

template< typename T >
bool DARowTable< T >::isCaseSensitivity() const
{
    return (m_caseSensitivity == CaseSensitive);
}

template< typename T >
void DARowTable< T >::remove(const QString& name)
{
    int r = nameToIndex(name);
    if (r < 0) {
        return;
    }
    remove(r);
}

template< typename T >
void DARowTable< T >::remove(int rindex)
{
    m_d.remove(rindex);
    if (0 == m_d.size()) {
        m_columns = 0;
    }
}

/**
 * @brief 列族表
 */
template< typename T >
class DAColumnTable
{
public:
    enum Mode
    {
        FixedMode,  ///< 固定模式，表的列不会随着行的变化而变化(默认)
        ExpandMode  ///< 扩展模式，表的列是浮动的，如果插入一行比原来要宽，会自动扩充表的列数
    };
    enum CaseSensitivity
    {
        CaseInsensitive,  ///< 大小写不敏感
        CaseSensitive     ///< 大小写敏感
    };
    typedef T Type;
    typedef DAColumnTable< T > TableType;
    typedef DAVector< T > SeriesType;
    typedef std::shared_ptr< SeriesType > SeriesPtr;
    typedef typename DAVector< T >::iterator SeriesIterator;
    typedef typename DAVector< T >::const_iterator SeriesConstIterator;
    typedef std::shared_ptr< TableType > TablePtr;

    static SeriesPtr makeSeries();
    static SeriesPtr makeSeries(const QString& n);
    static SeriesPtr makeSeries(std::initializer_list< T > args);
    static TablePtr makeTable();

    DAColumnTable();
    DAColumnTable(int rows, int columns);
    void resize(int r, int c);
    void reserve(int size);
    //判断是否存在field
    bool haveFieldid(const QString& field) const;
    /**
     * @brief 表的行数
     * @return
     */
    int rowCount() const;

    /**
     * @brief 表的列数
     * @return
     */
    int columnCount() const;
    /**
     * @brief 填充元素
     * @param v
     */
    void fill(const T& v);
    const T& at(int r, int c) const;
    T& at(int r, int c);
    T cell(int r, int c) const;
    int nameToIndex(const QString& n) const;
    SeriesPtr& column(int r);
    const SeriesPtr& column(int r) const;
    SeriesPtr& column(const QString& n);
    const SeriesPtr& column(const QString& n) const;
    //
    SeriesPtr appendColumn(const QString& name);
    SeriesPtr appendColumn(const QString& name, int size);
    void appendColumn(SeriesPtr col);
    void appendColumn(std::initializer_list< T > datas, const QString& colName);
    template< typename Ite1, typename Ite2 >
    void appendRow(Ite1 b, Ite2 e);
    void appendRow(std::initializer_list< T > rowDatas);
    //返回一列数据，返回一个SeriesPtr，这个seriesPtr的写操作不会影响table
    SeriesPtr row(int c) const;

    SeriesType& operator[](int c);
    const SeriesType& operator[](int c) const;
    SeriesType& operator[](const QString& colname);
    const SeriesType& operator[](const QString& colname) const;

    /**
     * @brief 以最大列数进行列数修正，保证所有行同列
     */
    void fixSize();
    /**
     * @brief 设置表格的模式
     * @param m 模式 @ref Mode
     */
    void setMode(Mode m);
    Mode getMode() const;

    /**
     * @brief 清空
     */
    void clear();

    void setName(const QString& n);
    QString getName() const;
    /**
     * @brief colNames
     * @return
     */
    QStringList columnNames() const;

private:
    DAVector< SeriesPtr > m_d;
    size_t m_rows;  ///< 记录行数
    Mode m_mode;
    SeriesPtr m_nullseries;
    CaseSensitivity m_caseSensitivity;
};

//==============================================================
// DAColumnTable
//==============================================================

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::makeSeries()
{
    return (std::make_shared< SeriesType >());
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::makeSeries(const QString& n)
{
    return (std::make_shared< SeriesType >(n));
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::makeSeries(std::initializer_list< T > args)
{
    return (std::make_shared< SeriesType >(args));
}

template< typename T >
typename DAColumnTable< T >::TablePtr DAColumnTable< T >::makeTable()
{
    return (std::make_shared< typename DAColumnTable< T >::TableType >());
}

template< typename T >
DAColumnTable< T >::DAColumnTable() : m_rows(0), m_mode(FixedMode), m_caseSensitivity(CaseInsensitive)
{
}

template< typename T >
DAColumnTable< T >::DAColumnTable(int rows, int columns) : m_mode(FixedMode), m_caseSensitivity(CaseInsensitive)
{
    m_d.clear();
    m_d.reserve(columns);
    for (int i = 0; i < columns; ++i) {
        m_d.push_back(SeriesType(rows));
    }
    m_rows = rows;
}

/**
 * @brief 改变table 的大小
 * @param r
 * @param c
 */
template< typename T >
void DAColumnTable< T >::resize(int r, int c)
{
    m_d.resize(c);
    for (SeriesPtr& col : m_d) {
        if (col == nullptr) {
            col = makeSeries();
        }
        col->resize(r);
    }
    m_rows = r;
}

template< typename T >
void DAColumnTable< T >::reserve(int size)
{
    for (SeriesPtr p : m_d) {
        p->reserve(size);
    }
}

template< typename T >
bool DAColumnTable< T >::haveFieldid(const QString& field) const
{
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (const SeriesPtr& col : qAsConst(m_d)) {
        if (col == nullptr) {
            continue;
        }
        if (col->name().compare(field, cs) == 0) {
            return true;
        }
    }
    return (false);
}

template< typename T >
int DAColumnTable< T >::rowCount() const
{
    return m_rows;
}

template< typename T >
int DAColumnTable< T >::columnCount() const
{
    return (m_d.size());
}

template< typename T >
void DAColumnTable< T >::fill(const T& v)
{
    for (SeriesPtr c : m_d) {
        c->fill(v);
    }
}

template< typename T >
const T& DAColumnTable< T >::at(int r, int c) const
{
    return (m_d.at(c)->at(r));
}

template< typename T >
T& DAColumnTable< T >::at(int r, int c)
{
    return (m_d[ c ]->operator[](r));
}

/**
 * @brief 获取单元格
 * @param r
 * @param c
 * @return 如果没有或超范围，返回默认构造
 */
template< typename T >
T DAColumnTable< T >::cell(int r, int c) const
{
    if (c < m_d.size()) {
        const SeriesPtr& cc = column(c);
        if (r < cc->size()) {
            return (cc->at(r));
        }
    }
    return (T());
}

template< typename T >
int DAColumnTable< T >::nameToIndex(const QString& n) const
{
    int c                  = columnCount();
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int i = 0; i < c; ++i) {
        if (column(i)->name().compare(n, cs) == 0) {
            return (i);
        }
    }
    return (-1);
}
/**
 * @brief 获取列引用
 * @param r
 * @return
 */
template< typename T >
DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c)
{
    return (m_d[ c ]);
}
/**
 * @brief 获取列引用
 * @param r
 * @return
 */
template< typename T >
const DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c) const
{
    return (m_d[ c ]);
}

template< typename T >
DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n)
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (m_nullseries);
    }
    return (column(r));
}

template< typename T >
const DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n) const
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (m_nullseries);
    }
    return (column(r));
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::appendColumn(const QString& name)
{
    return appendColumn(name, m_rows);
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::appendColumn(const QString& name, int size)
{
    SeriesPtr s = makeSeries(name);
    s->resize(size);
    appendColumn(s);
    return s;
}

template< typename T >
void DAColumnTable< T >::appendColumn(DAColumnTable< T >::SeriesPtr col)
{
    size_t s = col->size();

    if ((s == m_rows) || (0 == m_rows)) {
        m_d.push_back(col);
        m_rows = s;
    } else if (s < m_rows) {  //在结尾补充
        col->resize(m_rows);
        m_d.push_back(col);
    } else {  // s>m_columns
        if (getMode() == ExpandMode) {
            m_d.push_back(col);
            fixSize();
        } else {
            //固定模式的插入
            col->resize(m_rows);
            m_d.push_back(col);
        }
    }
}

template< typename T >
void DAColumnTable< T >::appendColumn(std::initializer_list< T > datas, const QString& colName)
{
    SeriesPtr c = makeSeries(datas);

    c->setName(n);
    appendColumn(c);
}

template< typename T >
template< typename Ite1, typename Ite2 >
void DAColumnTable< T >::appendRow(Ite1 b, Ite2 e)
{
    const int cc = columnCount();
    auto it      = b;

    for (int i = 0; i < cc; ++i) {
        it = b + i;
        if (it < e) {
            column(i)->push_back(*it);
        } else {
            column(i)->push_back(T());
        }
    }
    ++m_rows;
}

template< typename T >
void DAColumnTable< T >::appendRow(std::initializer_list< T > rowDatas)
{
    const int cc = columnCount();

    for (int i = 0; i < cc; ++i) {
        if (i < rowDatas.size()) {
            column(i)->push_back(rowDatas[ i ]);
        } else {
            column(i)->push_back(T());
        }
    }
    ++m_rows;
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::row(int r) const
{
    int csize    = columnCount();
    SeriesPtr rs = std::make_shared< SeriesType >(csize);

    for (int c = 0; c < csize; ++c) {
        rs->operator[](c) = cell(r, c);
    }
    return (rs);
}

template< typename T >
typename DAColumnTable< T >::SeriesType& DAColumnTable< T >::operator[](int colname) const
{
    return (*(column(colname)));
}

template< typename T >
const typename DAColumnTable< T >::SeriesType& DAColumnTable< T >::operator[](int c) const
{
    return (*(column(c)));
}
template< typename T >
typename DAColumnTable< T >::SeriesType& DAColumnTable< T >::operator[](const QString& colname)
{
    return (*(column(colname)));
}

template< typename T >
const typename DAColumnTable< T >::SeriesType& DAColumnTable< T >::operator[](const QString& colname) const
{
    return (*(column(colname)));
}

template< typename T >
void DAColumnTable< T >::fixSize()
{
    std::vector< int > ss;
    ss.reserve(m_d.size());

    for (const SeriesPtr& c : qAsConst(m_d)) {
        ss.push_back(c->size());
    }
    int maxsize = *(std::max_element(ss.begin(), ss.end()));

    for (SeriesPtr& c : m_d) {
        if (c->size() < maxsize) {
            c->resize(maxsize);
        }
    }
    m_rows = maxsize;
}

template< typename T >
void DAColumnTable< T >::setMode(DAColumnTable::Mode m)
{
    m_mode = m;
}

template< typename T >
typename DAColumnTable< T >::Mode DAColumnTable< T >::getMode() const
{
    return (m_mode);
}

template< typename T >
void DAColumnTable< T >::clear()
{
    m_d.clear();
    m_rows = 0;
}

template< typename T >
void DAColumnTable< T >::setName(const QString& n)
{
    m_d.setName(n);
}

template< typename T >
QString DAColumnTable< T >::getName() const
{
    return (m_d.getName());
}

template< typename T >
QStringList DAColumnTable< T >::columnNames() const
{
    QStringList r;

    for (const SeriesPtr& p : qAsConst(m_d)) {
        r.append(p->getName());
    }
    return (r);
}

}  // end DA

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
template< typename T >
QDebug operator<<(QDebug debug, const DA::DARowTable< T >& t)
{
    QDebugStateSaver saver(debug);
    int rs          = t.rowCount();
    QStringList rns = t.rowNames();
    int maxlen      = 0;

    for (const QString& r : rns) {
        maxlen = qMax(maxlen, r.size());
    }

    for (int i = 0; i < rs; ++i) {
        typename DA::DARowTable< T >::SeriesPtr r = t.row(i);
        QString name                              = r->getName();
        if (name.size() < maxlen) {
            name.resize(maxlen);
        }
        debug.noquote() << name << ":";
        int cs = r->size();
        if (cs > 10) {
            for (int j = 0; j < 5; ++j) {
                debug.noquote() << r->at(j) << ",";
            }
            debug.noquote() << "  ......  ";
            for (int j = cs - 6; j < cs; ++j) {
                debug.noquote() << r->at(j) << ",";
            }
        } else {
            for (int j = 0; j < cs; ++j) {
                debug << r->at(j) << ",";
            }
        }
        debug << "\n";
    }

    return (debug);
}
#else
template< typename T >
QDebug operator<<(QDebug debug, const DA::DARowTable< T >& t)
{
    QDebugStateSaver saver(debug);
    int rs          = t.rowCount();
    QStringList rns = t.rowNames();
    int maxlen      = 0;

    for (const QString& r : rns) {
        maxlen = qMax(maxlen, r.size());
    }

    for (int i = 0; i < rs; ++i) {
        typename DA::DARowTable< T >::SeriesPtr r = t.row(i);
        QString name                              = r->getName();
        if (name.size() < maxlen) {
            name.resize(maxlen);
        }
        debug.nospace() << name << ":";
        int cs = r->size();
        if (cs > 10) {
            for (int j = 0; j < 5; ++j) {
                debug.nospace() << r->at(j) << ",";
            }
            debug.nospace() << "  ......  ";
            for (int j = cs - 6; j < cs; ++j) {
                debug.nospace() << r->at(j) << ",";
            }
        } else {
            for (int j = 0; j < cs; ++j) {
                debug << r->at(j) << ",";
            }
        }
        debug << "\n";
    }

    return (debug);
}

#endif

#endif  // DATABLE_H
