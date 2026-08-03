#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief save_chart_image 工具：保存图表为图片文件
 *
 * 参数：chart_id(可选)、file_path、format(png/pdf/svg，默认png)、width、height
 * PDF 需链接 Qt::PrintSupport，SVG 需链接 Qt::Svg
 */
class DAAgentToolSaveChartImage : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
