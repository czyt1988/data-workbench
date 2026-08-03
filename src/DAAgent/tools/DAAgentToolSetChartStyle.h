#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief set_chart_style 工具：设置图表样式
 *
 * 参数：chart_id(可选)、title、x_label、y_label、legend(bool)、grid(bool)
 */
class DAAgentToolSetChartStyle : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    /// @copydoc DAAbstractAgentTool::getToolSpec
    QJsonObject getToolSpec() const override;
    /// @copydoc DAAbstractAgentTool::execute
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
