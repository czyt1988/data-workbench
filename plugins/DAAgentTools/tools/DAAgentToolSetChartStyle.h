#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief set_chart_style 工具：设置图表样式
 *
 * 参数：chart_id(可选)、figure_name(可选)、title、x_label、y_label、legend(bool)、grid(bool)、
 *       x_axis_type(normal/datetime)、x_date_format、background_color、border_color 等
 */
class DAAgentToolSetChartStyle : public DAAgentChartToolBase
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
