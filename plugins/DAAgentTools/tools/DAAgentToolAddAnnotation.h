#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief add_annotation 工具：添加文本/箭头/点标注
 *
 * 参数：chart_id(可选)、type(text/arrow/point)、text、color
 *       text/point 用 position([x,y])；arrow 用 start([x,y])/end([x,y])
 */
class DAAgentToolAddAnnotation : public DAAgentChartToolBase
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
