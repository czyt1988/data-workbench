#pragma once
#include "DAAgentAPI.h"
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantList>

namespace DA
{
/**
 * @brief Agent 工具参数定义（JSON Schema 单个 property 的结构化表达）
 *
 * 与 OpenAI function schema 的 parameters.properties 条目一一对应：
 * types 对应 "type"（单元素序列化为字符串，多元素为联合类型数组），
 * itemTypes 对应数组参数的 "items"."type"，required 由各参数携带、
 * 序列化时汇总为 parameters.required 数组。Object 类型且无嵌套定义
 * 即为 free-form 对象（如 run_code 的 args）。
 */
struct DAAgent_API DAAgentToolParam
{
    /// 参数类型，与 JSON Schema type 词汇表对应
    enum class Type
    {
        String,   ///< "string"
        Integer,  ///< "integer"
        Number,   ///< "number"
        Boolean,  ///< "boolean"
        Array,    ///< "array"
        Object    ///< "object"
    };

    QString name;             ///< 参数名（snake_case，参与 LLM 工具调用匹配，不翻译）
    QString description;      ///< 参数说明（英文）
    QList< Type > types;      ///< 主类型列表，单元素序列化为字符串，多元素为数组（联合类型）
    bool required = false;    ///< 是否必填，序列化时汇总成 required 数组
    QList< Type > itemTypes;  ///< Array 时的元素类型（items），同样支持联合，空则不输出
    QVariantList enumValues;  ///< 可选值列表（JSON Schema enum），空则不输出
    QVariant defaultValue;    ///< 默认值（JSON Schema default），invalid 则不输出
};

/**
 * @brief Agent 工具规格（OpenAI function schema 的结构化表达）
 *
 * 由工具实现者构造，经 DA::toJson(const DAAgentToolSpec&)（见
 * DAAgentToolSpecJson.h）序列化为 OpenAI function schema JSON 下发给
 * LLM。本头文件零 JSON 依赖，保持值类型与序列化投影的关注点分离。
 */
struct DAAgent_API DAAgentToolSpec
{
    QString name;                     ///< 工具名（小写 snake_case，参与 LLM 工具调用匹配，不翻译）
    QString description;              ///< 工具说明（英文）
    QList< DAAgentToolParam > params; ///< 参数定义列表

    // 追加一个参数定义
    void addParam(const DAAgentToolParam& param)
    {
        params.append(param);
    }
};
} // namespace DA
