#ifndef DAXMLFILEINTERFACE_H
#define DAXMLFILEINTERFACE_H
#include <QString>
#include "DAUtilsAPI.h"
#include <QPen>
#include <QFont>
#include <QBrush>
#include <QDomElement>
#include <QPointF>
#include <QVariant>
#include <QIODevice>
#include <QVector3D>
#include <QVersionNumber>
#include "da_qstring_util.hpp"
class QDomDocument;
namespace DA
{
/**
 * @brief 所有支持xml文件保存的类继承于它，从而提供saveToXml和loadFromXml接口
 */
class DAUTILS_API DAXMLFileInterface
{
public:
	DAXMLFileInterface();
	virtual ~DAXMLFileInterface();
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const = 0;
	virtual bool loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)                  = 0;

public:
	/// @defgroup qt类型 这是qt类型的xml保存
	/// @{
	// 标准保存—— QString
	static QDomElement makeElement(const QString& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QString& p, const QDomElement* ele);
	// 标准保存—— QRect
	static QDomElement makeElement(const QRect& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QRect& p, const QDomElement* ele);
	// 标准保存—— QRectF
	static QDomElement makeElement(const QRectF& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QRectF& p, const QDomElement* ele);
	// 标准保存—— QPoint
	static QDomElement makeElement(const QPoint& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QPoint& p, const QDomElement* ele);
	// 标准保存—— QPointF
	static QDomElement makeElement(const QPointF& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QPointF& p, const QDomElement* ele);
	// 标准保存—— QPen 画笔
	static QDomElement makeElement(const QPen& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QPen& p, const QDomElement* ele);
	// 标准保存—— QBrush 画笔
	static QDomElement makeElement(const QBrush& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QBrush& p, const QDomElement* ele);
	// 标准保存—— QFont
	static QDomElement makeElement(const QFont& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QFont& p, const QDomElement* ele);
	// 标准保存—— QVector3D
	static QDomElement makeElement(const QVector3D& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QVector3D& p, const QDomElement* ele);
	// 标准保存—— QVariant
	static QDomElement makeElement(const QVariant& v, const QString& tagName, QDomDocument* doc);
	static bool loadElement(QVariant& p, const QDomElement* ele);
	/// @}

	/// @defgroup stl类型 这是针对stl类型的xml保存
	/// @{
	template< typename T >
	QDomElement makeElement(const std::vector< T >& v, const QString& tagName, QDomDocument* doc)
	{
		auto ele = doc->createElement(tagName);
		ele.setAttribute("class", QString("std::vector<%1>").arg(typeid(T).name()));
		ele.setAttribute("size", v.size());
		for (const auto& r : v) {
			auto eleR     = doc->createElement("r");
			auto textNode = doc->createTextNode(toQString(r));
			eleR.appendChild(textNode);
			ele.appendChild(eleR);
		}
		return ele;
	}
	template< typename T >
	bool loadElement(std::vector< T >& v, const QDomElement* ele)
	{
		bool isok = false;
		auto size = ele->attribute("size").toUInt(&isok);
		if (!isok) {
			return false;
		}
		v.resize(size);
		auto cns = ele->childNodes();
		for (int i = 0; i < cns.size(); ++i) {
			auto rEle = cns.at(i).toElement();
			if (rEle.tagName() != "r") {
				continue;
			}
			typename std::vector< T >::value_type r = fromQString< typename std::vector< T >::value_type >(rEle.text());

			v[ i ] = r;
		}
		return true;
	}
	///@}
};

/**
 * @brief QVariant转为QString
 *
 * 此转换会以尽量可以明文的形式把QVariant转换为字符串，有些如QByteArray，无法用明文的，会转换为Base64进行保存
 * @param var QVariant值
 * @return 转换好的字符串
 * @see stringToVariant
 */
DAUTILS_API QString variantToString(const QVariant& var);

/**
 * @brief variantToString的逆方法
 * @param var 字面值
 * @param typeName 参数类型
 * @return 根据字面值转换回来的QVariant
 */
DAUTILS_API QVariant stringToVariant(const QString& var, const QString& typeName);

/**
 * @brief 把double转换为字符串，并且尽量不丢失精度，不会转换为xxe-5这样的情况
 *
 * @note 此函数效率不高，但能保证doulbe转换为文本，在转换回double的精度
 * @param a
 * @return
 */
DAUTILS_API QString doubleToString(const double a);
/**
 * @brief 把QVariant转换为Base64字符
 * @param var
 * @return
 */
template< typename T >
QString converVariantToBase64String(const QVariant& var)
{
	if (var.canConvert< T >()) {
		QByteArray byte;
		QDataStream st(&byte, QIODevice::ReadWrite);
		T ba = var.value< T >();
		st << ba;
		return (QString(byte.toBase64()));
	}
	return (QString());
}

/**
 * @brief 把Base64字符转换为对应变量
 * @param base64
 * @return
 */
template< typename T >
QVariant converBase64StringToVariant(const QString& base64)
{
	QByteArray byte = QByteArray::fromBase64(base64.toLocal8Bit());
	QDataStream st(&byte, QIODevice::ReadWrite);
	T ba;

	st >> ba;
	return (QVariant::fromValue(ba));
}

// 获取qreal值
bool DAUTILS_API getStringIntValue(const QString& valuestring, int& v);
bool DAUTILS_API getStringUIntValue(const QString& valuestring, unsigned int& v);
bool DAUTILS_API getStringULongLongValue(const QString& valuestring, unsigned long long& v);
bool DAUTILS_API getStringRealValue(const QString& valuestring, qreal& v);
bool DAUTILS_API getStringBoolValue(const QString& valuestring);

// Qt::Alignment的枚举转换
DAUTILS_API QString enumToString(Qt::Alignment e);
// Qt::Alignment的枚举转换
DAUTILS_API Qt::Alignment stringToEnum(const QString& s, Qt::Alignment defaultEnum = Qt::AlignCenter);

// Qt::PenStyle的枚举转换
DAUTILS_API QString enumToString(Qt::PenStyle e);
// Qt::PenStyle的枚举转换
DAUTILS_API Qt::PenStyle stringToEnum(const QString& s, Qt::PenStyle defaultEnum = Qt::SolidLine);

// Qt::BrushStyle 的枚举转换
DAUTILS_API QString enumToString(Qt::BrushStyle e);
// Qt::BrushStyle 的枚举转换
DAUTILS_API Qt::BrushStyle stringToEnum(const QString& s, Qt::BrushStyle defaultEnum = Qt::SolidPattern);

// Qt::AspectRatioMode的枚举转换
DAUTILS_API QString enumToString(Qt::AspectRatioMode e);
// Qt::AspectRatioMode的枚举转换
DAUTILS_API Qt::AspectRatioMode stringToEnum(const QString& s, Qt::AspectRatioMode defaultEnum = Qt::KeepAspectRatio);

// Qt::TransformationMode的枚举转换
DAUTILS_API QString enumToString(Qt::TransformationMode e);
// Qt::TransformationMode的枚举转换
DAUTILS_API Qt::TransformationMode stringToEnum(const QString& s,
												Qt::TransformationMode defaultEnum = Qt::FastTransformation);

// Qt::TransformationMode的枚举转换
DAUTILS_API QString enumToString(QFont::Weight e);
// Qt::TransformationMode的枚举转换
DAUTILS_API QFont::Weight stringToEnum(const QString& s, QFont::Weight defaultEnum = QFont::Normal);
}

#endif  // DAXMLFILEINTERFACE_H
