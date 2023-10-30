#ifndef DACOLUMNTABLE_H
#define DACOLUMNTABLE_H
// std
#include <memory>
// DA
#include "DAAlgorithm.h"
#include "DAVector.h"
// Qt
#include <QDebug>
namespace DA
{

/**
 * @brief 列族表
 * @sa DARowTable
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
    /**
     * @brief 设置行名，如果是个空的表会生成一个默认行
     * @param ns
     */
    void setColumnNames(const QStringList& ns);

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
    void remove(int colIndex);

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
typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c)
{
    return (m_d[ c ]);
}
/**
 * @brief 获取列引用
 * @param r
 * @return
 */
template< typename T >
const typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c) const
{
    return (m_d[ c ]);
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n)
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (m_nullseries);
    }
    return (column(c));
}

template< typename T >
const typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n) const
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (m_nullseries);
    }
    return (column(c));
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

    c->setName(colName);
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
typename DAColumnTable< T >::SeriesType& DAColumnTable< T >::operator[](int c)
{
    return (*(column(c)));
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

template< typename T >
void DAColumnTable< T >::setColumnNames(const QStringList& ns)
{
    auto cc = std::min(ns.size(), columnCount());
    for (int i = 0; i < cc; ++i) {
        column(i)->setName(ns[ i ]);
    }
}

template< typename T >
void DAColumnTable< T >::setCaseSensitivity(DAColumnTable< T >::CaseSensitivity cs)
{
    m_caseSensitivity = cs;
}

template< typename T >
bool DAColumnTable< T >::isCaseSensitivity() const
{
    return (m_caseSensitivity == CaseSensitive);
}

template< typename T >
void DAColumnTable< T >::remove(const QString& name)
{
    int r = nameToIndex(name);
    if (r < 0) {
        return;
    }
    remove(r);
}

template< typename T >
void DAColumnTable< T >::remove(int colIndex)
{
    m_d.remove(colIndex);
    if (0 == m_d.size()) {
        m_rows = 0;
    }
}

}  // end DA

#endif  // DACOLUMNTABLE_H
