#include "DAPaperLiteratureApi.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace DA
{
namespace DAPaperLiteratureApi
{
// 同步等待时长：学术 API 偶发慢响应，10s 内 abort 避免工具长时间卡住事件循环
static constexpr int HTTP_TIMEOUT_MS = 10000;

/**
 * @brief 从环境变量解析代理（受限网络的常见约定）
 *
 * 学术 API（CrossRef/OpenAlex）在部分内网环境需经代理访问。支持
 * https_proxy/HTTPS_PROXY/ http_proxy/HTTP_PROXY/ all_proxy 指定的
 * http://host:port 形式；未设置时保持 Qt 默认（直连）。
 */
static QNetworkProxy proxyFromEnv()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QStringList keys = { QStringLiteral("https_proxy"), QStringLiteral("HTTPS_PROXY"),
                               QStringLiteral("http_proxy"), QStringLiteral("HTTP_PROXY"),
                               QStringLiteral("all_proxy"), QStringLiteral("ALL_PROXY") };
    for (const QString& key : keys) {
        QString value = env.value(key).trimmed();
        if (value.isEmpty()) {
            continue;
        }
        if (!value.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)) {
            continue;  // 仅支持 http:// 形式的 CONNECT 代理，socks 等忽略
        }
        QUrl proxyUrl(value);
        if (proxyUrl.port() > 0) {
            return QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), static_cast< quint16 >(proxyUrl.port()));
        }
    }
    return QNetworkProxy();
}

/**
 * @brief 同步 GET 并解析 JSON
 *
 * QNetworkAccessManager 异步信号经 QEventLoop（ExcludeUserInputEvents，
 * 防用户输入重入）同步等待；超时经 QTimer abort。工具 execute 在主线程
 * 同步调用（见 DAAgentBridge::executeToolNow），此模式是项目内轻量同步
 * HTTP 的唯一可行路径（模式参考 DAAppProject::waitArchiveSave）。
 */
bool httpGetJson(const QString& url, QJsonObject& out, QString& error, int timeoutMs)
{
    QNetworkAccessManager nam;
    QNetworkProxy proxy = proxyFromEnv();
    if (proxy.type() != QNetworkProxy::DefaultProxy) {
        nam.setProxy(proxy);
    }
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "data-workbench DAPaperAgent plugin");
    QNetworkReply* reply = nam.get(request);
    QTimer::singleShot(timeoutMs, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        reply->deleteLater();
        error = QString("Request timeout after %1 ms: %2").arg(timeoutMs).arg(url);
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        // HTTP 404 等状态错误保留服务端原文，便于上层区分"不存在"与"网络故障"
        QByteArray body = reply->readAll();
        error = QString("Network error (%1): %2").arg(reply->error()).arg(reply->errorString());
        if (!body.isEmpty()) {
            error += QString(" | %1").arg(QString::fromUtf8(body.left(200)));
        }
        reply->deleteLater();
        return false;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError) {
        error = QString("Invalid JSON response: %1").arg(parseError.errorString());
        return false;
    }
    if (!doc.isObject()) {
        error = "Response is not a JSON object";
        return false;
    }
    out = doc.object();
    return true;
}

/**
 * @brief CrossRef 作品对象 → 规范化条目
 */
QJsonObject normalizeCrossrefWork(const QJsonObject& work)
{
    QJsonObject entry;
    QString doi = work.value("DOI").toString();
    entry["doi"] = doi;
    // title/container-title 均为字符串数组，取第一个非空元素
    QString title;
    for (const QJsonValue& v : work.value("title").toArray()) {
        if (!v.toString().isEmpty()) {
            title = v.toString();
            break;
        }
    }
    entry["title"] = title;
    QJsonArray authors;
    for (const QJsonValue& v : work.value("author").toArray()) {
        QJsonObject a = v.toObject();
        QString name = (a.value("family").toString() + " " + a.value("given").toString()).trimmed();
        if (!name.isEmpty()) {
            authors.append(name);
        }
    }
    entry["authors"] = authors;
    QString journal;
    for (const QJsonValue& v : work.value("container-title").toArray()) {
        if (!v.toString().isEmpty()) {
            journal = v.toString();
            break;
        }
    }
    entry["journal"] = journal;
    // issued.date-parts = [[year, month, day], ...]，取第一组的第一个元素
    int year = 0;
    QJsonArray parts = work.value("issued").toObject().value("date-parts").toArray();
    if (!parts.isEmpty()) {
        QJsonArray first = parts.first().toArray();
        if (!first.isEmpty()) {
            year = first.first().toInt();
        }
    }
    entry["year"]     = year;
    entry["type"]     = work.value("type").toString();
    entry["url"]      = QStringLiteral("https://doi.org/") + doi;
    return entry;
}

