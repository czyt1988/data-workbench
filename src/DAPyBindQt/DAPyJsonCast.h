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


// 将 QJsonObject 转换为 pybind11::dict
DAPYBINDQT_API pybind11::dict qjsonObjectToPyDict(const QJsonObject& jsonObj);

// 将 QJsonArray 转换为 pybind11::list
DAPYBINDQT_API pybind11::list qjsonArrayToPyList(const QJsonArray& jsonArray);

// 将 QJsonValue 转换为 pybind11::object
DAPYBINDQT_API pybind11::object qjsonValueToPyObject(const QJsonValue& jsonValue);

// 将 pybind11::dict 转换为 QJsonObject
DAPYBINDQT_API QJsonObject pyDictToQJsonObject(const pybind11::dict& pyDict);

// 将 pybind11::list 转换为 QJsonArray
DAPYBINDQT_API QJsonArray pyListToQJsonArray(const pybind11::list& pyList);

// 将 pybind11::object 转换为 QJsonValue
DAPYBINDQT_API QJsonValue pyObjectToQJsonValue(const pybind11::object& pyObj);

// 将 JSON 字符串转换为 pybind11::dict
DAPYBINDQT_API pybind11::dict jsonStringToPyDict(const QString& jsonStr);

// 将 pybind11::dict 转换为 JSON 字符串
DAPYBINDQT_API QString pyDictToJsonString(const pybind11::dict& pyDict, int indent = 0);

}  // namespace PY
}  // namespace DA

#endif  // DAPYJSONCAST_H
