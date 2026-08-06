#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief save_report 工具：保存 Markdown 报告为 md/pdf/docx
 *
 * 参数：content（必填，markdown 文本）、file_path（必填）、format（md/pdf/docx，默认 md）
 * - md: 直接写入文件
 * - pdf: 通过 QPrinter + QTextDocument 渲染（需链接 Qt::PrintSupport）
 * - docx: 通过 DAAxObjectWordWrapper Word COM 自动化（Windows only）
 */
class DAAgentToolSaveReport : public DAAgentToolBase
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
