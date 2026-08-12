#ifndef DA_DATA_TABLE_H
#define DA_DATA_TABLE_H
// Qt
#include <QVector>
#include <QHash>
#include <QSet>
#include <QDebug>
// std
#include <memory>
// DA
#include "da_algorithm.hpp"
namespace DA
{

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
    //名字的引用
    const QString& name() const;
    QString& name();
    //名字的操作
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

/**
 * @brief 以列为基础的数据表，和python 的 pandas.DataFrame类似
 *
 * 表格形如：
 *
 *         列名字
 * _______________________________
 * |name| "0" | "1" | "2" | "3" | …… | "n" |
 * 表内容：
 * 行索引
 * -------------------------------
 * |0   | 1   | 2   | 1   | 0   | …… | 6   |
 * |1   | 4   | 5   | 6   | 7   | …… | 8   |
 * ………………
 * |n   | 9   | 8   | 7   | 6   | …… | 0   |
 * -------------------------------
 *
 * @code
 * DADataTable<int> table;
 * table.setColumnNames({"c1","c2",……,"cn"});
 * table.append({1,4,……,9});
 * table.append({2,5,……,8});
 * table.append({1,6,……,7});
 * table.append({0,7,……,6});
 * ……
 * table.append({6,8,……,0});
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
class DADataTable
{
public:
    enum Mode
    {
        FixedMode,  ///< 固定模式，表的行不会随着列的变化而变化
        ExpandMode  ///< 扩展模式，表的行是浮动的，如果插入一列比原来要宽，会自动扩充表的原来那些列对应的行数(默认)
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
    typedef DADataTable< T > TableType;
    typedef std::shared_ptr< TableType > TablePtr;
    static SeriesPtr makeSeries();
    static SeriesPtr makeSeries(const QString& n);
    static SeriesPtr makeSeries(std::initializer_list< T > args);
    static TablePtr makeTable();

    DADataTable();
    DADataTable(int rows, int cols);
    void resize(int r, int c);

    //判断是否存在field
    bool haveColumns(const QString& field) const;

    // 填充元素
    void fill(const T& v);
    const T& at(int r, int c) const;
    T& at(int r, int c);
    const T& iloc(int r, int c) const;
    T& iloc(int r, int c);

    T cell(int r, int c) const;

    void appendSeries(SeriesPtr ser);
    void appendSeries(std::initializer_list< T > args, const QString& n);

    template< typename Ite1, typename Ite2 >
    void appendRow(Ite1 b, Ite2 e);
    void appendRow(std::initializer_list< T > args);
    //名字搜索引
    int nameToIndex(const QString& n) const;
    SeriesPtr& series(int c);
    const SeriesPtr& series(int c) const;
    SeriesPtr& series(const QString& n);
    const SeriesPtr& series(const QString& n) const;

    //返回一列数据，返回一个SeriesPtr，这个seriesPtr的写操作不会影响table
    SeriesPtr row(int r) const;

    SeriesType& operator[](int c);
    const SeriesType& operator[](int c) const;
    SeriesType& operator[](const QString& rowname);
    const SeriesType& operator[](const QString& rowname) const;

    void reserve(int size);

    // 以最大列数进行列数修正，保证所有行同列
    void fixSize();

    // 表的行数
    int columnCount() const;

    // 表的列数
    int rowCount() const;

    // 设置表格的模式
    void setMode(Mode m);

    // 获取模式
    Mode getMode() const;

    // 清空
    void clear();

    void setName(const QString& n);
    QString getName() const;

    // 和pandas统一
    QStringList columns() const;
    QStringList columnNames() const;

    // 设置行名，如果是个空的表会生成一个默认行
    void setSeriesNames(const QStringList& ns);

    // 提取某个值等于value作为新表
    TablePtr takeByValue(const QString& field, T value) const;

    // groupby
    QPair< QList< TablePtr >, QList< T > > groupBy(const QString& field) const;

    // orderBy
    void orderBy(const QString& sn);
    void orderBy(int cindex);

    // 查找第一个大于或等于某个元素的位置
    QPair< T, int > lowerBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > lowerBound(const T& v, int c) const;
    QPair< T, int > upperBound(const T& v, const QString& sortedfield) const;
    QPair< T, int > upperBound(const T& v, int c) const;

    // 设置名字查询时是否对大小写敏感
    void setCaseSensitivity(CaseSensitivity cs);

    // 判断是否大小写敏感
    bool isCaseSensitivity() const;

private:
    DAVector< SeriesPtr > mD;
    size_t mRows;
    Mode mMode;
    SeriesPtr mNullseries;
    CaseSensitivity mCaseSensitivity;
};

template< typename T >
typename DADataTable< T >::TablePtr take_by_value(const DADataTable< T >& table, const QString& field, T value)
{
    const int c = table.nameToIndex(field);

    typename DADataTable< T >::TablePtr res = DADataTable< T >::makeTable();

    res->setName(table.getName());
    res->setSeriesNames(table.columns());
    int rsize = table.rowCount();

    for (int i = 0; i < rsize; ++i) {
        if (table.iloc(i, c) == value) {
            typename DADataTable< T >::SeriesPtr rv = table.row(i);
            res->appendRow(rv->begin(), rv->end());
        }
    }
    return (res);
}

// 逐行遍历，把指定列的内容和值一致的行保留，形成一个新的表
template< typename T >
typename DADataTable< T >::TablePtr take_by_value(const DADataTable< T >& table, int c, T value)
{
    typename DADataTable< T >::TablePtr res = DADataTable< T >::makeTable();

    res->setName(table.getName());
    // setSeriesNames对于不存在的列会扩充
    res->setSeriesNames(table.columns());
    //逐行遍历
    int rsize = table.rowCount();

    for (int i = 0; i < rsize; ++i) {
        if (table.iloc(i, c) == value) {
            typename DADataTable< T >::SeriesPtr rv = table.row(i);
            res->appendRow(rv->begin(), rv->end());
        }
    }
    return (res);
}

// groupby 对某个字段执行group by操作
template< typename T >
QPair< QList< typename DADataTable< T >::TablePtr >, QList< T > > group_by(const DADataTable< T >& table, const QString& field)
{
    QList< typename DADataTable< T >::TablePtr > restables;
    QList< T > gr;

    int cindex = table.nameToIndex(field);

    Q_ASSERT_X(cindex >= 0, "groupby", "unknow field");
    typename DADataTable< T >::SeriesPtr r = table.series(cindex);
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
        restables.append(take_by_value(table, cindex, v));
    }
    return (qMakePair(restables, gr));
}

/**
 * @brief 含有index的值对
 */
template< typename T >
class _DAValueWithIndex
{
public:
    _DAValueWithIndex() : index(-1)
    {
    }

