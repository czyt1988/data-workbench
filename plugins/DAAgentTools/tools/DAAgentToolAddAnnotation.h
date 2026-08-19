#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief add_annotation 工具：添加文本/箭头/点/区域标注
 *
 * 参数：chart_id(可选)、type(text/arrow/point/region)、text、color
 *       text/point 用 position([x,y])；arrow 用 start([x,y])/end([x,y])；region 用 start_x/end_x
 */
class DAAgentToolAddAnnotation : public DAAgentChartToolBase
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
