#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief list_chart_items 工具：列出图表内的所有元素（曲线/标注/区域）
 *
 * 参数：chart_id、figure_name、item_type（与 remove_chart_item 对齐）。
 * 返回每个元素的图例名（title）、索引、显示名（name）、类型、可见性、
 * 颜色与数据点数，供 agent 把图例名或索引传给 remove_chart_item /
 * update_curve_style 精确定位元素。
 */
class DAAgentToolListChartItems : public DAAgentChartToolBase
{
    Q_OBJECT
public:
    using DAAgentChartToolBase::DAAgentChartToolBase;
    // 获取工具规格
    DAAgentToolSpec getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
