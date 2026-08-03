#pragma once
#include "DAAgentAPI.h"
#include <QJsonObject>
#include <QString>

namespace DA
{
/**
 * @brief agent 工具的抽象基类
 *
 * 每个工具提供一份 JSON 工具规格（OpenAI function schema）和一个 execute 方法。
 * 插件通过继承此类实现领域特定能力，交由 agent 调用。
 */
class DAAgent_API DAAbstractAgentTool
{
public:
    virtual ~DAAbstractAgentTool() = default;
    /**
     * @brief 获取工具的 OpenAI function 规格描述
     * @return 工具规格 JSON
     */
    virtual QJsonObject getToolSpec() const = 0;
    /**
     * @brief 执行工具
     * @param params 工具调用参数 JSON
     * @return 工具执行结果 JSON
     */
    virtual QJsonObject execute(const QJsonObject& params) = 0;
    /**
     * @brief 获取拥有此工具的模块名称
     * @return 模块名称
     */
    virtual QString getOwnerModule() const = 0;
};
} // namespace DA
