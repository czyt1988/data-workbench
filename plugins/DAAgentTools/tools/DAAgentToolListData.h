#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief list_data 工具：列出当前加载的所有数据集
 *
 * 返回每个数据集的名称、类型、行数、列数。
 */
class DAAgentToolListData : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    // 获取工具规格
    QJsonObject getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
