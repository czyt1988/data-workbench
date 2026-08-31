#include "DAPaperToolVerifyDoi.h"
#include "DAPaperLiteratureApi.h"

namespace DA
{
/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAPaperToolVerifyDoi::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("verify_doi"),
                         QStringLiteral("Verify whether a DOI exists and fetch its exact metadata (title, journal, year, authors) from CrossRef. Use this before including any DOI in a reference list to avoid fabricated citations. Requires direct internet access to api.crossref.org (an http:// proxy set via the https_proxy/http_proxy environment variable is honored).")};
    spec.addParam({QStringLiteral("doi"),
                   QStringLiteral("The DOI to verify, e.g. \"10.1038/s41586-020-2649-2\" (https://doi.org/ prefix is accepted and stripped)"),
                   {Type::String},
                   true});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAPaperToolVerifyDoi::execute(const QJsonObject& params)
{
    QString doi = params["doi"].toString().trimmed();
    if (doi.isEmpty()) {
        return errorResponse("doi is required");
    }
    // 容错：剥离常见前缀（用户/LLM 常带全 URL 或 doi: 前缀）
    if (doi.startsWith(QStringLiteral("https://doi.org/"), Qt::CaseInsensitive)) {
        doi = doi.mid(QStringLiteral("https://doi.org/").length());
    } else if (doi.startsWith(QStringLiteral("http://doi.org/"), Qt::CaseInsensitive)) {
        doi = doi.mid(QStringLiteral("http://doi.org/").length());
    } else if (doi.startsWith(QStringLiteral("doi:"), Qt::CaseInsensitive)) {
        doi = doi.mid(4);
    }
    doi = doi.trimmed();
    if (!doi.startsWith(QStringLiteral("10."))) {
        // DOI 语法快速校验（注册机构前缀必以 10. 开头），不发起网络请求
        QJsonObject data;
        data["valid"] = false;
        data["doi"]   = doi;
        data["reason"] = QStringLiteral("Malformed DOI: must start with \"10.\"");
        return successResponse(data);
    }

    QJsonObject entry;
    QString error;
    if (!DAPaperLiteratureApi::crossrefDoi(doi, entry, error)) {
        // 网络故障（超时/连接失败/解析失败）与"DOI 不存在"需区分：
        // 前者是工具错误，后者是有效查询结果（valid=false）
        if (error.contains(QStringLiteral("Network error"))) {
            return errorResponse(QString("DOI verification failed: %1").arg(error));
        }
        QJsonObject data;
        data["valid"]   = false;
        data["doi"]     = doi;
        data["reason"]  = error;
        return successResponse(data);
    }
    QJsonObject data = entry;
    data["valid"] = true;
    return successResponse(data);
}
}  // namespace DA
