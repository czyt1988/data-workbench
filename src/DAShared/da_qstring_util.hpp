#ifndef DA_QSTRING_UTIL_HPP
#define DA_QSTRING_UTIL_HPP
#include <QString>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <type_traits>
#include <string>


namespace DA
{

// 将任何类型转换为QString
template<typename T>
typename std::enable_if<std::is_fundamental<T>::value, QString>::type
toQString(const T& value)
{
	if constexpr (std::is_same_v<T, bool>) {
		return value ? QStringLiteral("true") : QStringLiteral("false");
	} else {
		return QString::number(value);
	}
}

// 特化QString -> T，对于基本类型
template<typename T>
T fromQString(const QString& str, const T& defaultValue = T()) {
	if constexpr (std::is_same_v<T, int>) {
		bool ok;
		int result = str.toInt(&ok);
		return ok ? result : defaultValue;
	} else if constexpr (std::is_same_v<T, float>) {
		bool ok;
		float result = str.toFloat(&ok);
		return ok ? result : defaultValue;
	} else if constexpr (std::is_same_v<T, double>) {
		bool ok;
		double result = str.toDouble(&ok);
		return ok ? result : defaultValue;
	} else if constexpr (std::is_same_v<T, bool>) {
		return str.toLower() == QStringLiteral("true");
	} else if constexpr (std::is_same_v<T, char>) {
		return str.isEmpty() ? defaultValue : str.front().toLatin1();
	}
	// 不应到达这里，因为我们有std::enable_if的检查
	return defaultValue;
}


// 特化QString -> std::string
template<>
std::string fromQString<std::string>(const QString& str, const std::string& defaultValue = std::string()) {
	return str.toStdString();
}
// 特化QString -> QByteArray
template<>
QByteArray fromQString<QByteArray>(const QString& str)
{
	return QByteArray::fromStdString(str.toStdString());
}

// 特化QString -> QDate
template<>
QDate fromQString<QDate>(const QString& str)
{
	return QDate::fromString(str, Qt::TextDate);
}

// 特化QString -> QDateTime
template<>
QDateTime fromQString<QDateTime>(const QString& str)
{
	return QDateTime::fromString(str, Qt::TextDate);
}

// 特化QString -> QTime
template<>
QTime fromQString<QTime>(const QString& str)
{
	return QTime::fromString(str, Qt::TextDate);
}

// 特化T -> QString
template<>
QString toQString<std::string>(const std::string& value)
{
	return QString::fromStdString(value);
}

// 特化T -> QString
template<>
QString toQString<QByteArray>(const QByteArray& value)
{
	return QString::fromUtf8(value);
}

// 特化T -> QString
template<>
QString toQString<QDate>(const QDate& value)
{
	return value.toString(Qt::TextDate);
}

// 特化T -> QString
template<>
QString toQString<QDateTime>(const QDateTime& value)
{
	return value.toString(Qt::TextDate);
}

// 特化T -> QString
template<>
QString toQString<QTime>(const QTime& value)
{
	return value.toString(Qt::TextDate);
}
}
#endif  // DA_QSTRING_UTIL_HPP
