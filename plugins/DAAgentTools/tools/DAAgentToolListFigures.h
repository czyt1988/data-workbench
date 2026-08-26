#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief list_figures 工具：列出所有 figure 及其内部 chart 信息
 *
 * 无参数。返回每个 figure 的名称、ID、chart 数量及各 chart 的标题/索引，
 * 便于 agent 后续通过 figure_name + chart_id 定位具体绘图进行修改。
 */
class DAAgentToolListFigures : public DAAgentChartToolBase
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
