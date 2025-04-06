#include "DAPyDataFrame.h"
#include <vector>
#include <map>
#include <QHash>
#include "../numpy/DAPyModuleNumpy.h"
#include "DAPyModulePandas.h"
#include "DAPybind11QtTypeCast.h"
#include "DAPyModulePandas.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyDataFrame
//===================================================
DAPyDataFrame::DAPyDataFrame() : DAPyObjectWrapper()
{
	try {
		auto pandas = DAPyModule("pandas");
		_object     = pandas.attr("DataFrame")();
	} catch (const std::exception& e) {
		qCritical() << "can not import pandas,or can not create pandas.Dataframe(),because:" << e.what();
	}
}

DAPyDataFrame::DAPyDataFrame(const DAPyDataFrame& df) : DAPyObjectWrapper(df)
{
	checkObjectValid();
}

DAPyDataFrame::DAPyDataFrame(const DAPyObjectWrapper& df) : DAPyObjectWrapper(df)
{
	checkObjectValid();
}

DAPyDataFrame::DAPyDataFrame(DAPyDataFrame&& df) : DAPyObjectWrapper(std::move(df))
{
	checkObjectValid();
}

DAPyDataFrame::DAPyDataFrame(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
	checkObjectValid();
}

DAPyDataFrame::DAPyDataFrame(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
	checkObjectValid();
}

DAPyDataFrame::~DAPyDataFrame()
{
}

