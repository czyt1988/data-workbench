#ifndef DAXMLFILEINTERFACE_H
#define DAXMLFILEINTERFACE_H
#include <QString>
#include "DAUtilsAPI.h"
// #include "DAStringUtil.h"
#include <QPen>
#include <QFont>
#include <QBrush>
#include <QDomElement>
#include <QPointF>
#include <QVariant>
#include <QIODevice>
#include <QVector3D>
#include <QVersionNumber>
#include "da_qstring_cast.h"
class QDomDocument;
#ifndef DAXMLFILEINTERFACE_POD_NUMBER_H
#define DAXMLFILEINTERFACE_TYPE_MAKE_H(pod_type)                                                                       \
	static QDomElement makeElement(const pod_type& v, const QString& tagName, QDomDocument* doc);                      \
	static bool loadElement(pod_type& p, const QDomElement* ele);
#endif
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
	/// @defgroup POD类型 这是POD类型的xml保存
	/// @{
	DAXMLFILEINTERFACE_TYPE_MAKE_H(short)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(unsigned short)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(int)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(unsigned int)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(long)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(unsigned long)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(long long)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(unsigned long long)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(double)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(float)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(std::string)
	/// @}

	/// @defgroup qt类型 这是qt类型的xml保存
	/// @{
	// 标准保存—— QString
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QString)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QRect)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QRectF)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QPoint)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QPointF)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QPen)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QBrush)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QFont)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QVector3D)
	DAXMLFILEINTERFACE_TYPE_MAKE_H(QVariant)
	/// @}

	/// @defgroup stl类型 这是针对stl类型的xml保存
	/// @{
	template< typename std_container_like >
	static QDomElement makeElement(const std_container_like& v, const QString& tagName, QDomDocument* doc);
	template< typename std_container_like >
	static bool loadElement(std_container_like& v, const QDomElement* ele);
	///@}
};

#ifndef DAXMLELEMENTSERIALIZATION_INOUT_H
#define DAXMLELEMENTSERIALIZATION_INOUT_H(type)                                                                        \
	DAXMLElementSerialization& operator<<(const type& t);                                                              \
	DAXMLElementSerialization& operator>>(type& t);
#endif

/**
 * @brief 针对xml文件序列化，实现类似二进制文件一样<<和>>的序列化
 *
 * 举例：
 * @code
 * QDomElement parentElement;
 * QString v1;
 * QPoint v2;
 * double v3;
 * int v4;
 * std::string v5;
 * ...
 * // write
 * DAXMLSerialization s(&parentElement);
 * s << v1 << v2 << v3 << v4 << v5;
 *
 * // read
 * s >> v1 >> v2 >> v3 >> v4 >> v5;
 * @endcode
 */
class DAUTILS_API DAXMLElementSerialization
{
public:
	DAXMLElementSerialization(QDomElement* parentElement);
	~DAXMLElementSerialization();
	// clang-format off
    DAXMLELEMENTSERIALIZATION_INOUT_H(short) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(unsigned short)
    DAXMLELEMENTSERIALIZATION_INOUT_H(int)
    DAXMLELEMENTSERIALIZATION_INOUT_H(unsigned int)
    DAXMLELEMENTSERIALIZATION_INOUT_H(long)
    DAXMLELEMENTSERIALIZATION_INOUT_H(unsigned long) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(long long)
    DAXMLELEMENTSERIALIZATION_INOUT_H(unsigned long long) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(double)
    DAXMLELEMENTSERIALIZATION_INOUT_H(float) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(std::string)
    DAXMLELEMENTSERIALIZATION_INOUT_H(QString) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(QRect)
    DAXMLELEMENTSERIALIZATION_INOUT_H(QRectF) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(QPoint)
    DAXMLELEMENTSERIALIZATION_INOUT_H(QPointF) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(QPen)
    DAXMLELEMENTSERIALIZATION_INOUT_H(QBrush) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(QFont)
    DAXMLELEMENTSERIALIZATION_INOUT_H(QVector3D) 
    DAXMLELEMENTSERIALIZATION_INOUT_H(QVariant)
	// clang-format on
private:
	QDomElement* mParentElement { nullptr };
	QDomElement* mCurrentElement { nullptr };
};

template< typename std_container_like >
QDomElement DAXMLFileInterface::makeElement(const std_container_like& v, const QString& tagName, QDomDocument* doc)
{
	// using T = std_container_like::value_type;
	auto ele = doc->createElement(tagName);
	ele.setAttribute("class", QString("%1").arg(typeid(std_container_like).name()));
	ele.setAttribute("size", v.size());
	for (const auto& r : v) {
		auto eleR     = doc->createElement("r");
		auto textNode = doc->createTextNode(toQString(r));
		eleR.appendChild(textNode);
		ele.appendChild(eleR);
	}
	return ele;
}

template< typename std_container_like >
bool DAXMLFileInterface::loadElement(std_container_like& v, const QDomElement* ele)
{
	using T   = typename std_container_like::value_type;
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
		T r;
		if (!fromQString(rEle.text(), r)) {
			continue;
		}

		v[ i ] = r;
	}
	return true;
}

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
DAUTILS_API QString doubleToString(double a);
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
