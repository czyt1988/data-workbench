#pragma once
#include "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief save_chart_image 工具：保存图表为图片文件
 *
 * 参数：chart_id(可选)、file_path、format(png/pdf/svg，默认png)、width、height
 * PDF 需链接 Qt::PrintSupport，SVG 需链接 Qt::Svg
 */
class DAAgentToolSaveChartImage : public DAAgentChartToolBase
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
