#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief add_curve 工具：向已有图表添加曲线
 *
 * 参数：chart_id(可选)、data_name、x_column、y_column、name、color、width、style
 */
class DAAgentToolAddCurve : public DAAgentToolBase
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
