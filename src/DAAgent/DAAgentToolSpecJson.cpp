#include "DAAgentToolSpecJson.h"
#include <QJsonArray>
#include <QJsonValue>

namespace DA
{
namespace
{
/// 参数类型枚举 → JSON Schema type 字符串
QString typeToString(DAAgentToolParam::Type type)
{
    switch (type) {
    case DAAgentToolParam::Type::String:
        return QStringLiteral("string");
    case DAAgentToolParam::Type::Integer:
        return QStringLiteral("integer");
    case DAAgentToolParam::Type::Number:
        return QStringLiteral("number");
    case DAAgentToolParam::Type::Boolean:
        return QStringLiteral("boolean");
    case DAAgentToolParam::Type::Array:
        return QStringLiteral("array");
    case DAAgentToolParam::Type::Object:
        return QStringLiteral("object");
    }
    return QStringLiteral("string");
}

/// 类型列表 → JSON 值：单元素输出字符串，多元素输出数组（联合类型）
QJsonValue typeListToJson(const QList< DAAgentToolParam::Type >& types)
{
    if (types.size() == 1) {
        return QJsonValue(typeToString(types.first()));
    }
    QJsonArray arr;
    for (auto t : types) {
        arr.append(typeToString(t));
    }
    return QJsonValue(arr);
}

/// 单个参数定义 → properties 条目
QJsonObject paramToJson(const DAAgentToolParam& param)
{
    QJsonObject obj;
    if (!param.types.isEmpty()) {
        obj[QStringLiteral("type")] = typeListToJson(param.types);
    }
    if (!param.description.isEmpty()) {
        obj[QStringLiteral("description")] = param.description;
    }
    if (!param.itemTypes.isEmpty()) {
        obj[QStringLiteral("items")] = QJsonObject{{QStringLiteral("type"), typeListToJson(param.itemTypes)}};
    }
    if (!param.enumValues.isEmpty()) {
        obj[QStringLiteral("enum")] = QJsonArray::fromVariantList(param.enumValues);
    }
    if (param.defaultValue.isValid()) {
        obj[QStringLiteral("default")] = QJsonValue::fromVariant(param.defaultValue);
    }
    return obj;
}
} // namespace

/**
 * @brief 把工具规格序列化为 OpenAI function schema
 *
 * 输出形态与旧手写 JSON 等价：{"name", "description",
 * "parameters":{"type":"object","properties":{...},"required":[...]}}。
 * 归一化约定：required 数组为空时省略该键（JSON Schema 语义不变）。
 */
QJsonObject toJson(const DAAgentToolSpec& spec)
{
    QJsonObject properties;
    QJsonArray requiredArr;
    for (const auto& param : spec.params) {
        properties[param.name] = paramToJson(param);
        if (param.required) {
            requiredArr.append(param.name);
        }
    }

    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = properties;
    if (!requiredArr.isEmpty()) {
        parameters[QStringLiteral("required")] = requiredArr;
    }

    QJsonObject obj;
    obj[QStringLiteral("name")] = spec.name;
    if (!spec.description.isEmpty()) {
        obj[QStringLiteral("description")] = spec.description;
    }
    obj[QStringLiteral("parameters")] = parameters;
    return obj;
}
} // namespace DA
