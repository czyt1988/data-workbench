#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief add_annotation 工具：添加文本/箭头/点标注
 *
 * 参数：chart_id(可选)、type(text/arrow/point)、position([x,y])、text、color
 */
class DAAgentToolAddAnnotation : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    QJsonObject getToolSpec() const override;
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
