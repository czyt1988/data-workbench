#include "DAPyScriptsDataFrame.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>
#include "DALogCategory.h"
namespace DA
{

//===================================================
// DAPyScriptsDataFrame
//===================================================

/** @brief 构造DAPyScriptsDataFrame @param autoImport 是否自动导入模块 */
DAPyScriptsDataFrame::DAPyScriptsDataFrame(bool autoImport) : DAPyModule()
{
    if (autoImport) {
        if (!import()) {
            daCritical << QObject::tr("cannot import da_dataframe module");  // cn:无法导入 da_dataframe 模块
        }
    }
}

/** @brief 构造DAPyScriptsDataFrame @param obj Python模块对象 */
DAPyScriptsDataFrame::DAPyScriptsDataFrame(const pybind11::object& obj) : DAPyModule(obj)
{
    if (!isModule()) {
        daCritical << QObject::tr("cannot import DAWorkbench.dataframe");  // cn:无法导入 DAWorkbench.dataframe 模块
    }
}

/** @brief 析构DAPyScriptsDataFrame */
DAPyScriptsDataFrame::~DAPyScriptsDataFrame()
{
}

/**
 * @brief 移除dataframe的行，对应da_dataframe.da_drop_irow
 * @param df
 * @param index
 * @return 成功执行返回true
 */
bool DAPyScriptsDataFrame::drop_irow(DAPyDataFrame& df, const QList< int >& index) noexcept
{
    try {
        pybind11::object da_drop_irow = attr("da_drop_irow");
        da_drop_irow(df.object(), DA::PY::toPyObject(index));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 移除dataframe的列，对应da_dataframe.da_drop_icolumn
 * @param df
 * @param index
 * @return 成功执行返回true
 */
bool DAPyScriptsDataFrame::drop_icolumn(DAPyDataFrame& df, const QList< int >& index) noexcept
{
    try {
        pybind11::object da_drop_icolumn = attr("da_drop_icolumn");
        da_drop_icolumn(df.object(), DA::PY::toPyObject(index));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief DAPyScriptsDataFrame::insert_nanrow
 * @param df
 * @param r
 * @return
 */
bool DAPyScriptsDataFrame::insert_nanrow(DAPyDataFrame& df, int r) noexcept
{
    try {
        pybind11::object da_insert_nanrow = attr("da_insert_nanrow");
        da_insert_nanrow(df.object(), pybind11::int_(r));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 插入列
 * @param df
 * @param c 列的索引
 * @param name 列名
 * @param defaultvalue 值，如果为QVariant,将插入np.nan,指定不同类型将插入不同的值
 * @return
 */
bool DAPyScriptsDataFrame::insert_column(DAPyDataFrame& df, int c, const QString& name, const QVariant& defaultvalue) noexcept
{
    try {
        pybind11::object da_insert_column = attr("da_insert_column");
        pybind11::dict args;
        args[ "df" ]   = df.object();
        args[ "col" ]  = pybind11::int_(c);
        args[ "name" ] = DA::PY::toPyObject(name);
        if (defaultvalue.isValid()) {
            args[ "defaultvalue" ] = pybind11::cast(defaultvalue);
        }
        da_insert_column(**args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 插入列
 * @param df
 * @param c
 * @param name
 * @param start
 * @param stop
 * @return
 */
bool DAPyScriptsDataFrame::insert_column(DAPyDataFrame& df,
                                         int c,
                                         const QString& name,
                                         const QVariant& start,
                                         const QVariant& stop) noexcept
{
    try {
        pybind11::object da_insert_column = attr("da_insert_column");
        pybind11::dict args;
        args[ "df" ]    = df.object();
        args[ "col" ]   = pybind11::int_(c);
        args[ "name" ]  = DA::PY::toPyObject(name);
        args[ "start" ] = pybind11::cast(start);
        args[ "stop" ]  = pybind11::cast(stop);
        if (start.canConvert< QDateTime >() || start.canConvert< QDate >() || start.canConvert< QTime >()) {
            args[ "dtype" ] = pybind11::dtype("datetime64");
        }
        da_insert_column(**args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 保存为txt/csv 对应da_dataframe.da_to_csv
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::to_csv(const DAPyDataFrame& df, const QString& path, const QString& sep) noexcept
{
    try {
        pybind11::object da_to_csv = attr("da_to_csv");
        da_to_csv(df.object(), DA::PY::toPyObject(path), DA::PY::toPyObject(sep));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 保存为xlsx 对应da_dataframe.da_to_xlsx
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::to_excel(const DAPyDataFrame& df, const QString& path) noexcept
{
    try {
        pybind11::object da_to_excel = attr("da_to_excel");
        da_to_excel(df.object(), DA::PY::toPyObject(path));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 保存为pickle 对应da_dataframe.da_to_pickle
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::to_pickle(const DAPyDataFrame& df, const QString& path) noexcept
{
    try {
        pybind11::object da_to_pickle = attr("da_to_pickle");
        da_to_pickle(df.object(), DA::PY::toPyObject(path));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 保存为parquet对应da_dataframe.da_to_parquet
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::to_parquet(const DAPyDataFrame& df, const QString& path) noexcept
{
    try {
        pybind11::object da_to_parquet = attr("da_to_parquet");
        da_to_parquet(df.object(), DA::PY::toPyObject(path));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 从pickle加载
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::from_pickle(DAPyDataFrame& df, const QString& path) noexcept
{
    try {
        pybind11::object da_from_pickle = attr("da_from_pickle");
        da_from_pickle(df.object(), DA::PY::toPyObject(path));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 从parquet加载
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::from_parquet(DAPyDataFrame& df, const QString& path) noexcept
{
    try {
        pybind11::object da_from_parquet = attr("da_from_parquet");
        da_from_parquet(df.object(), DA::PY::toPyObject(path));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 对列转换为对应的类型
 * @param df
 * @param colsIndex
 * @param dt
 * @return
 */
bool DAPyScriptsDataFrame::astype(DAPyDataFrame& df, const QList< int >& colsIndex, const DAPyDType& dt) noexcept
{
    try {
        pybind11::object da_astype = attr("da_astype");
        pybind11::list index;
        for (int v : colsIndex) {
            index.append(pybind11::int_(v));
        }
        da_astype(df.object(), index, dt.object());
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 设置nan值
 * @param df
 * @param rowsIndex
 * @param colsIndex
 * @return
 */
bool DAPyScriptsDataFrame::setnan(DAPyDataFrame& df, const QList< int >& rowsIndex, const QList< int >& colsIndex) noexcept
{
    // rowsIndex 和 colsIndex 应为一一对应的坐标对，长度必须一致
    if (rowsIndex.size() != colsIndex.size()) {
        daWarning << QObject::tr("setnan: rowsIndex size(%1) != colsIndex size(%2)")
                         .arg(rowsIndex.size())
                         .arg(colsIndex.size());  // cn:setnan: 行索引长度(%1)与列索引长度(%2)不一致
        return false;
    }
    try {
        pybind11::object da_setnan = attr("da_setnan");
        pybind11::list rows;
        pybind11::list columns;
        for (int i = 0; i < rowsIndex.size(); ++i) {
            rows.append(pybind11::int_(rowsIndex[ i ]));
            columns.append(pybind11::int_(colsIndex[ i ]));
        }
        da_setnan(df.object(), rows, columns);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/** @brief 导入DAWorkbench.dataframe模块 @return 导入成功返回true */
bool DAPyScriptsDataFrame::import() noexcept
{
    try {
        pybind11::module m = pybind11::module::import("DAWorkbench");
        object()           = m.attr("dataframe");
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}

/**
 * @brief DAPyScriptsDataFrame::cast_to_num
 * @param df
 * @param args
 * @return
 */
bool DAPyScriptsDataFrame::cast_to_num(DAPyDataFrame& df, const QList< int >& colsIndex, pybind11::dict args) noexcept
{
    try {
        pybind11::object da_cast_to_num = attr("da_cast_to_num");
        pybind11::list index;
        for (int v : colsIndex) {
            index.append(pybind11::int_(v));
        }
        da_cast_to_num(df.object(), index, **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}
/**
 * @brief DAPyScriptsDataFrame::cast_to_num
 * @param df
 * @param args
 * @return
 */
bool DAPyScriptsDataFrame::cast_to_datetime(DAPyDataFrame& df, const QList< int >& colsIndex, pybind11::dict args) noexcept
{
    try {
        pybind11::object da_cast_to_datetime = attr("da_cast_to_datetime");
        pybind11::list index;
        for (int v : colsIndex) {
            index.append(pybind11::int_(v));
        }
        da_cast_to_datetime(df.object(), index, **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief DAPyScriptsDataFrame::set_index
 * @param df
 * @param colsIndex
 * @return
 */
bool DAPyScriptsDataFrame::set_index(DAPyDataFrame& df, const QList< int >& colsIndex) noexcept
{
    try {
        pybind11::object da_setindex = attr("da_setindex");
        pybind11::list index;
        for (int v : colsIndex) {
            index.append(pybind11::int_(v));
        }
        da_setindex(df.object(), index);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 提取一列
 * @param df
 * @param col
 * @return
 */
DAPySeries DAPyScriptsDataFrame::itake_column(DAPyDataFrame& df, int col) noexcept
{
    try {
        pybind11::object da_itake_column = attr("da_itake_column");
        // 执行
        DAPySeries s = da_itake_column(df.object(), pybind11::int_(col));
        return s;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPySeries();
}

/**
 * @brief 插入series
 * @param df
 * @param col
 * @param series
 * @return
 */
bool DAPyScriptsDataFrame::insert_at(DAPyDataFrame& df, int col, const DAPySeries& series) noexcept
{
    try {
        pybind11::object da_insert_at = attr("da_insert_at");
        da_insert_at(df.object(), pybind11::int_(col), series.object());
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief dropna方法的wrapper
 * @param df
 * @param axis
 * @param how
 * @param indexs
 * @param thresh
 * @return
 */
bool DAPyScriptsDataFrame::dropna(DAPyDataFrame& df,
                                  int axis,
                                  const QString& how,
                                  const QList< int >& indexs,
                                  std::optional< int > thresh) noexcept
{
    try {
        pybind11::object da_drop_na = attr("da_drop_na");
        pybind11::object threshobj  = pybind11::none();
        if (thresh) {
            threshobj = pybind11::int_(thresh.value());
        }
        pybind11::dict args;
        args[ "axis" ]   = axis;
        args[ "how" ]    = DA::PY::toPyObject(how);
        args[ "index" ]  = DA::PY::toPyObject(indexs);
        args[ "thresh" ] = threshobj;
        da_drop_na(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief fillna方法的wrapper
 * @param df
 * @param value 要填充的值
 * @param limit 填充限制，-1代表不限制，大于0生效
 * @return
 */
bool DAPyScriptsDataFrame::fillna(DAPyDataFrame& df, double value, int limit) noexcept
{
    try {
        pybind11::object da_fill_na = attr("da_fill_na");
        pybind11::object limitObj   = pybind11::none();
        if (limit > 0) {
            limitObj = pybind11::int_(limit);
        }
        pybind11::dict args;
        args[ "value" ] = value;
        args[ "limit" ] = limitObj;
        da_fill_na(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief ffillna方法的wrapper
 * @param df
 * @param axis
 * @param limit 填充限制，-1代表不限制，大于0生效
 * @return
 */
bool DAPyScriptsDataFrame::ffillna(DAPyDataFrame& df, int axis, int limit) noexcept
{
    try {
        pybind11::object da_ffill_na = attr("da_ffill_na");
        pybind11::object limitObj    = pybind11::none();
        if (limit > 0) {
            limitObj = pybind11::int_(limit);
        }
        pybind11::dict args;
        args[ "axis" ]  = axis;
        args[ "limit" ] = limitObj;
        da_ffill_na(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief bfillna方法的wrapper
 * @param df
 * @param axis
 * @param limit 填充限制，-1代表不限制，大于0生效
 * @return
 */
bool DAPyScriptsDataFrame::bfillna(DAPyDataFrame& df, int axis, int limit) noexcept
{
    try {
        pybind11::object da_bfill_na = attr("da_bfill_na");
        pybind11::object limitObj    = pybind11::none();
        if (limit > 0) {
            limitObj = pybind11::int_(limit);
        }
        pybind11::dict args;
        args[ "axis" ]  = axis;
        args[ "limit" ] = limitObj;
        da_bfill_na(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}
/**
 * @brief interpolate方法的wrapper
 * @param df
 * @param method 插值填充的方法
 * @param order 多项式插值的次数
 * @param limit 填充限制，-1代表不限制，大于0生效
 * @return
 */
bool DAPyScriptsDataFrame::interpolate(DAPyDataFrame& df, const QString& method, int order, int limit) noexcept
{
    try {
        pybind11::object da_interpolate = attr("da_fill_interpolate");
        pybind11::object limitObj       = pybind11::none();
        if (limit > 0) {
            limitObj = pybind11::int_(limit);
        }
        pybind11::dict args;
        args[ "method" ] = DA::PY::toPyObject(method);
        args[ "order" ]  = order;
        args[ "limit" ]  = limitObj;
        da_interpolate(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}
/**
 * @brief dropduplicates方法的wrapper
 * @param df
 * @param keep
 * @param indexs
 * @return
 */
bool DAPyScriptsDataFrame::dropduplicates(DAPyDataFrame& df, const QString& keep, const QList< int >& indexs) noexcept
{
    try {
        pybind11::object da_drop_duplicates = attr("da_drop_duplicates");
        pybind11::dict args;
        args[ "keep" ] = DA::PY::toPyObject(keep);
        if (indexs.empty()) {
            args[ "index" ] = pybind11::none();
        } else {
            args[ "index" ] = DA::PY::toPyObject(indexs);
        }
        da_drop_duplicates(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief nstdfilter方法的wrapper
 * @param df
 * @param axis
 * @param n 标准差的倍数，范围是0.1~10，默认为3
 * @return
 */
bool DAPyScriptsDataFrame::nstdfilteroutlier(DAPyDataFrame& df, double n, int axis, const QList< int >& indexs) noexcept
{
    try {
        pybind11::object da_nstd_filter_outlier = attr("da_nstd_filter_outlier");
        if (n > 10 || n < 0.1) {
            throw std::invalid_argument("n must be between 0.1 and 10.");
        }
        pybind11::dict args;
        args[ "n" ]    = n;
        args[ "axis" ] = pybind11::int_(axis);
        if (indexs.empty()) {
            args[ "index" ] = pybind11::none();
        } else {
            args[ "index" ] = DA::PY::toPyObject(indexs);
        }
        da_nstd_filter_outlier(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief clipoutlier方法的wrapper
 * @param df
 * @param lowervalue
 * @param uppervalue
 * @return
 */
bool DAPyScriptsDataFrame::clipoutlier(DAPyDataFrame& df, double lowervalue, double uppervalue, int axis) noexcept
{
    try {
        pybind11::object da_clip_outlier = attr("da_clip_outlier");
        pybind11::dict args;
        // 0.0 为哨兵值，表示不限制该边界
        if (lowervalue == 0.0) {
            args[ "lower" ] = pybind11::none();
        } else {
            args[ "lower" ] = lowervalue;
        }
        // 0.0 为哨兵值，表示不限制该边界
        if (uppervalue == 0.0) {
            args[ "upper" ] = pybind11::none();
        } else {
            args[ "upper" ] = uppervalue;
        }
        args[ "axis" ] = pybind11::int_(axis);
        da_clip_outlier(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief query方法的wrapper
 * @param df
 * @param expr
 * @return
 */
bool DAPyScriptsDataFrame::queryDatas(DAPyDataFrame& df, const QString& expr) noexcept
{
    try {
        if (expr.isEmpty()) {
            return false;
        }
        pybind11::object da_query_datas = attr("da_query_datas");
        da_query_datas(df.object(), DA::PY::toPyObject(expr));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

namespace
{
/**
 * @brief python 整型数组（numpy int32/int64 或 list）→ QVector<int>（缓冲整体拷贝，避免逐元素 cast）
 */
QVector< int > pyIntSequenceToVector(const pybind11::object& obj)
{
    QVector< int > res;
    if (obj.is_none()) {
        return res;
    }
    pybind11::array_t< int, pybind11::array::c_style | pybind11::array::forcecast > arr(obj);
    const auto buf = arr.request();
    const int* ptr = static_cast< const int* >(buf.ptr);
    const int n    = static_cast< int >(buf.size);
    res.reserve(n);
    for (int i = 0; i < n; ++i) {
        res.append(ptr[ i ]);
    }
    return res;
}

/**
 * @brief 行/列范围闭区间 → python tuple（nullopt → None）
 */
pybind11::object rangeToPyTuple(const std::optional< QPair< int, int > >& range)
{
    if (!range) {
        return pybind11::none();
    }
    return pybind11::make_tuple(range->first, range->second);
}
}

/**
 * @brief search方法的wrapper（扩展选项，§4.2）
 *
 * 对应 da_dataframe.da_search_data：大小写/正则/整格匹配 + 行/列范围闭区间；
 * 命中上限 maxHits 截断（硬约束③），totalHits 返回真实命中总数；
 * 无效正则等 Python 抛错时 ok=false（经 dealException 记录）。
 * @param df 目标数据
 * @param value 关键字（空串直接返回空结果）
 * @param caseSensitive 区分大小写
 * @param useRegex 正则（只作用文本类列）
 * @param wholeCell 整格匹配
 * @param rowRange 行范围闭区间（nullopt=全表）
 * @param colRange 列范围闭区间（nullopt=全表）
 * @param maxHits 命中上限（默认 10 万）
 * @return 紧凑命中数组 + 真实总数
 */
DAPyScriptsDataFrame::SearchDataResult DAPyScriptsDataFrame::searchData(const DAPyDataFrame& df,
                                                                        const QString& value,
                                                                        bool caseSensitive,
                                                                        bool useRegex,
                                                                        bool wholeCell,
                                                                        const std::optional< QPair< int, int > >& rowRange,
                                                                        const std::optional< QPair< int, int > >& colRange,
                                                                        int maxHits) noexcept
{
    SearchDataResult res;
    try {
        if (value.isEmpty()) {
            res.ok = true;
            return res;
        }
        pybind11::object da_search_data = attr("da_search_data");
        pybind11::dict args;
        args[ "case_sensitive" ] = caseSensitive;
        args[ "use_regex" ]      = useRegex;
        args[ "whole_cell" ]     = wholeCell;
        args[ "row_range" ]      = rangeToPyTuple(rowRange);
        args[ "col_range" ]      = rangeToPyTuple(colRange);
        args[ "max_hits" ]       = maxHits;
        pybind11::object result  = da_search_data(df.object(), DA::PY::toPyObject(value), pybind11::none(), pybind11::none(), **args);
        pybind11::tuple t        = result.cast< pybind11::tuple >();
        res.rows                 = pyIntSequenceToVector(t[ 0 ]);
        res.cols                 = pyIntSequenceToVector(t[ 1 ]);
        res.totalHits            = t[ 2 ].cast< int >();
        res.ok                   = true;
        return res;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return res;
}

/**
 * @brief 查找替换的wrapper（对应 da_find_and_replace）
 *
 * 按与 searchData 同一匹配语义就地替换，变更清单输出到出参（D4 值记录撤销）；
 * old/new 值为 Python list（元素为原生标量），供 da_write_cells 撤销/重做写回。
 * 出参可为空指针（超阈值降级路径只要替换结果、不要清单）。
 * @return 成功返回true；正则无效等抛错返回false（经dealException记录）
 */
bool DAPyScriptsDataFrame::findReplace(DAPyDataFrame& df,
                                       const QString& find,
                                       const QString& replace,
                                       bool caseSensitive,
                                       bool useRegex,
                                       bool wholeCell,
                                       const std::optional< QPair< int, int > >& rowRange,
                                       const std::optional< QPair< int, int > >& colRange,
                                       QVector< int >* rows,
                                       QVector< int >* cols,
                                       pybind11::list* oldValues,
                                       pybind11::list* newValues) noexcept
{
    if (find.isEmpty()) {
        return false;
    }
    try {
        pybind11::object da_find_and_replace = attr("da_find_and_replace");
        pybind11::dict args;
        args[ "case_sensitive" ] = caseSensitive;
        args[ "use_regex" ]      = useRegex;
        args[ "whole_cell" ]     = wholeCell;
        args[ "row_range" ]      = rangeToPyTuple(rowRange);
        args[ "col_range" ]      = rangeToPyTuple(colRange);
        pybind11::object result  = da_find_and_replace(df.object(), DA::PY::toPyObject(find), DA::PY::toPyObject(replace), **args);
        pybind11::tuple t        = result.cast< pybind11::tuple >();
        if (rows) {
            *rows = pyIntSequenceToVector(t[ 0 ]);
        }
        if (cols) {
            *cols = pyIntSequenceToVector(t[ 1 ]);
        }
        if (oldValues) {
            *oldValues = t[ 2 ].cast< pybind11::list >();
        }
        if (newValues) {
            *newValues = t[ 3 ].cast< pybind11::list >();
        }
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 按变更清单批量写回单元格值的wrapper（对应 da_write_cells）
 * @param df 数据（就地修改）
 * @param rows 行位置
 * @param cols 列位置
 * @param values 值（Python list，与 rows/cols 一一对应）
 * @return 成功返回true
 */
bool DAPyScriptsDataFrame::writeCells(DAPyDataFrame& df, const QVector< int >& rows, const QVector< int >& cols, const pybind11::list& values) noexcept
{
    if (rows.isEmpty()) {
        return true;
    }
    try {
        pybind11::object da_write_cells = attr("da_write_cells");
        da_write_cells(df.object(), DA::PY::toPyObject(rows), DA::PY::toPyObject(cols), values);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**

 * @param df
 * @param expr
 * @return
 */
bool DAPyScriptsDataFrame::evalDatas(DAPyDataFrame& df, const QString& expr) noexcept
{
    try {
        if (expr.isEmpty()) {
            return false;
        }
        pybind11::object da_eval_datas = attr("da_eval_datas");
        da_eval_datas(df.object(), DA::PY::toPyObject(expr));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 多键稳定排序的wrapper
 *
 * 对应 da_dataframe.da_sort(df, keys)：keys 为 (列位置, 是否升序) 列表，
 * 顺序即优先级（第一个是主键）。Python 侧就地重排并返回置换
 * perm（perm[i] = 新表第 i 行在旧表中的行号，与 DATableKeyRemap /
 * DATableStyleManager::onRowsReordered 的置换语义一致）。
 * @param df 数据（就地重排）
 * @param keys (列位置, 是否升序) 列表
 * @param perm 非空时输出置换；空表时输出空列表
 * @return 成功返回true；列越界/混合类型不可比较等抛错返回false（经dealException记录）
 */
bool DAPyScriptsDataFrame::sort(DAPyDataFrame& df, const QVector< QPair< int, bool > >& keys, QVector< int >* perm) noexcept
{
    if (keys.isEmpty()) {
        return false;
    }
    try {
        pybind11::object da_sort = attr("da_sort");
        pybind11::list pykeys;
        for (const auto& k : keys) {
            pykeys.append(pybind11::make_tuple(k.first, k.second));
        }
        pybind11::object result = da_sort(df.object(), pykeys);
        if (perm) {
            perm->clear();
            if (!result.is_none()) {
                const int n = static_cast< int >(pybind11::len(result));
                perm->reserve(n);
                for (auto item : result) {
                    perm->append(item.cast< int >());
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 按给定置换就地重排行的wrapper
 *
 * 对应 da_dataframe.da_reorder_rows(df, perm)：new[i] = old[perm[i]]，
 * 排序命令的 redo/undo 用它施加正置换/逆置换，不重新计算排序。
 * @param df 数据（就地重排）
 * @param perm 置换（perm[i] = 新表第 i 行在旧表中的行号）；空置换视为无操作
 * @return 成功返回true
 */
bool DAPyScriptsDataFrame::reorderRows(DAPyDataFrame& df, const QVector< int >& perm) noexcept
{
    if (perm.isEmpty()) {
        return true;
    }
    try {
        pybind11::object da_reorder_rows = attr("da_reorder_rows");
        da_reorder_rows(df.object(), DA::PY::toPyObject(perm));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 单列统计（min/max/median）的wrapper
 *
 * 对应 da_dataframe.da_column_stats(df, column)：全表 O(n) 归约（NaN 排除，
 * 不采样不近似，003 条件格式色阶/数据条端点）。Python 侧对全 NaN/非数值列
 * 返回 None → ok=false；列越界抛 IndexError → ok=false（经 dealException 记录）。
 * @param df 数据（只读）
 * @param col 列位置索引
 * @return 统计结果（ok=false 表示不可用）
 */
DAPyScriptsDataFrame::ColumnStatsResult DAPyScriptsDataFrame::columnStats(const DAPyDataFrame& df, int col) noexcept
{
    ColumnStatsResult result;
    if (df.isNone()) {
        return result;
    }
    try {
        pybind11::object da_column_stats = attr("da_column_stats");
        pybind11::object ret             = da_column_stats(df.object(), col);
        if (ret.is_none()) {
            return result;  // 全 NaN / 非数值列：统计不可用
        }
        pybind11::tuple t = ret.cast< pybind11::tuple >();
        if (t.size() != 3) {
            return result;
        }
        result.minValue    = t[ 0 ].cast< double >();
        result.maxValue    = t[ 1 ].cast< double >();
        result.medianValue = t[ 2 ].cast< double >();
        result.ok          = true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return result;
}

/**
 * @brief select方法的wrapper
 * @param df
 * @param contents
 * @return
 */
bool DAPyScriptsDataFrame::dataselect(DAPyDataFrame& df, double lowervalue, double uppervalue, const QString& index) noexcept
{
    try {
        pybind11::object da_data_select = attr("da_data_select");
        pybind11::dict args;
        // 0.0 为哨兵值，表示不限制该边界
        if (lowervalue == 0.0) {
            args[ "lower" ] = pybind11::none();
        } else {
            args[ "lower" ] = lowervalue;
        }
        // 0.0 为哨兵值，表示不限制该边界
        if (uppervalue == 0.0) {
            args[ "upper" ] = pybind11::none();
        } else {
            args[ "upper" ] = uppervalue;
        }
        args[ "index" ] = DA::PY::toPyObject(index);
        da_data_select(df.object(), **args);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief pivot_table方法的wrapper
 * @param values 要进行汇总的数据值
 * @param index 确定行参数
 * @param columns 确定列参数
 * @param aggfunc 要计算的函数，mean求均值、sum求和、size计算个数
 * @param margins 行列数据的统计
 * @param sort 聚合后的结果排序
 * @return DAPyDataFrame
 */
DAPyDataFrame DAPyScriptsDataFrame::pivotTable(const DAPyDataFrame& df,
                                               const QStringList& values,
                                               const QStringList& index,
                                               const QStringList& columns,
                                               const QString& aggfunc,
                                               bool margins,
                                               const QString& marginsName,
                                               bool sort) noexcept
{
    try {
        pybind11::object da_pivot_table = attr("da_create_pivot_table");
        pybind11::dict args;
        if (values.empty()) {
            args[ "values" ] = pybind11::none();
        } else {
            args[ "values" ] = DA::PY::toPyObject(values);
        }
        if (index.empty()) {
            args[ "index" ] = pybind11::none();
        } else {
            args[ "index" ] = DA::PY::toPyObject(index);
        }
        if (columns.empty()) {
            args[ "columns" ] = pybind11::none();
        } else {
            args[ "columns" ] = DA::PY::toPyObject(columns);
        }
        args[ "aggfunc" ]      = DA::PY::toPyObject(aggfunc);
        args[ "margins" ]      = margins;
        args[ "margins_name" ] = DA::PY::toPyObject(marginsName);
        args[ "sort" ]         = sort;

        return da_pivot_table(df.object(), **args);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyDataFrame();
}

}  // end DA namespace
