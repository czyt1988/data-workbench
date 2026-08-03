#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief get_column_stats 工具：获取某列的描述性统计信息
 *
 * 参数：data_name（必填）、column（必填）
 * 返回：count/mean/std/min/25%/50%/75%/max + 缺失值计数
 */
class DAAgentToolColumnStats : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