    _DAValueWithIndex(const T& v, int i) : value(v), index(i)
    {
    }

    T value;
    int index;
};

template< typename T >
bool operator<(const _DAValueWithIndex< T >& a, const _DAValueWithIndex< T >& b)
{
    return (a.value < b.value);
}

// 把序列转换为带序号的序列
template< typename T >
std::shared_ptr< DAVector< _DAValueWithIndex< T > > > makeIndexSeries(const typename DADataTable< T >::SeriesPtr& p)
{
    std::shared_ptr< DAVector< _DAValueWithIndex< T > > > res = std::make_shared< DAVector< _DAValueWithIndex< T > > >();

    res->setName(p->getName());
    const int s = p->size();

    res->reserve(s);
    for (int i = 0; i < s; ++i) {
        res->push_back(_DAValueWithIndex< T >(p->at(i), i));
    }
    return (res);
}

template< typename T >
void order_by(DADataTable< T >& table, const QString& field)
{
    const int r = table.nameToIndex(field);
    if (r < 0) {
        return;
    }
    order_by(table, r);
}

template< typename T >
void order_by(DADataTable< T >& table, int c)
{
    typename DADataTable< T >::SeriesPtr ser = table.series(c);
    Q_ASSERT_X(ser != nullptr, "orderBy", "unknow field");
    auto ordser = makeIndexSeries< T >(ser);

    std::sort(ordser->begin(), ordser->end());
    int sercount = table.columnCount();

    //开始逐一转换
    for (int i = 0; i < sercount; ++i) {
        typename DADataTable< T >::SeriesPtr series = table.series(i);
        typename DADataTable< T >::SeriesPtr ns     = DADataTable< T >::makeSeries(series->getName());
        ns->reserve(series->size());
        for (auto j = ordser->begin(); j != ordser->end(); ++j) {
            ns->push_back(series->at((*j).index));
        }
        table.series(i).swap(ns);
    }
}

// 构造一个空的数据表
template< typename T >
DADataTable< T >::DADataTable() : mRows(0), mMode(ExpandMode), mCaseSensitivity(CaseInsensitive)
{
}

// 构造一个rows行，cols列的表，表的命名以column_{n}来命名
template< typename T >
DADataTable< T >::DADataTable(int rows, int cols) : mMode(ExpandMode), mCaseSensitivity(CaseInsensitive)
{
    mD.clear();
    mD.reserve(cols);
    for (int i = 0; i < cols; ++i) {
        SeriesPtr r = makeSeries(rows);
        r->setName(QString("column_%1").arg(i));
        appendSeries(r);
    }
}

// 改变table 的大小
template< typename T >
void DADataTable< T >::resize(int r, int c)
{
    mD.resize(c);
    for (SeriesPtr& ser : mD) {
        if (ser == nullptr) {
            ser = makeSeries();
        }
        ser->resize(r);
    }
    mRows = r;
}

// 判断是否存在Columns
template< typename T >
bool DADataTable< T >::haveColumns(const QString& field) const
{
    int c                  = columnCount();
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int i = 0; i < c; ++i) {
        if (series(i)->name().compare(field, cs) == 0) {
            return (true);
        }
    }
    return (false);
}

