#include "DAPaperToolSearchLiterature.h"
#include "DAPaperLiteratureApi.h"

namespace DA
{
/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAPaperToolSearchLiterature::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("search_literature"),
                         QStringLiteral("Search real academic literature (papers, books, conference proceedings) via the free CrossRef / OpenAlex metadata APIs. Returns normalized entries with DOI, title, authors, journal, year and type. Requires direct internet access to api.crossref.org and api.openalex.org (an http:// proxy set via the https_proxy/http_proxy environment variable is honored). If the network fails, report the error and fall back to user-provided literature.")};
    spec.addParam({QStringLiteral("query"), QStringLiteral("Search keywords, e.g. \"linear regression air quality\""), {Type::String}, true});
    spec.addParam({QStringLiteral("max_results"),
                   QStringLiteral("Maximum number of results to return (default 8, max 20)"),
                   {Type::Integer}});
    spec.addParam({QStringLiteral("source"),
                   QStringLiteral("API source: crossref, openalex, or auto (default auto: try CrossRef first, fall back to OpenAlex on failure)"),
                   {Type::String},
                   false,
                   {},
                   {QStringLiteral("auto"), QStringLiteral("crossref"), QStringLiteral("openalex")},
                   QStringLiteral("auto")});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAPaperToolSearchLiterature::execute(const QJsonObject& params)
{
    QString query = params["query"].toString().trimmed();
    if (query.isEmpty()) {
        return errorResponse("query is required");
    }
    int maxResults = params.contains("max_results") ? params["max_results"].toInt() : 8;
    if (maxResults < 1) {
        maxResults = 1;
    } else if (maxResults > 20) {
        maxResults = 20;
    }
    QString source = params.contains("source") ? params["source"].toString().toLower() : QStringLiteral("auto");

    QJsonArray results;
    QString usedSource;
    QString error;
    if (source == QStringLiteral("crossref")) {
        if (!DAPaperLiteratureApi::crossrefSearch(query, maxResults, results, error)) {
            return errorResponse(QString("CrossRef search failed: %1").arg(error));
        }
        usedSource = QStringLiteral("crossref");
    } else if (source == QStringLiteral("openalex")) {
        if (!DAPaperLiteratureApi::openalexSearch(query, maxResults, results, error)) {
            return errorResponse(QString("OpenAlex search failed: %1").arg(error));
        }
        usedSource = QStringLiteral("openalex");
    } else {
        // auto：CrossRef 优先，失败降级 OpenAlex；两者皆败才报错
        if (DAPaperLiteratureApi::crossrefSearch(query, maxResults, results, error)) {
            usedSource = QStringLiteral("crossref");
        } else {
            QString crossrefError = error;
            if (DAPaperLiteratureApi::openalexSearch(query, maxResults, results, error)) {
                usedSource = QStringLiteral("openalex");
            } else {
                return errorResponse(QString("Both sources failed. CrossRef: %1 | OpenAlex: %2").arg(crossrefError, error));
            }
        }
    }

    QJsonObject data;
    data["query"]     = query;
    data["source"]    = usedSource;
    data["count"]     = results.size();
    data["results"]   = results;
    data["note"]      = QStringLiteral(
        "Entries come from public metadata APIs and may be incomplete. Verify with verify_doi before citing.");
    return successResponse(data);
}
}  // namespace DA
