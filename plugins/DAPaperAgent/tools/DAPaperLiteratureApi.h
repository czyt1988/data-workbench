#pragma once
#include <QJsonArray>
#include <QJsonObject>

namespace DA
{
/**
 * @brief 文献检索工具共享的 HTTP 与 API 辅助函数（DAPaperAgent 插件内部使用）
 *
 * 封装对 CrossRef / OpenAlex 两个免费公开学术元数据 API 的同步访问：
 * httpGetJson 提供"QEventLoop 同步等待 + 超时 abort"的 GET（模式参考
 * DAProviderEditDialog::onFetchModels 与 DAAppProject::waitArchiveSave），
 * 其余函数负责请求构造与响应条目的规范化（统一字段：doi/title/authors/
 * journal/year/type/url）。
 */
namespace DAPaperLiteratureApi
{
/// 同步 GET 并解析 JSON；失败返回 false 并填充 error（网络错误/超时/非 JSON）
bool httpGetJson(const QString& url, QJsonObject& out, QString& error, int timeoutMs = 10000);

/// CrossRef 作品对象 → 规范化条目 {doi,title,authors,journal,year,type,url}
QJsonObject normalizeCrossrefWork(const QJsonObject& work);

/// OpenAlex 作品对象 → 规范化条目 {doi,title,authors,journal,year,type,url}
QJsonObject normalizeOpenAlexWork(const QJsonObject& work);

/// CrossRef 检索：GET api.crossref.org/works?query=...&rows=N；成功填充 out 数组
bool crossrefSearch(const QString& query, int maxResults, QJsonArray& out, QString& error);

/// OpenAlex 检索：GET api.openalex.org/works?search=...&per-page=N；成功填充 out 数组
bool openalexSearch(const QString& query, int maxResults, QJsonArray& out, QString& error);

/// CrossRef DOI 元数据查询：GET api.crossref.org/works/{doi}；成功填充规范条目
bool crossrefDoi(const QString& doi, QJsonObject& out, QString& error);
}
}  // namespace DA
