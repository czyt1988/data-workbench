#include "DAPyScriptsDataFrame.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>
namespace DA
{

//===================================================
// DAPyScriptsDataFrame
//===================================================
DAPyScriptsDataFrame::DAPyScriptsDataFrame(bool autoImport) : DAPyModule()
{
	if (autoImport) {
		if (!import()) {
			qCritical() << QObject::tr("can not import da_dataframe module");
		}
	}
}

DAPyScriptsDataFrame::DAPyScriptsDataFrame(const pybind11::object& obj) : DAPyModule(obj)
{
	if (isModule()) {
		qCritical() << QObject::tr("can not import DAWorkBench.io");
	}
}

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
		da_drop_irow(df.object(), DA::PY::toList(index));
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
		da_drop_icolumn(df.object(), DA::PY::toList(index));
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
		args[ "name" ] = DA::PY::toString(name);
		if (defaultvalue.isValid()) {
			args[ "defaultvalue" ] = DA::PY::toPyObject(defaultvalue);
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
		args[ "name" ]  = DA::PY::toString(name);
		args[ "start" ] = DA::PY::toPyObject(start);
		args[ "stop" ]  = DA::PY::toPyObject(stop);
		if (start.canConvert(QMetaType::QDateTime) || start.canConvert(QMetaType::QDate)
			|| start.canConvert(QMetaType::QTime)) {
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
 * @brief 保存为pickle 对应da_dataframe.da_to_pickle
 * @param df
 * @param path
 * @return
 */
bool DAPyScriptsDataFrame::to_pickle(const DAPyDataFrame& df, const QString& path) noexcept
{
	try {
		pybind11::object da_to_pickle = attr("da_to_pickle");
		da_to_pickle(df.object(), DA::PY::toString(path));
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
		da_from_pickle(df.object(), DA::PY::toString(path));
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
bool DAPyScriptsDataFrame::dropna(DAPyDataFrame& df, int axis, const QString& how, const QList< int >& indexs, int thresh)
{
	try {
		pybind11::object da_drop_na = attr("da_drop_na");
		pybind11::object threshobj  = pybind11::none();
		if (thresh > 0) {
			threshobj = pybind11::int_(thresh);
		}
		pybind11::dict args;
		args[ "axis" ]   = axis;
		args[ "how" ]    = DA::PY::toString(how);
		args[ "index" ]  = DA::PY::toList(indexs);
		args[ "thresh" ] = threshobj;
		da_drop_na(df.object(), **args);
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
bool DAPyScriptsDataFrame::dropduplicates(DAPyDataFrame& df, const QString& keep, const QList< int >& indexs)
{
	try {
		pybind11::object da_drop_duplicates = attr("da_drop_duplicates");
		pybind11::dict args;
		args[ "keep" ]  = DA::PY::toString(keep);
		args[ "index" ] = DA::PY::toList(indexs);
		da_drop_duplicates(df.object(), **args);
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
bool DAPyScriptsDataFrame::fillna(DAPyDataFrame& df, double value, int limit)
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
 * @brief interpolate方法的wrapper
 * @param df
 * @param method 插值填充的方法
 * @param order 多项式插值的次数
 * @param limit 填充限制，-1代表不限制，大于0生效
 * @return
 */
bool DAPyScriptsDataFrame::interpolate(DAPyDataFrame& df, const QString& method, int order, int limit)
{
	try {
		pybind11::object da_interpolate = attr("da_fill_interpolate");
		pybind11::object limitObj       = pybind11::none();
		if (limit > 0) {
			limitObj = pybind11::int_(limit);
		}
		pybind11::dict args;
		args[ "method" ] = DA::PY::toString(method);
		args[ "order" ]  = order;
		args[ "limit" ]  = limitObj;
		da_interpolate(df.object(), **args);
		return true;
	} catch (const std::exception& e) {
		dealException(e);
	}
	return false;
}
}  // end DA namespace