// 填充值
template< typename T >
void DADataTable< T >::fill(const T& v)
{
    for (const SeriesPtr& s : qAsConst(mD)) {
        s->fill(v);
    }
}

// 定位单元格
template< typename T >
const T& DADataTable< T >::at(int r, int c) const
{
    return (mD.at(c)->at(r));
}

// 定位单元格
template< typename T >
T& DADataTable< T >::at(int r, int c)
{
    return (*mD[ c ])[ r ];
}

// 定位单元格
template< typename T >
const T& DADataTable< T >::iloc(int r, int c) const
{
    return at(r, c);
}

// 定位单元格
template< typename T >
T& DADataTable< T >::iloc(int r, int c)
{
    return at(r, c);
}

// 获取单元格
template< typename T >
T DADataTable< T >::cell(int r, int c) const
{
    if (c >= 0 && c < mD.size()) {
        const SeriesPtr& s = series(c);
        if (s && r >= 0 && r < s->size()) {
            return (s->at(r));
        }
    }
    return (T());
}

// 追加一列
template< typename T >
void DADataTable< T >::appendSeries(SeriesPtr ser)
{
    size_t s = ser->size();

    if (0 == mRows) {
        //如果是空表，空表不对mode做判断
        resize(s, columnCount());
        mD.push_back(ser);
        mRows = s;
    } else if (s == mRows) {
        //如果插入的列的行数和表行数一致，直接插入
        mD.push_back(ser);
    } else if (s < mRows) {
        //如果插入的列的行数比表行数少，先在结尾补充
        ser->resize(mRows);
        mD.push_back(ser);
    } else {
        // 如果插入的函数s>mRows
        if (getMode() == ExpandMode) {
            for (const SeriesPtr& colser : qAsConst(mD)) {
                colser->resize(s);
            }
            mD.push_back(ser);
            mRows = s;
        } else {
            //固定模式则把ser截断插入
            ser->resize(mRows);
            mD.push_back(ser);
        }
    }
}

template< typename T >
void DADataTable< T >::appendSeries(std::initializer_list< T > args, const QString& n)
{
    SeriesPtr r = makeSeries(args);

    r->setName(n);
    appendSeries(r);
}

template< typename T >
template< typename Ite1, typename Ite2 >
void DADataTable< T >::appendRow(Ite1 b, Ite2 e)
{
    const int rc = columnCount();
    auto it      = b;

    for (int i = 0; i < rc; ++i) {
        it = b + i;
        if (it < e) {
            series(i)->push_back(*it);
        } else {
            series(i)->push_back(T());
        }
    }
    ++mRows;
}

template< typename T >
void DADataTable< T >::appendRow(std::initializer_list< T > args)
{
    appendRow(args.begin(), args.end());
    //    const int rc = columnCount();
    //    for (int i = 0; i < rc; ++i) {
    //        if (i < args.size()) {
    //            series(i)->push_back(args[ i ]);
    //        } else {
    //            series(i)->push_back(T());
    //        }
    //    }
    //    ++mRows;
}

template< typename T >
typename DADataTable< T >::SeriesPtr DADataTable< T >::makeSeries(const QString& n)
{
    return (std::make_shared< SeriesType >(n));
}

