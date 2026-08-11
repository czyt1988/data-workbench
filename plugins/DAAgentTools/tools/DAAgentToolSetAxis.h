#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief set_axis 工具：配置坐标轴类型、范围和外观
 *
 * 参数：axis(必填)、scale_type、date_format、min、max、color、label_rotation
 */
class DAAgentToolSetAxis : public DAAgentChartToolBase
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
