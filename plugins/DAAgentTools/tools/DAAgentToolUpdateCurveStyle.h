#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief update_curve_style 工具：修改已有曲线的外观（颜色、线型、宽度、符号、填充）
 *
 * 参数：curve_name(必填)、color、width、style、symbol、symbol_size、fill_color
 */
class DAAgentToolUpdateCurveStyle : public DAAgentChartToolBase
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