template< typename T >
typename DADataTable< T >::SeriesPtr DADataTable< T >::makeSeries(std::initializer_list< T > args)
{
    return (std::make_shared< SeriesType >(args));
}

template< typename T >
typename DADataTable< T >::TablePtr DADataTable< T >::makeTable()
{
    return (std::make_shared< typename DADataTable< T >::TableType >());
}

template< typename T >
typename DADataTable< T >::SeriesPtr DADataTable< T >::makeSeries()
{
    return (std::make_shared< SeriesType >());
}

template< typename T >
int DADataTable< T >::nameToIndex(const QString& n) const
{
    int r                  = columnCount();
    Qt::CaseSensitivity cs = isCaseSensitivity() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int i = 0; i < r; ++i) {
        if (series(i)->name().compare(n, cs) == 0) {
            return (i);
        }
    }
    return (-1);
}

// 获取行引用
template< typename T >
typename DADataTable< T >::SeriesPtr& DADataTable< T >::series(int c)
{
    return (mD[ c ]);
}

// 获取列引用
template< typename T >
const typename DADataTable< T >::SeriesPtr& DADataTable< T >::series(int c) const
{
    return (mD[ c ]);
}

// 获取列引用
template< typename T >
typename DADataTable< T >::SeriesPtr& DADataTable< T >::series(const QString& n)
{
    int r = nameToIndex(n);

    if ((r < 0) || (r >= columnCount())) {
        return (mNullseries);
    }
    return (series(r));
}

// 获取列引用
template< typename T >
const typename DADataTable< T >::SeriesPtr& DADataTable< T >::series(const QString& n) const
{
    int i = nameToIndex(n);

    if ((i < 0) || (i >= columnCount())) {
        return (mNullseries);
    }
    return (series(i));
}

// 获取一行数据
template< typename T >
typename DADataTable< T >::SeriesPtr DADataTable< T >::row(int r) const
{
    int csize    = columnCount();
    SeriesPtr sr = std::make_shared< SeriesType >(csize);

    for (int c = 0; c < csize; ++c) {
        (*sr)[ c ] = cell(r, c);
    }
    return (sr);
}

template< typename T >
typename DADataTable< T >::SeriesType& DADataTable< T >::operator[](int c)
{
    return (*(series(c)));
}

template< typename T >
const typename DADataTable< T >::SeriesType& DADataTable< T >::operator[](int c) const
{
    return (*(series(c)));
}

template< typename T >
typename DADataTable< T >::SeriesType& DADataTable< T >::operator[](const QString& rowname)
{
    return (*(series(rowname)));
}

template< typename T >
const typename DADataTable< T >::SeriesType& DADataTable< T >::operator[](const QString& rowname) const
{
    return (*(series(rowname)));
}

template< typename T >
void DADataTable< T >::reserve(int size)
{
    for (const SeriesPtr& p : qAsConst(mD)) {
        p->reserve(size);
    }
}

// 在每个列的行数不一样的时候调用此函数让整个表格规整
template< typename T >
void DADataTable< T >::fixSize()
{
    std::vector< int > ss;
    ss.reserve(mD.size());

    for (const SeriesPtr& r : mD) {
        ss.push_back(r->size());
    }
    int maxsize = *(std::max_element(ss.begin(), ss.end()));

    for (SeriesPtr& r : mD) {
        if (r->size() < maxsize) {
            r->resize(maxsize);
        }
    }
    mRows = maxsize;
}

// 列数
template< typename T >
int DADataTable< T >::columnCount() const
{
    return (mD.size());
}

// 行数
template< typename T >
int DADataTable< T >::rowCount() const
{
    return (mRows);
}

template< typename T >
void DADataTable< T >::setMode(typename DADataTable< T >::Mode m)
{
    mMode = m;
}

template< typename T >
typename DADataTable< T >::Mode DADataTable< T >::getMode() const
{
    return (mMode);
}

template< typename T >
void DADataTable< T >::clear()
{
    mD.clear();
    mRows = 0;
}

// 设置表的名字
template< typename T >
void DADataTable< T >::setName(const QString& n)
{
    mD.setName(n);
}

// 获取表的名字
template< typename T >
QString DADataTable< T >::getName() const
{
    return (mD.getName());
}

// 获取列的名字序列 等同 @sa columnNames
template< typename T >
QStringList DADataTable< T >::columns() const
{
    return columnNames();
}

