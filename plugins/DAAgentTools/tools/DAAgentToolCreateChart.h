#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief create_chart 工具：创建折线/散点/柱状/直方/箱线图
 *
 * 参数：type(line/scatter/bar/hist/box)、data_name、x、y、title、x_label、y_label
 */
class DAAgentToolCreateChart : public DAAgentChartToolBase
{
    Q_OBJECT
public:
    using DAAgentChartToolBase::DAAgentChartToolBase;
    /// @copydoc DAAbstractAgentTool::getToolSpec
    QJsonObject getToolSpec() const override;
    /// @copydoc DAAbstractAgentTool::execute
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