/**
 * @brief OpenAlex 作品对象 → 规范化条目
 */
QJsonObject normalizeOpenAlexWork(const QJsonObject& work)
{
    QJsonObject entry;
    // OpenAlex 的 doi 字段带 https://doi.org/ 前缀，规范化为裸 DOI
    QString doiUrl = work.value("doi").toString();
    QString doi    = doiUrl;
    if (doi.startsWith(QStringLiteral("https://doi.org/"), Qt::CaseInsensitive)) {
        doi = doi.mid(QStringLiteral("https://doi.org/").length());
    }
    entry["doi"] = doi;
    // title 可能为 null（旧作品），回退 display_name
    QString title = work.value("title").toString();
    if (title.isEmpty()) {
        title = work.value("display_name").toString();
    }
    entry["title"] = title;
    QJsonArray authors;
    for (const QJsonValue& v : work.value("authorships").toArray()) {
        QString name = v.toObject().value("author").toObject().value("display_name").toString();
        if (!name.isEmpty()) {
            authors.append(name);
        }
    }
    entry["authors"]  = authors;
    entry["journal"]  = work.value("primary_location").toObject().value("source").toObject().value("display_name").toString();
    entry["year"]     = work.value("publication_year").toInt();
    entry["type"]     = work.value("type").toString();
    entry["url"]      = QStringLiteral("https://doi.org/") + doi;
    return entry;
}

/**
 * @brief CrossRef 检索
 */
bool crossrefSearch(const QString& query, int maxResults, QJsonArray& out, QString& error)
{
    QUrl url(QStringLiteral("https://api.crossref.org/works"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("query"), query);
    q.addQueryItem(QStringLiteral("rows"), QString::number(maxResults));
    url.setQuery(q);
    QJsonObject resp;
    if (!httpGetJson(url.toString(), resp, error)) {
        return false;
    }
    QJsonArray items = resp.value("message").toObject().value("items").toArray();
    for (const QJsonValue& v : items) {
        out.append(normalizeCrossrefWork(v.toObject()));
    }
    return true;
}

/**
 * @brief OpenAlex 检索
 */
bool openalexSearch(const QString& query, int maxResults, QJsonArray& out, QString& error)
{
    QUrl url(QStringLiteral("https://api.openalex.org/works"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("search"), query);
    q.addQueryItem(QStringLiteral("per-page"), QString::number(maxResults));
    url.setQuery(q);
    QJsonObject resp;
    if (!httpGetJson(url.toString(), resp, error)) {
        return false;
    }
    QJsonArray items = resp.value("results").toArray();
    for (const QJsonValue& v : items) {
        out.append(normalizeOpenAlexWork(v.toObject()));
    }
    return true;
}

/**
 * @brief CrossRef DOI 元数据查询
 */
bool crossrefDoi(const QString& doi, QJsonObject& out, QString& error)
{
    // DOI 含 '/' '.' 之外的保留字（如 '(' '#'），仅对非保留字符做百分号编码；
    // '/' 保留原样（CrossRef 以裸 DOI 作为路径段）
    QString encoded;
    for (const QChar& ch : doi) {
        if (ch.isLetterOrNumber() || ch == QLatin1Char('/') || ch == QLatin1Char('.') || ch == QLatin1Char('-')
            || ch == QLatin1Char('_') || ch == QLatin1Char('~')) {
            encoded += ch;
        } else {
            encoded += QString::fromUtf8(QUrl::toPercentEncoding(ch));
        }
    }
    QString url = QStringLiteral("https://api.crossref.org/works/") + encoded;
    QJsonObject resp;
    if (!httpGetJson(url, resp, error)) {
        return false;
    }
    QJsonObject work = resp.value("message").toObject();
    if (work.isEmpty()) {
        error = "DOI lookup returned an empty record";
        return false;
    }
    out = normalizeCrossrefWork(work);
    return true;
}
}  // namespace DAPaperLiteratureApi
}  // namespace DA
