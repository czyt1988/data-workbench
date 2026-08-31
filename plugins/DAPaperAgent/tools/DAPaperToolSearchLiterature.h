#pragma once
#include "DAAgentToolBase.h"

namespace DA
{
/**
 * @brief search_literature 工具：学术文献检索（CrossRef / OpenAlex 免费公开 API）
 *
 * 参数：query（必填）、max_results（默认 8，上限 20）、source（crossref/openalex/auto，默认 auto）
 * 返回规范化条目数组（doi/title/authors/journal/year/type/url）；
 * auto 模式下 CrossRef 失败自动降级 OpenAlex。
 * 网络故障返回英文错误信息，由提示词层切换离线文献模式。
 */
class DAPaperToolSearchLiterature : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;
    // 获取工具规格
    DAAgentToolSpec getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
    // 获取拥有此工具的模块名称
    QString getOwnerModule() const override { return QStringLiteral("DAPaperAgent"); }
};
}  // namespace DA
