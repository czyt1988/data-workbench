#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief add_region 工具：添加区域高亮（垂直矩形）
 *
 * 参数：chart_id(可选)、start_x、end_x、color、label
 */
class DAAgentToolAddRegion : public DAAgentChartToolBase
{
    Q_OBJECT
public:
    using DAAgentChartToolBase::DAAgentChartToolBase;
    // 获取工具规格
    QJsonObject getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
