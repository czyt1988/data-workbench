#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief get_data_info 工具：获取数据集的 schema 和前 N 行预览
 *
 * 参数：data_name（必填）、preview_rows（可选，默认10）
 * 返回：列名列表、dtype 列表、行数、列数、前 N 行数据
 */
class DAAgentToolDataInfo : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
