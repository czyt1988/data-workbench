#include "DAPyModulePandas.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>
#include <QObject>
namespace DA
{
class DAPyModulePandasPrivate
{
public:
    DA_IMPL_PUBLIC(DAPyModulePandas)
    DAPyModulePandasPrivate(DAPyModulePandas* p);

    //释放模块
    void del();

public:
    QString _lastErrorString;
    pybind11::object _seriesType;
    pybind11::object _dataFrameType;
    pybind11::object _indexType;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyModulePandasPrivate
//===================================================
DAPyModulePandasPrivate::DAPyModulePandasPrivate(DAPyModulePandas* p) : q_ptr(p)
{
}

void DAPyModulePandasPrivate::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object() = pybind11::none();
}
//===================================================
// DAPyModulePandas
//===================================================
DAPyModulePandas::DAPyModulePandas() : DAPyModule(), d_ptr(new DAPyModulePandasPrivate(this))
{
    import();
    try {
        d_ptr->_seriesType    = attr("Series");
        d_ptr->_dataFrameType = attr("DataFrame");
        d_ptr->_indexType     = attr("Index");
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
}

DAPyModulePandas::~DAPyModulePandas()
{
}

DAPyModulePandas& DAPyModulePandas::getInstance()
{
    static DAPyModulePandas s_pandas;
    return s_pandas;
}

void DAPyModulePandas::finalize()
{
    d_ptr->del();
}

/**
 * @brief 获取最后的错误
 * @return
 */
QString DAPyModulePandas::getLastErrorString()
{
    return d_ptr->_lastErrorString;
}

/**
 * @brief 导入模块
 * @return
 */
bool DAPyModulePandas::import()
{
    return DAPyModule::import("pandas");
}

bool DAPyModulePandas::isInstanceSeries(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_seriesType);
}

bool DAPyModulePandas::isInstanceDataFrame(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_dataFrameType);
}

bool DAPyModulePandas::isInstanceIndex(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_indexType);
}

bool DAPyModulePandas::isInstanceDataFrame_(const pybind11::object& obj)
{
    try {
        pybind11::module m         = pybind11::module::import("pandas");
        pybind11::object dataframe = m.attr("DataFrame");
        return pybind11::isinstance(obj, dataframe);
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}
/**
 * @brief 对pandas.read_csv的封装
 * pandas.read_csv(filepath_or_buffer, sep=NoDefault.no_default, delimiter=None, header='infer', names=NoDefault.no_default, index_col=None, usecols=None, squeeze=None,
 * prefix=NoDefault.no_default, mangle_dupe_cols=True, dtype=None, engine=None, converters=None, true_values=None, false_values=None, skipinitialspace=False, skiprows=None,
 * skipfooter=0, nrows=None, na_values=None, keep_default_na=True, na_filter=True, verbose=False, skip_blank_lines=True, parse_dates=None, infer_datetime_format=False,
 * keep_date_col=False, date_parser=None, dayfirst=False, cache_dates=True, iterator=False, chunksize=None, compression='infer', thousands=None, decimal='.', lineterminator=None,
 * quotechar='"', quoting=0, doublequote=True, escapechar=None, comment=None, encoding=None, encoding_errors='strict', dialect=None, error_bad_lines=None, warn_bad_lines=None,
 * on_bad_lines=None, delim_whitespace=False, low_memory=True, memory_map=False, float_precision=None, storage_options=None)
 * @param path
 *
 * @code
 * DAPyDataFrame df = DAPyModulePandas::getInstance()
 *                            .read_csv(QStringLiteral("F:/数据文件.csv"),
 *                                      { { "encoding", "ANSI" } });
 * if (!df) {
 *     qDebug().noquote() << DAPyModulePandas::getInstance().getLastErrorString();
 * }
 * @endcode
 * @return
 */
DAPyDataFrame DAPyModulePandas::read_csv(const QString& path, const QVariantHash& args)
{
    if (!isImport()) {
        if (!import()) {
            return DAPyDataFrame();
        }
    }
    try {
        pybind11::object obj_read_csv = attr("read_csv");
        pybind11::str a0              = DA::PY::toString(path);
        pybind11::object obj_df;
        if (args.contains("encoding")) {
            //说明用户已经制定编码
            pybind11::dict a1 = DA::PY::toDict(args);
            obj_df            = obj_read_csv(a0, **a1);
            DAPyDataFrame df(obj_df);
            return df;
        } else {
            //用户没有指定编码，这里做一个编码检测（主要针对中文windows系统的ansi编码）
            try {
                //先用默认utf-8编码打开
                pybind11::dict a1 = DA::PY::toDict(args);
                obj_df            = obj_read_csv(a0, **a1);
                DAPyDataFrame df(obj_df);
                return df;
            } catch (const std::exception& e) {
                //不行就用ansi编码打开
                Q_UNUSED(e);
                qWarning() << QObject::tr("use utf-8 open file %1 error,try to use ansi encoding").arg(path);
                QVariantHash t    = args;
                t[ "encoding" ]   = QString("ANSI");
                pybind11::dict a1 = DA::PY::toDict(t);
                obj_df            = obj_read_csv(a0, **a1);
                DAPyDataFrame df(obj_df);
                return df;
            }
        }

    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
    return DAPyDataFrame();
}
