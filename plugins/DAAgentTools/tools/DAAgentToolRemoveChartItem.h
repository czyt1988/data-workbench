#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief remove_chart_item 工具：删除图表中的曲线、标注或区域
 *
 * 参数：item_name(必填)、item_type
 */
class DAAgentToolRemoveChartItem : public DAAgentChartToolBase
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
