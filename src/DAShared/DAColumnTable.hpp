#ifndef DACOLUMNTABLE_H
#define DACOLUMNTABLE_H
// std
#include <memory>
// DA
#include "da_algorithm.hpp"
#include "DAVector.hpp"
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
    // 表的行数
    int rowCount() const;

    // 表的列数
    int columnCount() const;
    // 填充元素
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

    // 以最大列数进行列数修正，保证所有行同列
    void fixSize();
    // 设置表格的模式
    void setMode(Mode m);
    Mode getMode() const;

    // 清空
    void clear();

    void setName(const QString& n);
    QString getName() const;
    // colNames
    QStringList columnNames() const;
    // 设置行名，如果是个空的表会生成一个默认行
    void setColumnNames(const QStringList& ns);

    // 设置名字查询时是否对大小写敏感
    void setCaseSensitivity(CaseSensitivity cs);

    // 判断是否大小写敏感
    bool isCaseSensitivity() const;
    //移除
    void remove(const QString& name);
    void remove(int colIndex);

private:
    DAVector< SeriesPtr > mD;
    size_t mRows;  ///< 记录行数
    Mode mMode;
    SeriesPtr mNullseries;
    CaseSensitivity mCaseSensitivity;
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
DAColumnTable< T >::DAColumnTable() : mRows(0), mMode(FixedMode), mCaseSensitivity(CaseInsensitive)
{
}

template< typename T >
DAColumnTable< T >::DAColumnTable(int rows, int columns) : mMode(FixedMode), mCaseSensitivity(CaseInsensitive)
{
    mD.clear();
    mD.reserve(columns);
    for (int i = 0; i < columns; ++i) {
        mD.push_back(SeriesType(rows));
    }
    mRows = rows;
}

// 改变table 的大小
template< typename T >
void DAColumnTable< T >::resize(int r, int c)
{
    mD.resize(c);
    for (SeriesPtr& col : mD) {
        if (col == nullptr) {
            col = makeSeries();
        }
        col->resize(r);
    }
    mRows = r;
}

template< typename T >
void DAColumnTable< T >::reserve(int size)
{
    for (const SeriesPtr& p : qAsConst(mD)) {
        p->reserve(size);
    }
}

template< typename T >
bool DAColumnTable< T >::haveFieldid(const QString& field) const
{
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (const SeriesPtr& col : qAsConst(mD)) {
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
    return mRows;
}

template< typename T >
int DAColumnTable< T >::columnCount() const
{
    return (mD.size());
}

template< typename T >
void DAColumnTable< T >::fill(const T& v)
{
    for (const SeriesPtr& c : qAsConst(mD)) {
        c->fill(v);
    }
}

template< typename T >
const T& DAColumnTable< T >::at(int r, int c) const
{
    return (mD.at(c)->at(r));
}

template< typename T >
T& DAColumnTable< T >::at(int r, int c)
{
    return (mD[ c ]->operator[](r));
}

// 获取单元格
template< typename T >
T DAColumnTable< T >::cell(int r, int c) const
{
    if (c < mD.size()) {
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
// 获取列引用
template< typename T >
typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c)
{
    return (mD[ c ]);
}
// 获取列引用
template< typename T >
const typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(int c) const
{
    return (mD[ c ]);
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n)
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (mNullseries);
    }
    return (column(c));
}

template< typename T >
const typename DAColumnTable< T >::SeriesPtr& DAColumnTable< T >::column(const QString& n) const
{
    int c = nameToIndex(n);

    if ((c < 0) || (c >= columnCount())) {
        return (mNullseries);
    }
    return (column(c));
}

template< typename T >
typename DAColumnTable< T >::SeriesPtr DAColumnTable< T >::appendColumn(const QString& name)
{
    return appendColumn(name, mRows);
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

    if ((s == mRows) || (0 == mRows)) {
        mD.push_back(col);
        mRows = s;
    } else if (s < mRows) {  //在结尾补充
        col->resize(mRows);
        mD.push_back(col);
    } else {  // s>m_columns
        if (getMode() == ExpandMode) {
            mD.push_back(col);
            fixSize();
        } else {
            //固定模式的插入
            col->resize(mRows);
            mD.push_back(col);
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
    ++mRows;
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
    ++mRows;
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
    ss.reserve(mD.size());

    for (const SeriesPtr& c : qAsConst(mD)) {
        ss.push_back(c->size());
    }
    int maxsize = *(std::max_element(ss.begin(), ss.end()));

    for (const SeriesPtr& c : qAsConst(mD)) {
        if (c->size() < maxsize) {
            c->resize(maxsize);
        }
    }
    mRows = maxsize;
}

template< typename T >
void DAColumnTable< T >::setMode(DAColumnTable::Mode m)
{
    mMode = m;
}

template< typename T >
typename DAColumnTable< T >::Mode DAColumnTable< T >::getMode() const
{
    return (mMode);
}

template< typename T >
void DAColumnTable< T >::clear()
{
    mD.clear();
    mRows = 0;
}

template< typename T >
void DAColumnTable< T >::setName(const QString& n)
{
    mD.setName(n);
}

template< typename T >
QString DAColumnTable< T >::getName() const
{
    return (mD.getName());
}

template< typename T >
QStringList DAColumnTable< T >::columnNames() const
{
    QStringList r;

    for (const SeriesPtr& p : qAsConst(mD)) {
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
    mCaseSensitivity = cs;
}

template< typename T >
bool DAColumnTable< T >::isCaseSensitivity() const
{
    return (mCaseSensitivity == CaseSensitive);
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
    mD.remove(colIndex);
    if (0 == mD.size()) {
        mRows = 0;
    }
}

}  // end DA

#endif  // DACOLUMNTABLE_H
