#ifndef DAPYJSONCAST_H
#define DAPYJSONCAST_H
#include "DAPyBindQtGlobal.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QVariant>
#include "DAPybind11InQt.h"
/**
 * @file DAPyJsonCast.h
 * @brief QJsonObject 和 pybind11::dict 之间的转换函数
 *
 * 这个模块提供了 Qt 的 JSON 类型和 Python 字典之间的双向转换功能。
 * 它依赖于 DAPybind11QtTypeCast 中的现有转换函数，并专门处理 JSON 结构。
 *
 * @note 此文件函数会抛出异常
 */

namespace DA
{
namespace PY
{


/**
 * @brief 将 QJsonObject 转换为 pybind11::dict
 *
 * 这个函数递归地将 QJsonObject 及其嵌套的所有 JSON 结构转换为 Python 字典。
 * 支持的类型包括：
 * - QJsonObject -> dict
 * - QJsonArray -> list
 * - QJsonValue 的基本类型（bool, int, double, string, null）
 *
 * @param jsonObj 要转换的 QJsonObject
 * @return 转换后的 pybind11::dict 对象
 *
 * @example
 * @code
 * QJsonObject json;
 * json["name"] = "张三";
 * json["age"] = 25;
 * json["scores"] = QJsonArray{85, 90, 78};
 *
 * pybind11::dict pyDict = DA::PY::qjsonObjectToPyDict(json);
 * // 现在 pyDict 可以在 Python 中使用
 * @endcode
 */
DAPYBINDQT_API pybind11::dict qjsonObjectToPyDict(const QJsonObject& jsonObj);

/**
 * @brief 将 QJsonArray 转换为 pybind11::list
 *
 * 递归地将 QJsonArray 转换为 Python 列表，支持嵌套的 JSON 结构。
 *
 * @param jsonArray 要转换的 QJsonArray
 * @return 转换后的 pybind11::list 对象
 */
DAPYBINDQT_API pybind11::list qjsonArrayToPyList(const QJsonArray& jsonArray);

/**
 * @brief 将 QJsonValue 转换为 pybind11::object
 *
 * 将单个 QJsonValue 转换为对应的 Python 对象。
 *
 * @param jsonValue 要转换的 QJsonValue
 * @return 转换后的 pybind11::object
 */
DAPYBINDQT_API pybind11::object qjsonValueToPyObject(const QJsonValue& jsonValue);

/**
 * @brief 将 pybind11::dict 转换为 QJsonObject
 *
 * 这个函数递归地将 Python 字典及其嵌套的所有 Python 结构转换为 QJsonObject。
 * 支持的类型包括：
 * - dict -> QJsonObject
 * - list/tuple -> QJsonArray
 * - Python 基本类型（bool, int, float, str, None）
 *
 * @param pyDict 要转换的 pybind11::dict
 * @return 转换后的 QJsonObject
 *
 * @example
 * @code
 * // Python 端传递过来的字典
 * pybind11::dict pyDict;
 * pyDict["name"] = pybind11::str("李四");
 * pyDict["age"] = pybind11::int_(30);
 * pyDict["hobbies"] = pybind11::list(pybind11::make_tuple("篮球", "音乐"));
 *
 * QJsonObject jsonObj = DA::PY::pyDictToQJsonObject(pyDict);
 * // 现在 jsonObj 可以在 C++ 中使用
 * @endcode
 */
DAPYBINDQT_API QJsonObject pyDictToQJsonObject(const pybind11::dict& pyDict);

/**
 * @brief 将 pybind11::list 转换为 QJsonArray
 *
 * 递归地将 Python 列表转换为 QJsonArray，支持嵌套的 Python 结构。
 *
 * @param pyList 要转换的 pybind11::list
 * @return 转换后的 QJsonArray
 */
DAPYBINDQT_API QJsonArray pyListToQJsonArray(const pybind11::list& pyList);

/**
 * @brief 将 pybind11::object 转换为 QJsonValue
 *
 * 将单个 Python 对象转换为对应的 QJsonValue。
 *
 * @param pyObj 要转换的 pybind11::object
 * @return 转换后的 QJsonValue
 */
DAPYBINDQT_API QJsonValue pyObjectToQJsonValue(const pybind11::object& pyObj);

/**
 * @brief 将 JSON 字符串转换为 pybind11::dict
 *
 * 将 JSON 格式的字符串解析为 QJsonObject，然后转换为 Python 字典。
 *
 * @param jsonStr JSON 格式的字符串
 * @return 转换后的 pybind11::dict
 * @throws 如果 JSON 解析失败，返回空字典
 */
DAPYBINDQT_API pybind11::dict jsonStringToPyDict(const QString& jsonStr);

/**
 * @brief 将 pybind11::dict 转换为 JSON 字符串
 *
 * 将 Python 字典转换为 QJsonObject，然后序列化为 JSON 字符串。
 *
 * @param pyDict 要转换的 pybind11::dict
 * @param indent 缩进空格数，0 表示紧凑格式
 * @return JSON 格式的字符串
 */
DAPYBINDQT_API QString pyDictToJsonString(const pybind11::dict& pyDict, int indent = 0);

}  // namespace PY
}  // namespace DA

#endif  // DAPYJSONCAST_H
