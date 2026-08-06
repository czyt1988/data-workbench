#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief write_file 工具：写入文本文件
 *
 * 参数：file_path（必填）、content（必填）
 * 包含路径安全检查，拒绝写入系统关键目录
 */
class DAAgentToolWriteFile : public DAAgentToolBase
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
