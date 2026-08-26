#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief run_code 工具：在共享持久命名空间中执行内联 Python 代码
 *
 * 参数：code（必填）、args（可选，注入命名空间）
 * 无需工程即可用（命名空间为 app 级，da_app.getCore() 始终可用）
 */
class DAAgentToolRunCode : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    // 获取工具规格
    DAAgentToolSpec getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
