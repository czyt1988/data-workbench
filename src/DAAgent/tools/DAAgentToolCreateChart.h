#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief create_chart 工具：创建折线/散点/柱状/直方/箱线图
 *
 * 参数：type(line/scatter/bar/hist/box)、data_name、x、y、title、x_label、y_label
 */
class DAAgentToolCreateChart : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