// 获取列的名字序列
template< typename T >
QStringList DADataTable< T >::columnNames() const
{
    QStringList r;

    for (const SeriesPtr& p : qAsConst(mD)) {
        r.append(p->getName());
    }
    return (r);
}

// 设置列的名字序列
template< typename T >
void DADataTable< T >::setSeriesNames(const QStringList& ns)
{
    const int s = ns.size();

    for (int i = 0; i < s; ++i) {
        if (i < columnCount()) {
            series(i)->setName(ns[ i ]);
        } else {
            SeriesPtr r = makeSeries(ns[ i ]);
            appendSeries(r);
        }
    }
}

// 逐行遍历，把指定列的内容和值一致的行保留，形成一个新的表,主要用于匹配
template< typename T >
typename DADataTable< T >::TablePtr DADataTable< T >::takeByValue(const QString& field, T value) const
{
    return (take_by_value(*this, field, value));
}

// 聚合
template< typename T >
QPair< QList< typename DADataTable< T >::TablePtr >, QList< T > > DADataTable< T >::groupBy(const QString& field) const
{
    return (group_by(*this, field));
}

// 排序
template< typename T >
void DADataTable< T >::orderBy(const QString& sn)
{
    order_by(*this, sn);
}

// 排序
template< typename T >
void DADataTable< T >::orderBy(int cindex)
{
    order_by(*this, cindex);
}

// DADataTable<T>::lowerBound 查找
template< typename T >
QPair< T, int > DADataTable< T >::lowerBound(const T& v, const QString& sortedfield) const
{
    const int r = nameToIndex(sortedfield);

    return (lowerBound(v, r));
}

template< typename T >
QPair< T, int > DADataTable< T >::lowerBound(const T& v, int c) const
{
    typename DADataTable< T >::SeriesPtr prow = series(c);
    SeriesIterator ite                        = std::lower_bound(prow->begin(), prow->end(), v);

    if (ite == prow->end()) {
        return (qMakePair< T, int >(prow->back(), prow->size() - 1));
    }
    size_t dis = std::distance(prow->begin(), ite);

    return (qMakePair< T, int >(*ite, dis));
}

template< typename T >
QPair< T, int > DADataTable< T >::upperBound(const T& v, const QString& sortedfield) const
{
    const int r = nameToIndex(sortedfield);

    return (upperBound(v, r));
}

template< typename T >
QPair< T, int > DADataTable< T >::upperBound(const T& v, int c) const
{
    typename DADataTable< T >::SeriesPtr prow = series(c);
    SeriesIterator ite                        = std::upper_bound(prow->begin(), prow->end(), v);

    if (ite == prow->end()) {
        return (qMakePair< T, int >(prow->back(), prow->size() - 1));
    }
    size_t dis = std::distance(prow->begin(), ite);

    return (qMakePair< T, int >(*ite, dis));
}

// 设置大小写敏感性
template< typename T >
void DADataTable< T >::setCaseSensitivity(typename DADataTable< T >::CaseSensitivity cs)
{
    mCaseSensitivity = cs;
}

// 获取大小写敏感性
template< typename T >
bool DADataTable< T >::isCaseSensitivity() const
{
    return (mCaseSensitivity == CaseSensitive);
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))

template< typename T >
QDebug operator<<(QDebug debug, const DADataTable< T >& t)
{
    QDebugStateSaver saver(debug);
    int colCnt      = t.columnCount();
    QStringList rns = t.columnNames();
    int maxlen      = 0;

    for (const QString& r : rns) {
        //计算列宽
        maxlen = qMax(maxlen, r.size());
    }

    for (int i = 0; i < colCnt; ++i) {
        typename DADataTable< T >::SeriesPtr r = t.series(i);
        QString name                           = r->getName();
        if (name.size() < maxlen) {
            name.leftJustified(maxlen);  //补充长度
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
QDebug operator<<(QDebug debug, const DADataTable< T >& t)
{
    QDebugStateSaver saver(debug);
    int rs          = t.columnCount();
    QStringList rns = t.columns();
    int maxlen      = 0;

    for (const QString& r : rns) {
        maxlen = qMax(maxlen, r.size());
    }

    for (int i = 0; i < rs; ++i) {
        typename DADataTable< T >::SeriesPtr r = t.series(i);
        QString name                           = r->getName();
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
}
#endif  // SATABLE_H
