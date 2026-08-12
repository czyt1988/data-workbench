#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief read_file 工具：读取文本文件内容
 *
 * 参数：file_path（必填）
 * 包含路径安全检查，拒绝读取系统关键目录
 */
class DAAgentToolReadFile : public DAAgentToolBase
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
