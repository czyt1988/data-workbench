#pragma once
#include "DAAgentAPI.h"
#include "DAAgentToolSpec.h"
#include <QJsonObject>
#include <QString>

namespace DA
{
/**
 * @brief agent 工具的抽象基类
 *
 * 每个工具提供一份结构化工具规格（DAAgentToolSpec，可序列化为 OpenAI
 * function schema）和一个 execute 方法。插件通过继承此类实现领域特定
 * 能力，交由 agent 调用。
 */
class DAAgent_API DAAbstractAgentTool
{
public:
    virtual ~DAAbstractAgentTool() = default;
    // 获取工具规格（结构化表达，经 DA::toJson(spec) 序列化为 OpenAI function schema）
    virtual DAAgentToolSpec getToolSpec() const = 0;
    // 执行工具
    virtual QJsonObject execute(const QJsonObject& params) = 0;
    // 获取拥有此工具的模块名称
    virtual QString getOwnerModule() const = 0;
};
} // namespace DA
