// DAAgentProvider.h
#pragma once
#include "DAAgentAPI.h"
#include <QString>
#include <QList>
#include <QMetaType>

namespace DA
{
/**
 * @brief LLM 模型条目（供应商下的一个模型）
 *
 * 纯数据值类型（镜像 DAAgentPermissionRule 的头文件内联风格）。
 * contextWindow/maxOutputTokens 默认 262144(256K)/131072(128K)，与
 * DAAgentConfig 的默认值保持一致。
 */
struct DAAgent_API DAAgentModel
{
    QString id;                          ///< 模型 id（如 gpt-4o）
    int contextWindow  = 262144;         ///< 上下文窗口（token）
    int maxOutputTokens = 131072;        ///< 单次最大输出 token（128K，防长文档写作被截断）

    bool operator==(const DAAgentModel& other) const
    {
        return id == other.id && contextWindow == other.contextWindow && maxOutputTokens == other.maxOutputTokens;
    }
    bool operator!=(const DAAgentModel& other) const { return !(*this == other); }
};

/**
 * @brief LLM 供应商配置（多供应商多模型管理）
 *
 * 纯数据值类型。apiKey 为内存态明文——持久化加密（DPAPI）只发生在
 * DAAgentConfig::save()/load() 的序列化边界，结构体本身不感知加密。
 */
struct DAAgent_API DAAgentProvider
{
    QString name;                        ///< 供应商名称（唯一键）
    QString baseUrl;                     ///< API 基础地址
    QString apiKey;                      ///< 内存态明文；持久化时加密
    QList< DAAgentModel > models;        ///< 可选模型列表

    bool operator==(const DAAgentProvider& other) const
    {
        return name == other.name && baseUrl == other.baseUrl && apiKey == other.apiKey && models == other.models;
    }
    bool operator!=(const DAAgentProvider& other) const { return !(*this == other); }
};

/**
 * @brief 可选模型引用（Dock 模型选择器数据源，不含 api_key）
 *
 * availableModelsChanged 信号与 getAvailableModels 的载荷元素，
 * 替代原 QVariantMap{provider,model,context_window,max_output_tokens}。
 */
struct DAAgent_API DAAgentModelRef
{
    QString provider;                    ///< 所属供应商名
    QString model;                       ///< 模型 id
    int contextWindow  = 262144;         ///< 上下文窗口（token）
    int maxOutputTokens = 131072;        ///< 单次最大输出 token（128K）

    bool operator==(const DAAgentModelRef& other) const
    {
        return provider == other.provider && model == other.model && contextWindow == other.contextWindow
               && maxOutputTokens == other.maxOutputTokens;
    }
    bool operator!=(const DAAgentModelRef& other) const { return !(*this == other); }
};
}  // namespace DA

Q_DECLARE_METATYPE(DA::DAAgentModelRef)
