#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief run_script 工具：在共享持久命名空间中执行工程脚本工作区内的 .py 文件
 *
 * 参数：path（必填，工作区相对路径）、args（可选，注入命名空间）
 * 含路径遍历防护：拒绝逃逸脚本工作区边界的相对路径
 */
class DAAgentToolRunScript : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    // 获取工具规格
    QJsonObject getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
