#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief query_data 工具：使用 pandas query 表达式筛选数据
 *
 * 参数：data_name（必填）、expr（必填）、preview_rows（可选，默认10）
 * 返回：筛选结果的行列数 + 前 N 行预览（避免返回全量数据）
 */
class DAAgentToolQueryData : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    /// @copydoc DAAbstractAgentTool::getToolSpec
    QJsonObject getToolSpec() const override;
    /// @copydoc DAAbstractAgentTool::execute
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
