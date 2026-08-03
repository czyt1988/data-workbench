#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief export_data 工具：将数据集导出为 csv 或 excel 文件
 *
 * 参数：data_name（必填）、file_path（必填）、format（可选，csv/excel，默认 csv）
 */
class DAAgentToolExportData : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
