#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief verify_doi 工具：DOI 真伪验证（CrossRef 元数据查询）
 *
 * 参数：doi（必填）
 * 返回 valid / doi / title / journal / year / authors / type；
 * DOI 不存在返回 valid=false（非错误响应，供 agent 判断引用真伪），
 * 网络故障才返回错误响应。
 */
class DAPaperToolVerifyDoi : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    // 获取工具规格
    DAAgentToolSpec getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
    // 获取拥有此工具的模块名称
    QString getOwnerModule() const override { return QStringLiteral("DAPaperAgent"); }
};
}  // namespace DA