DAPySeries DAPyDataFrame::operator[](const QString& n) const
{
	try {
		pybind11::object obj = object()[ DA::PY::toPyStr(n) ];
		return DAPySeries(obj);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPySeries();
}

DAPySeries DAPyDataFrame::operator[](int n) const
{
	try {
		auto headers = columns();
		if (n < 0 || n >= headers.size()) {
			return DAPySeries();
		}
		QString name = headers[ n ];
		return operator[](name);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPySeries();
}

DAPyDataFrame& DAPyDataFrame::operator=(const pybind11::object& obj)
{
	_object = obj;
	checkObjectValid();
	return *this;
}

DAPyDataFrame& DAPyDataFrame::operator=(pybind11::object&& obj)
{
	_object = std::move(obj);
	checkObjectValid();
	return *this;
}

DAPyDataFrame& DAPyDataFrame::operator=(const DAPyDataFrame& obj)
{
	if (this != &obj) {
		DAPyObjectWrapper::operator=(obj);  // 调用基类赋值
		checkObjectValid();
	}
	return *this;
}

DAPyDataFrame& DAPyDataFrame::operator=(DAPyDataFrame&& obj)
{
	if (this != &obj) {
		DAPyObjectWrapper::operator=(std::move(obj));  // 调用基类赋值
		checkObjectValid();
	}
	return *this;
}

DAPyDataFrame& DAPyDataFrame::operator=(const DAPyObjectWrapper& obj)
{
	DAPyObjectWrapper::operator=(obj);
	checkObjectValid();
	return *this;
}

DAPyDataFrame& DAPyDataFrame::operator=(DAPyObjectWrapper&& obj)
{
	DAPyObjectWrapper::operator=(std::move(obj));
	checkObjectValid();
	return *this;
}
/**
 * @brief DataFrame.columns
 * @return
 */
QList< QString > DAPyDataFrame::columns() const
{
	QList< QString > res;
	try {
		pybind11::list obj_columns = object().attr("columns");
		const size_t s             = obj_columns.size();
		for (size_t i = 0; i < s; ++i) {
			pybind11::str obj_str = obj_columns[ i ];
			res.append(DA::PY::toString(obj_str));
		}
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return res;
}

/**
 * @brief 设置列名
 * @param i
 * @param name
 * @return
 */
bool DAPyDataFrame::columns(std::size_t i, const QString& name)
{
	try {
		pybind11::list obj_columns = object().attr("columns");
		const size_t s             = obj_columns.size();
		if (i >= s) {
			return false;
		}
		obj_columns[ i ]         = DA::PY::toPyStr(name);
		object().attr("columns") = obj_columns;
		return true;
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return false;
}

/**
 * @brief 设置阵列内容
 * @param cols
 * @return
 */
bool DAPyDataFrame::columns(const QList< QString >& cols)
{
	try {
		pybind11::list obj_columns = PY::toPyList(cols);

		object().attr("columns") = obj_columns;
		return true;
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return false;
}

/**
 * @brief DADataFrame::empty property DataFrame.empty
 * @return
 */
bool DAPyDataFrame::empty() const
{
	try {
		pybind11::bool_ obj = object().attr("empty");
		bool r              = obj.cast< bool >();
		return r;
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return true;
}

/**
 * @brief DADataFrame::shape property DataFrame.shape(row,column)
 * @return
 */
std::pair< std::size_t, std::size_t > DAPyDataFrame::shape() const
{
	try {
		pybind11::tuple obj = object().attr("shape");
		std::size_t row     = obj[ 0 ].cast< std::size_t >();
		std::size_t col     = obj[ 1 ].cast< std::size_t >();
		return std::make_pair(row, col);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return std::make_pair(0, 0);
}

/**
 * @brief DADataFrame::size property DataFrame.size
 * @return  return the number of rows times number of columns if DataFrame.
 */
std::size_t DAPyDataFrame::size() const
{
	try {
		pybind11::int_ obj = object().attr("size");
		return obj.cast< std::size_t >();
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return 0;
}

/**
 * @brief DataFrame.iat Access a single value for a row/column pair by integer position.
 * @param r row
 * @param c column
 * @return 如果没有或超出范围，返回QVariant(),此函数不会做isNone判断
 */
QVariant DAPyDataFrame::iat(std::size_t r, std::size_t c) const
{
	try {
		pybind11::object obj_v = object().attr("iat")[ pybind11::make_tuple(r, c) ];
		return DA::PY::toVariant(obj_v);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return QVariant();
}

/**
 * @brief DataFrame.iat Access a single value for a row/column pair by integer position.
 * @param r
 * @param c
 * @return
 */
pybind11::object DAPyDataFrame::iatObj(std::size_t r, std::size_t c) const
{
	try {
		return object().attr("iat")[ pybind11::make_tuple(r, c) ];
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return pybind11::none();
}

/**
 * @brief DataFrame.iat Access a single value for a row/column pair by integer position.
 * @param r row
 * @param c column
 * @param v 需要设置的参数值，由于类似于 pandas.iat[r,c] = v
 * @return 如果设置成功，返回true
 */
bool DAPyDataFrame::iat(std::size_t r, std::size_t c, const QVariant& v)
{
	try {
		object().attr("iat")[ pybind11::make_tuple(r, c) ] = DA::PY::toPyObject(v, dtypes(c));
		return true;
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return false;
}

bool DAPyDataFrame::iat(std::size_t r, std::size_t c, const pybind11::object& v)
{
	try {
		object().attr("iat")[ pybind11::make_tuple(r, c) ] = v;
		return true;
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return false;
}

DAPySeries DAPyDataFrame::iloc(std::size_t c) const
{
	try {
		pybind11::object obj_series = object().attr("iloc")[ pybind11::int_(c) ];
		return DAPySeries(obj_series);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPySeries();
}

/**
 * @brief DADataFrame::loc
 * @param n
 * @return
 */
DAPySeries DAPyDataFrame::loc(const QString& n) const
{
	try {
		pybind11::object obj = object().attr("iloc")[ DA::PY::toPyStr(n) ];
		return DAPySeries(obj);
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPySeries();
}

/**
 * @brief DataFrame.drop(index=index,axis=axis,inplace=True)
 * @param index
 * @param axis
 * @return
 */
bool DAPyDataFrame::drop(std::size_t index, int axis)
{
	try {
		pybind11::object obj_drop = object().attr("drop");
		using namespace pybind11::literals;
		obj_drop("index"_a = pybind11::int_(index), "axis"_a = pybind11::int_(axis), "inplace"_a = pybind11::bool_(true));
		return true;
	} catch (const std::exception& e) {
		dealException(e);
	}
	return false;
}

/**
 * @brief DAPyDataFrame::index
 * @return
 */
DAPyIndex DAPyDataFrame::index() const
{
	try {
		return DAPyIndex(object().attr("index"));
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPyIndex();
}

/**
 * @brief 获取某一列对应的dtype,等同于pandas.DataFrame.dtypes[c]
 * @param c
 * @return
 * @note 注意，此函数会抛出异常
 */
pybind11::dtype DAPyDataFrame::dtypes(std::size_t c) const
{
    return object().attr("dtypes")[ pybind11::int_(c) ];
}

/**
 * @brief pandas.DataFrame.describe
 * @return
 */
DAPyDataFrame DAPyDataFrame::describe() const
{
	try {
		return object().attr("describe")();
	} catch (const std::exception& e) {
		qCritical().noquote() << e.what();
	}
	return DAPyDataFrame();
}

void DAPyDataFrame::checkObjectValid()
{
	if (!isDataFrame(object())) {
		object() = pybind11::none();
		//        qCritical() << QObject::tr(
		//                "DADataFrame(const pybind11::object& obj) get python object type is not pandas.DataFrame");
	} else {
	}
}

/**
 * @brief 打印为字符串
 * @param maxrow
 * @return
 */
QString DAPyDataFrame::toString(std::size_t maxrow) const
{
	if (isNone()) {
		return QString();
	}
	std::pair< std::size_t, std::size_t > sh = shape();

	std::size_t printRowCnt = sh.first + 3;
	QList< QList< QString > > strlist;  // std::vector<列字符>
	QList< QString > headers = columns();
	for (int c = 0; c < sh.second; ++c) {
		// 先获取名字和类型名称
		strlist.push_back({ "----", headers[ c ], "----" });
	}
	if (sh.first > maxrow) {
		// 说明需要截断
		int showrowCnt = (int)maxrow / 2;
		for (int c = 0; c < sh.second; ++c) {
			// 头部内容推入
			for (int r = 0; r < showrowCnt; ++r) {
				strlist[ c ].push_back(iat(r, c).toString());
			}
			strlist[ c ].push_back("...");
			// 尾部内容推入
			for (int r = (int)sh.first - 1 - showrowCnt; r < (int)sh.first; ++r) {
				strlist[ c ].push_back(iat(r, c).toString());
			}
		}
	} else {
		// 全部推入
		for (int c = 0; c < sh.second; ++c) {
			for (int r = 0; r < sh.first; ++r) {
				strlist[ c ].push_back(iat(r, c).toString());
			}
		}
	}
	// 确定字符串最大长度，并对字符串的长度进行匹配
	for (QList< QString >& vecColStr : strlist) {
		int s = 0;
		for (const QString& str : vecColStr) {
			if (str.size() > s) {
				s = str.size();
			}
		}
		for (int i = 0; i < vecColStr.size(); ++i) {
			if (0 == i || 3 == i) {
				vecColStr[ i ].fill('-', s);
			}
			vecColStr[ i ] = vecColStr[ i ].leftJustified(s);
		}
		printRowCnt = vecColStr.size();
	}
	// 拼接为完整的字符串
	QString res;
	for (int r = 0; r < printRowCnt; ++r) {
		// 逐行打印
		res += "| ";
		for (const QList< QString >& vecColStr : strlist) {
			res += vecColStr[ r ] + " | ";
		}
		res += "\n";
	}
	return res;
}

bool DAPyDataFrame::isDataFrame(const pybind11::object& obj)
{
	return DAPyModulePandas::getInstance().isInstanceDataFrame(obj);
}

bool DAPyDataFrame::read_csv(const QString& path, const QVariantHash args, QString* why)
{
	DAPyDataFrame df = DAPyModulePandas::getInstance().read_csv(path, args);
	if (!df.isNone()) {
		*this = std::move(df);
		return true;
	} else {
		if (why) {
			*why = DAPyModulePandas::getInstance().getLastErrorString();
		}
	}
	return false;
}

/**
 * @brief 通过此操作符可直接进行true/false的判断
 * @code
 *  DADataFrame df = DAPandas::getInstance().read_csv(
 *               QStringLiteral("F:/work/[07]数据挖掘/挖掘结果汇总/不同气候带不同月份发生时间.csv"));
 *   if (!df) {
 *       qDebug().noquote() << DAPandas::getInstance().getLastErrorString();
 *   }
 * @endcode
 */
// DAPyDataFrame::operator bool() const
//{
//    return !isNone();
//}

QDebug operator<<(QDebug dbg, const DAPyDataFrame& df)
{
	QDebugStateSaver saver(dbg);
	dbg.noquote() << df.toString();
	return (dbg);
}
