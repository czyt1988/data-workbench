#include "DAPyJsonCast.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonDocument>
#include <cmath>
#include <limits>
#include <QDateTime>
#include "numpy/DAPyModuleNumpy.h"
#include "DAPyModuleDatetime.h"

namespace DA
{
namespace PY
{

namespace
{
/**
 * @brief 检查浮点数是否为整数（在容差范围内）
 * @param d 要检查的浮点数
 * @return 如果接近整数返回 true
 */
inline bool isDoubleCloseToInt(double d)
{
    if (std::isnan(d) || std::isinf(d)) {
        return false;
    }
    double intPart;
    return std::modf(d, &intPart) < std::numeric_limits< double >::epsilon();
}

/**
 * @brief 获取 QJsonValue 的类型字符串，用于调试
 * @param value QJsonValue
 * @return 类型字符串
 */
QString jsonValueTypeString(const QJsonValue& value)
{
    switch (value.type()) {
    case QJsonValue::Null:
        return "Null";
    case QJsonValue::Bool:
        return "Bool";
    case QJsonValue::Double:
        return "Double";
    case QJsonValue::String:
        return "String";
    case QJsonValue::Array:
        return "Array";
    case QJsonValue::Object:
        return "Object";
    case QJsonValue::Undefined:
        return "Undefined";
    default:
        return "Unknown";
    }
}
}  // 匿名命名空间

pybind11::dict qjsonObjectToPyDict(const QJsonObject& jsonObj)
{
    pybind11::dict result;

    if (jsonObj.isEmpty()) {
        return result;
    }

    for (auto it = jsonObj.constBegin(); it != jsonObj.constEnd(); ++it) {
        QString key                                = it.key();
        const QJsonValue& value                    = it.value();
        result[ pybind11::str(key.toStdString()) ] = qjsonValueToPyObject(value);
    }

    return result;
}

pybind11::list qjsonArrayToPyList(const QJsonArray& jsonArray)
{
    pybind11::list result;

    for (const QJsonValue& value : jsonArray) {
        result.append(qjsonValueToPyObject(value));
    }

    return result;
}

pybind11::object qjsonValueToPyObject(const QJsonValue& jsonValue)
{
    switch (jsonValue.type()) {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return pybind11::none();

    case QJsonValue::Bool:
        return pybind11::bool_(jsonValue.toBool());

    case QJsonValue::Double: {
        double d = jsonValue.toDouble();
        // 检查是否为整数（在容差范围内）
        if (isDoubleCloseToInt(d)) {
            long long intValue = static_cast< long long >(std::round(d));
            // 检查是否在 int 范围内
            if (intValue >= std::numeric_limits< int >::min() && intValue <= std::numeric_limits< int >::max()) {
                return pybind11::int_(static_cast< int >(intValue));
            }
            return pybind11::int_(intValue);
        }
        return pybind11::float_(d);
    }

    case QJsonValue::String:
        return toPyStr(jsonValue.toString());

    case QJsonValue::Array:
        return qjsonArrayToPyList(jsonValue.toArray());

    case QJsonValue::Object:
        return qjsonObjectToPyDict(jsonValue.toObject());
    }

    // 不应该到达这里
    return pybind11::none();
}

QJsonObject pyDictToQJsonObject(const pybind11::dict& pyDict)
{
    QJsonObject result;

    if (pyDict.empty()) {
        return result;
    }


    for (const auto& item : pyDict) {
        pybind11::handle keyHandle   = item.first;
        pybind11::handle valueHandle = item.second;

        // 获取键名
        QString key;
        if (pybind11::isinstance< pybind11::str >(keyHandle)) {
            key = toString(pybind11::str(keyHandle));
        } else {
            // 非字符串键转换为字符串
            key = QString::fromStdString(pybind11::str(keyHandle).cast< std::string >());
        }

        // 转换值
        pybind11::object valueObj = pybind11::reinterpret_borrow< pybind11::object >(valueHandle);
        result[ key ]             = pyObjectToQJsonValue(valueObj);
    }


    return result;
}

QJsonArray pyListToQJsonArray(const pybind11::list& pyList)
{
    QJsonArray result;
    for (size_t i = 0; i < pyList.size(); ++i) {
        pybind11::object item = pyList[ i ];
        result.append(pyObjectToQJsonValue(item));
    }
    return result;
}

QJsonValue pyObjectToQJsonValue(const pybind11::object& pyObj)
{
    if (pyObj.is_none()) {
        return QJsonValue::Null;
    }

    // 使用现有的 toString 和 toVariant 函数处理其他类型
    if (pybind11::isinstance< pybind11::bool_ >(pyObj)) {
        return QJsonValue(pyObj.cast< bool >());
    }

    if (pybind11::isinstance< pybind11::int_ >(pyObj)) {
        try {
            return QJsonValue(pyObj.cast< long long >());
        } catch (...) {
            // 如果转换失败，尝试转换为字符串
            return QJsonValue(toString(pyObj));
        }
    }

    if (pybind11::isinstance< pybind11::float_ >(pyObj)) {
        try {
            return QJsonValue(pyObj.cast< double >());
        } catch (...) {
            // 如果转换失败，尝试转换为字符串
            return QJsonValue(toString(pyObj));
        }
    }

    if (pybind11::isinstance< pybind11::str >(pyObj)) {
        return QJsonValue(toString(pybind11::str(pyObj)));
    }

    if (DAPyModuleDatetime::getInstance().isInstanceDateTime(pyObj)) {
        try {
            QDateTime dt = toVariant(pyObj).toDateTime();
            if (dt.isValid()) {
                return QJsonValue(dt.toString(Qt::ISODate));
            }
        } catch (...) {
            // 忽略错误，继续尝试其他转换
        }
    }

    if (DAPyModuleDatetime::getInstance().isInstanceDate(pyObj)) {
        try {
            QDate date = toVariant(pyObj).toDate();
            if (date.isValid()) {
                return QJsonValue(date.toString(Qt::ISODate));
            }
        } catch (...) {
            // 忽略错误，继续尝试其他转换
        }
    }

    if (pybind11::isinstance< pybind11::list >(pyObj) || pybind11::isinstance< pybind11::tuple >(pyObj)) {
        try {
            return pyListToQJsonArray(pybind11::cast< pybind11::list >(pyObj));
        } catch (...) {
            return QJsonArray();
        }
    }

    if (pybind11::isinstance< pybind11::dict >(pyObj)) {
        try {
            return pyDictToQJsonObject(pybind11::cast< pybind11::dict >(pyObj));
        } catch (...) {
            return QJsonObject();
        }
    }

    // 其他类型：先尝试使用现有的 toVariant 函数
    QVariant var = toVariant(pyObj);
    if (var.isValid() && !var.isNull()) {
        // 尝试转换为合适的 JSON 类型
        if (var.canConvert< QString >()) {
            return QJsonValue(var.toString());
        }
        if (var.canConvert< double >()) {
            return QJsonValue(var.toDouble());
        }
        if (var.canConvert< bool >()) {
            return QJsonValue(var.toBool());
        }
        if (var.canConvert< long long >()) {
            return QJsonValue(var.toLongLong());
        }
    }

    // 最后的手段：转换为字符串
    return QJsonValue(toString(pyObj));
}

pybind11::dict jsonStringToPyDict(const QString& jsonStr)
{
    if (jsonStr.isEmpty()) {
        return pybind11::dict();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse JSON string:" << parseError.errorString();
        return pybind11::dict();
    }

    if (doc.isObject()) {
        return qjsonObjectToPyDict(doc.object());
    } else if (doc.isArray()) {
        // 如果是数组，包装在字典中返回
        pybind11::dict result;
        result[ "array" ] = qjsonArrayToPyList(doc.array());
        return result;
    }

    return pybind11::dict();
}

QString pyDictToJsonString(const pybind11::dict& pyDict, int indent)
{
    QJsonObject jsonObj = pyDictToQJsonObject(pyDict);
    QJsonDocument doc(jsonObj);

    if (indent > 0) {
        return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    } else {
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }
}

}  // namespace PY
}  // namespace DA
