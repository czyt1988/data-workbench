#include "DAPybind11QtTypeCast.h"
#include <chrono>
#include <QDateTime>
#include <QUuid>
#include <QUrl>
#include "DAPyModuleDatetime.h"
#include "numpy/DAPyModuleNumpy.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include "pandas/DAPyIndex.h"
namespace DA
{
namespace PY
{

/**
 * @brief toVariant
 *
 * 针对numpy的char进行如下操作：
 * 类型|kind|char
 * bool b ?
 * int8 i b
 * int16 i h
 * int32 i l
 * int64 i q
 * uint8 u B
 * uint16 u H
 * uint32 u L
 * uint64 u Q
 * float16 f e
 * float32 f f
 * float64 f d
 * complex64 c F
 * complex128 c D
 * <U0 U U
 * datetime64 M M
 * timedelta64 m m
 * |S0 S S
 * |V0 V V
 * object O O
 * @param obj
 * @return
 */
QVariant toVariant(const pybind11::object& obj)
{
	if (obj.is_none()) {
		return QVariant();
	}
	DAPyModuleNumpy& np = DAPyModuleNumpy::getInstance();
	// 对numpy的转换
	if (np.isInstanceGeneric(obj)) {
		// 说明是numpy的类型,获取dtype
		pybind11::dtype dt = obj.attr("dtype");
		// 根据dtype进行转换
		switch (dt.char_()) {
		case '?':
			return obj.cast< bool >();
		case 'b':
			return QChar(obj.cast< char >());
		case 'h':
		case 'l':
			return obj.cast< int >();
		case 'q':
			return obj.cast< long long >();
		case 'B':
			return QChar(obj.cast< unsigned char >());
		case 'H':
		case 'L':
			return obj.cast< unsigned int >();
		case 'Q':
			return obj.cast< unsigned long long >();
		case 'e':
		case 'f':
			return obj.cast< float >();
		case 'd':
			return obj.cast< double >();
		case 'U':
		case 'S':
			return QString::fromStdString(obj.cast< std::string >());
		case 'M': {
			std::chrono::system_clock::time_point dt = obj.cast< std::chrono::system_clock::time_point >();
			auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(dt.time_since_epoch());
			return QDateTime::fromMSecsSinceEpoch(ms.count());
		}
		default:
			break;
		}
		return QVariant();
	}
	//    obj.ptr()

	if (pybind11::isinstance< pybind11::str >(obj)) {
		return QString::fromStdString(obj.cast< std::string >());
	} else if (pybind11::isinstance< pybind11::int_ >(obj)) {
		return obj.cast< long long >();
	} else if (pybind11::isinstance< pybind11::float_ >(obj)) {
		return obj.cast< double >();
	} else if (DAPyModuleDatetime::getInstance().isInstanceDateTime(obj)) {
		try {
			pybind11::int_ y  = obj.attr("year");
			pybind11::int_ m  = obj.attr("month");
			pybind11::int_ d  = obj.attr("day");
			pybind11::int_ h  = obj.attr("hour");
			pybind11::int_ mm = obj.attr("minute");
			pybind11::int_ ss = obj.attr("second");
			return QDateTime(QDate(int(y), int(m), int(d)), QTime(int(h), int(mm), int(ss)));
		} catch (...) {
			return QVariant();
		}
	} else if (DAPyModuleDatetime::getInstance().isInstanceTime(obj)) {
		try {
			pybind11::int_ h  = obj.attr("hour");
			pybind11::int_ mm = obj.attr("minute");
			pybind11::int_ ss = obj.attr("second");
			pybind11::int_ ms = obj.attr("microsecond");
			return QTime(int(h), int(mm), int(ss), int(ms));
		} catch (...) {
			return QVariant();
		}
	} else if (DAPyModuleDatetime::getInstance().isInstanceDate(obj)) {
		try {
			pybind11::int_ y = obj.attr("year");
			pybind11::int_ m = obj.attr("month");
			pybind11::int_ d = obj.attr("day");
			return QDate(int(y), int(m), int(d));
		} catch (...) {
			return QVariant();
		}
	} else if (pybind11::isinstance< pybind11::list >(obj)) {
		pybind11::list l = obj;
		QVariantList vl;
		std::size_t s = l.size();
		for (int i = 0; i < s; ++i) {
			vl.append(toVariant(l[ i ]));
		}
		return vl;
	} else if (pybind11::isinstance< pybind11::tuple >(obj)) {
		pybind11::tuple l = obj;
		QVariantList vl;
		std::size_t s = l.size();
		for (int i = 0; i < s; ++i) {
			vl.append(toVariant(l[ i ]));
		}
		return vl;
	} else if (pybind11::isinstance< pybind11::dict >(obj)) {
		pybind11::dict d = obj;
		QVariantHash vh;
		for (auto i : d) {
			pybind11::str k      = pybind11::str(i.first);
			pybind11::object obj = pybind11::reinterpret_borrow< pybind11::object >(i.second);
			vh[ toString(k) ]    = toVariant(obj);
		}
		return vh;
	}
	return QVariant();
}

/**
 * @brief 把字符串按照dtype转换为qvariant
 *
 * 此函数主要用户获取用户输入的文本转换为对应的变量
 * @param str 字符串
 * @param dt dtype 如果是一个无效的dtype，将先尝试转换为数字，如果无法转换为数字直接转换为字符串
 * @return 如果转换失败返回QVariant()
 */
QVariant toVariant(const QString& str, const DAPyDType& dt)
{
	if (str.isEmpty()) {
		return QVariant();
	}
	if (dt.isNone()) {
		bool isok = false;
		{
			int v = str.toInt(&isok);
			if (isok) {
				return v;
			}
		}
		{
			double v = str.toDouble(&isok);
			if (isok) {
				return v;
			}
		}
		return str;
	}
	switch (dt.char_()) {
	case '?': {
		if (str.toLower() == "false" || str == "0") {
			return false;
		} else {
			return true;
		}
		break;
	}
	case 'b':
	case 'h': {
		bool isok = false;
		short v   = str.toShort(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'l': {
		bool isok = false;
		int v     = str.toInt(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'q': {
		bool isok   = false;
		long long v = str.toLongLong(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'B':
	case 'H': {
		bool isok        = false;
		unsigned short v = str.toUShort(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'L': {
		bool isok      = false;
		unsigned int v = str.toUInt(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'Q': {
		bool isok            = false;
		unsigned long long v = str.toULongLong(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'e':
	case 'f': {
		bool isok = false;
		float v   = str.toFloat(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'd': {
		bool isok = false;
		double v  = str.toDouble(&isok);
		if (isok) {
			return v;
		}
	} break;
	case 'M': {
		// 日期
		QDateTime datetime = QDateTime::fromString(str, "yyyy-MM-dd HH:mm:ss");
		if (datetime.isValid()) {
			return datetime;
		}
	} break;
	default:
		break;
	}
	return str;
}

QString toString(const pybind11::str& obj)
{
	return QString::fromStdString(std::string(obj));
}

pybind11::str toPyStr(const QString& str)
{
	std::string s(str.toUtf8().constData());
	return pybind11::str(s);
}

QString toString(const pybind11::dtype& dtype)
{
    return toString(dtype.cast< pybind11::str >());
}

/**
 * @brief QVariant 转换为pybind11::object
 * @param var
 * @return
 */
pybind11::object toPyObject(const QVariant& var)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	auto varypeid = var.type();
#else
	auto varypeid = var.typeId();
#endif
	switch (varypeid) {
	case QMetaType::UnknownType:
		return (pybind11::none());
	case QMetaType::Bool:
		return pybind11::bool_(var.toBool());
	case QMetaType::QBitArray: {
		QByteArray byte = var.toByteArray();
		return pybind11::bytes(byte.constData(), byte.size());
	}
	case QMetaType::Char:
		return pybind11::int_(var.toInt());
	case QMetaType::QTime:
		return DAPyModuleDatetime::getInstance().timeObject(var.toTime());
	case QMetaType::QDate:
		return DAPyModuleDatetime::getInstance().dateObject(var.toDate());
	case QMetaType::QDateTime:
		return DAPyModuleDatetime::getInstance().datetimeObject(var.toDateTime());
	case QMetaType::Double:
		return pybind11::float_(var.toDouble());
	case QMetaType::QUuid:
		return pybind11::str(var.toUuid().toString().toStdString());
	case QMetaType::QVariantHash:
		return toPyDict(var.toHash());
	case QMetaType::Int:
		return pybind11::int_(var.toInt());
	case QMetaType::QVariantList:
		return toPyList(var.toList());
	case QMetaType::LongLong:
		return pybind11::int_(var.toLongLong());
	case QMetaType::QVariantMap:
		return toPyDict(var.toMap());
	case QMetaType::QString:
		return toPyStr(var.toString());
	case QMetaType::QStringList:
		return toPyList(var.toStringList());
	case QMetaType::UInt:
		return (pybind11::int_(var.toUInt()));
	case QMetaType::ULongLong:
		return (pybind11::int_(var.toULongLong()));
	case QMetaType::QUrl:
		return pybind11::str(var.toUrl().toString().toStdString());
	default:
		break;
	}
	return pybind11::none();
}

/**
 * @brief 通过指定的dtype，把QVariant转换为pybind11::object
 * @param var QVariant
 * @param dt 指定的类型
 * @return
 */
pybind11::object toPyObject(const QVariant& var, const pybind11::dtype& dt)
{
    return dt.attr("type")(toPyObject(var));
}

pybind11::dict toPyDict(const QVariantHash& qvhash)
{
	pybind11::dict d;
	for (auto i = qvhash.begin(); i != qvhash.end(); ++i) {
		d[ toPyStr(i.key()) ] = toPyObject(i.value());
	}
	return d;
}

pybind11::dict toPyDict(const QVariantMap& qvmap)
{
	pybind11::dict d;
	for (auto i = qvmap.begin(); i != qvmap.end(); ++i) {
		d[ toPyStr(i.key()) ] = toPyObject(i.value());
	}
	return d;
}

pybind11::list toPyList(const QVariantList& list)
{
	pybind11::list d;
	for (const QVariant& v : list) {
		d.append(toPyObject(v));
	}
	return d;
}

pybind11::list toPyList(const QStringList& list)
{
	pybind11::list d;
	for (const QString& v : list) {
		d.append(toPyStr(v));
	}
	return d;
}

/**
 * @brief pybind11::object 转QList<QString>
 * @param obj
 * @param err
 * @return
 */
QList< QString > toStringList(const pybind11::object& obj, QString* err)
{
	QList< QString > res;
	try {
		pybind11::list sl = obj;
		for (auto i = sl.begin(); i != sl.end(); ++i) {
			QString a = QString::fromStdString((*i).cast< std::string >());
			res.append(a);
		}
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
	}
	return res;
}

/**
 * @brief 把常用的类型注册到元对象，此函数最好在初始化的时候调用
 */
void registerMetaType()
{
	qRegisterMetaType< DAPyObjectWrapper >("DAPyObjectWrapper");
	qRegisterMetaType< DAPyDType >("DAPyDType");
	qRegisterMetaType< DAPyDataFrame >("DAPyDataFrame");
	qRegisterMetaType< DAPyIndex >("DAPyIndex");
	qRegisterMetaType< DAPySeries >("DAPySeries");
}

/**
 * @brief  转换为列表的特化模板函数，T = QString
 * @param arr
 * @return
 */
template<>
inline pybind11::list toPyList< QString >(const QList< QString >& arr)
{
	pybind11::list pylist;
	for (const QString& v : arr) {
		pybind11::object o = toPyStr(v);
		pylist.append(o);
	}
	return pylist;
}

/**
 * @brief  转换为列表的特化模板函数，T = QString
 * @param arr
 * @return
 */
template<>
inline pybind11::list toPyList< QString >(const QSet< QString >& arr)
{
	pybind11::list pylist;
	for (const QString& v : arr) {
		pybind11::object o = toPyStr(v);
		pylist.append(o);
	}
    return pylist;
}

/**
 * @brief object转换为string
 * @param obj
 * @return
 */
QString toString(const pybind11::object& obj)
{
    return QString::fromStdString(obj.cast< std::string >());
}

}  // namespace PY

namespace
{
// 静态注册辅助类
struct MetaTypeRegistrar_DAPy
{
	MetaTypeRegistrar_DAPy()
	{
		qRegisterMetaType< DA::DAPyObjectWrapper >("DA::DAPyObjectWrapper");
		qRegisterMetaType< DA::DAPyDType >("DA::DAPyDType");
		qRegisterMetaType< DA::DAPyDataFrame >("DA::DAPyDataFrame");
		qRegisterMetaType< DA::DAPyIndex >("DA::DAPyIndex");
		qRegisterMetaType< DA::DAPySeries >("DA::DAPySeries");
	}
};
// 静态实例触发注册
static MetaTypeRegistrar_DAPy _registrar;
}
}  // namespace DA
