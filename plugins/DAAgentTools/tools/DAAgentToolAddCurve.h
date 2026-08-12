#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief add_curve 工具：向已有图表添加曲线
 *
 * 参数：chart_id(可选)、data_name、x_column、y_column、name、color、width、style
 */
class DAAgentToolAddCurve : public DAAgentChartToolBase
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
