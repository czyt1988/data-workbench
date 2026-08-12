#ifndef DAROWTABLE_H
#define DAROWTABLE_H
// std
#include <memory>
// DA
#include "da_algorithm.hpp"
#include "DAVector.hpp"
// Qt
#include <QSet>
#include <QDebug>
namespace DA
{

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
 * DARowTable<int> table;
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
 *
 * @sa DAColumnTable
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

    // 填充元素
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

    // 以最大列数进行列数修正，保证所有行同列
    void fixSize();

    // 表的行数
    int rowCount() const;

    // 表的列数
    int columnCount() const;

    // 设置表格的模式
    void setMode(Mode m);

    // 获取模式
    Mode getMode() const;

    // 清空
    void clear();

    void setName(const QString& n);
    QString getName() const;

    // rowNames
    QStringList rowNames() const;

    // 设置行名，如果是个空的表会生成一个默认行
    void setRowNames(const QStringList& ns);

    // 提取某个值等于value作为新表
    TablePtr takeByValue(const QString& field, T value) const;

    // groupby
    QPair< QList< TablePtr >, QList< T > > groupBy(const QString& field) const;

    // orderBy
    void orderBy(const QString& sn);
    void orderBy(int rindex);

    // 查找第一个大于或等于某个元素的位置
    QPair< T, int > lowerBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > lowerBound(const T& v, int r) const;
    QPair< T, int > upperBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > upperBound(const T& v, int r) const;

    // 设置名字查询时是否对大小写敏感
    void setCaseSensitivity(CaseSensitivity cs);

    // 判断是否大小写敏感
    bool isCaseSensitivity() const;
    //移除
    void remove(const QString& name);
    void remove(int rindex);

private:
    DAVector< SeriesPtr > mD;
    size_t mColumns;
    Mode mMode;
    SeriesPtr mNullseries;
    CaseSensitivity mCaseSensitivity;
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

// takeByValue 类似于select * from table where table.r = value
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

// groupby 对某个字段执行group by操作
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
    QSet< T > uniqueSet;
    for (const T& v : qAsConst(*r)) {
        uniqueSet.insert(v);
    }
    gr = QList< T >(uniqueSet.begin(), uniqueSet.end());
    std::sort(gr.begin(), gr.end());
    for (const T& v : qAsConst(gr)) {
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

// 把序列转换为带序号的序列
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
DARowTable< T >::DARowTable() : mColumns(0), mMode(FixedMode), mCaseSensitivity(CaseInsensitive)
{
}

template< typename T >
DARowTable< T >::DARowTable(int rows, int columns) : mMode(FixedMode), mCaseSensitivity(CaseInsensitive)
{
    mD.clear();
    mD.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        mD.push_back(SeriesType(columns));
    }
    mColumns = columns;
}

// 改变table 的大小
template< typename T >
void DARowTable< T >::resize(int r, int c)
{
    mD.resize(r);
    for (SeriesPtr& row : mD) {
        if (row == nullptr) {
            row = makeSeries();
        }
        row->resize(c);
    }
    mColumns = c;
}

// 判断是否存在field
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
    for (const SeriesPtr& r : qAsConst(mD)) {
        r->fill(v);
    }
}

template< typename T >
const T& DARowTable< T >::at(int r, int c) const
{
    return (mD.at(r)->at(c));
}

template< typename T >
T& DARowTable< T >::at(int r, int c)
{
    return (mD[ r ]->operator[](c));
}

// 获取单元格
template< typename T >
T DARowTable< T >::cell(int r, int c) const
{
    if (r < mD.size()) {
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

    if ((s == mColumns) || (0 == mColumns)) {
        mD.push_back(row);
        mColumns = s;
    } else if (s < mColumns) {  //在结尾补充
        row->resize(mColumns);
        mD.push_back(row);
    } else {  // s>mColumns
        if (getMode() == ExpandMode) {
            mD.push_back(row);
            fixSize();
        } else {
            //固定模式的插入
            row->resize(mColumns);
            mD.push_back(row);
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
    ++mColumns;
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
    ++mColumns;
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

// 获取行引用
template< typename T >
typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(int r)
{
    return (mD[ r ]);
}

// 获取行引用
template< typename T >
const typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(int r) const
{
    return (mD[ r ]);
}

template< typename T >
typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(const QString& n)
{
    int r = nameToIndex(n);

    if ((r < 0) || (r >= rowCount())) {
        return (mNullseries);
    }
    return (row(r));
}

template< typename T >
const typename DARowTable< T >::SeriesPtr& DARowTable< T >::row(const QString& n) const
{
    int r = nameToIndex(n);

    if ((r < 0) || (r >= rowCount())) {
        return (mNullseries);
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
    for (const SeriesPtr& p : qAsConst(mD)) {
        p->reserve(size);
    }
}

template< typename T >
void DARowTable< T >::fixSize()
{
    std::vector< int > ss;

    for (const SeriesPtr& r : qAsConst(mD)) {
        ss.push_back(r->size());
    }
    int maxsize = *(std::max_element(ss.begin(), ss.end()));

    for (SeriesPtr& r : mD) {
        if (r->size() < maxsize) {
            r->resize(maxsize);
        }
    }
    mColumns = maxsize;
}

template< typename T >
int DARowTable< T >::rowCount() const
{
    return (mD.size());
}

template< typename T >
int DARowTable< T >::columnCount() const
{
    return (mColumns);
}

template< typename T >
void DARowTable< T >::setMode(typename DARowTable< T >::Mode m)
{
    mMode = m;
}

template< typename T >
typename DARowTable< T >::Mode DARowTable< T >::getMode() const
{
    return (mMode);
}

template< typename T >
void DARowTable< T >::clear()
{
    mD.clear();
    mColumns = 0;
}

template< typename T >
void DARowTable< T >::setName(const QString& n)
{
    mD.setName(n);
}

template< typename T >
QString DARowTable< T >::getName() const
{
    return (mD.getName());
}

template< typename T >
QStringList DARowTable< T >::rowNames() const
{
    QStringList r;

    for (const SeriesPtr& p : qAsConst(mD)) {
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
        }
    }
}

template< typename T >
typename DARowTable< T >::TablePtr DARowTable< T >::takeByValue(const QString& field, T value) const
{
    return (DA::takeByValue(*this, field, value));
}

template< typename T >
QPair< QList< typename DARowTable< T >::TablePtr >, QList< T > > DARowTable< T >::groupBy(const QString& field) const
{
    return (DA::groupby(*this, field));
}

template< typename T >
void DARowTable< T >::orderBy(const QString& sn)
{
    DA::orderBy(*this, sn);
}

template< typename T >
void DARowTable< T >::orderBy(int rindex)
{
    DA::orderBy(*this, rindex);
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
    mCaseSensitivity = cs;
}

template< typename T >
bool DARowTable< T >::isCaseSensitivity() const
{
    return (mCaseSensitivity == CaseSensitive);
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
    mD.remove(rindex);
    if (0 == mD.size()) {
        mColumns = 0;
    }
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

#endif  // DAROWTABLE_H
