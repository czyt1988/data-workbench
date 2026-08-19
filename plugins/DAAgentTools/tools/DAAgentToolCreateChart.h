#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief create_chart 工具：创建折线/散点/柱状/直方/箱线图
 *
 * 参数：type(line/scatter/bar/hist/box)、data_name、x、y(支持多列数组)、title、figure_name、x_label、y_label
 * 自动检测：当 X 列为 datetime64 类型时自动设置时间坐标轴
 */
class DAAgentToolCreateChart : public DAAgentChartToolBase
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
