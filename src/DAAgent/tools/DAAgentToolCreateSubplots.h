#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief create_subplots 工具：创建网格子图布局
 *
 * 参数：layout(如"2x2")、data_name(可选)、其他绘图参数
 * 使用 createChart(QRectF) 指定网格位置，避免双重添加
 */
class DAAgentToolCreateSubplots : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
