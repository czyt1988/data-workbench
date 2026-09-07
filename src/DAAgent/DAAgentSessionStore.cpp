// DAAgentSessionStore.cpp
#include "DAAgentSessionStore.h"
#include "DADir.h"
#include "DALogCategory.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QVariant>
#include <QSet>
#include <algorithm>

namespace DA
{

// ===========================================================================
// PrivateData
// ===========================================================================
class DAAgentSessionStore::PrivateData
{
public:
    PrivateData(DAAgentSessionStore*) {}  // 接收 q 指针但不存储（当前无需反向访问 public 类）

    /// sessions 目录路径：<appData>/sessions/（getAppDataPath 内部已 mkpath）
    QString sessionsDir() const { return DADir::getAppDataPath("sessions"); }
    QString sessionFilePath(const QString& id) const { return sessionsDir() + "/" + id + ".jsonl"; }
    QString indexFilePath() const { return sessionsDir() + "/sessions_index.json"; }
    QString lastActiveFilePath() const { return sessionsDir() + "/last_active.json"; }

    // index 读写（不带日志的内部版本）
    QVector<SessionMeta> readIndexInternal() const;
    bool writeIndex(const QVector<SessionMeta>& metas) const;  // 原子写 tmp+rename

    // last_active 读写
    QString readLastActive(const QString& projectPathFilter) const;
    bool writeLastActive(const QString& sessionId, const QString& projectPath) const;
    // 读原始指针（不过滤 projectPath，cleanup 跳过用——last_active 全局唯一）
    bool readLastActivePointer(QString& sid, QString& projectPath) const;

    // 跳过损坏行的 JSONL 行解析
    QJsonObject parseLineTolerant(const QByteArray& line, const QString& sessionId) const;

    // 扫描 JSONL usage 记录求 token 累计（旧索引懒迁移/导入统计用）
    void computeTokenStats(const QString& sessionId, qint64& in, qint64& out, qint64& total) const;
};

// ===========================================================================
// ctor / dtor
// ===========================================================================
DAAgentSessionStore::DAAgentSessionStore()
    : d_ptr(std::make_unique<PrivateData>(this))
{
}

DAAgentSessionStore::~DAAgentSessionStore() = default;

// ===========================================================================
// 会话生命周期
// ===========================================================================
QString DAAgentSessionStore::createSession(const QString& projectPath)
{
    DA_D(d);
    QString sid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    // 创建空 jsonl 文件（占位，确保文件存在）
    QFile f(d->sessionFilePath(sid));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentSessionStore: failed to create session file:" << f.fileName()
                  << "error:" << f.errorString();  // cn: 创建会话文件失败
    } else {
        f.close();
    }

    // 写 index
    QVector<SessionMeta> metas = d->readIndexInternal();
    SessionMeta m;
    m.id          = sid;
    m.title       = QString();
    m.createdAt   = now;
    m.updatedAt   = now;
    m.messageCount = 0;
    m.projectPath = projectPath;
    m.inputTokens  = 0;
    m.outputTokens = 0;
    m.totalTokens  = 0;
    metas.append(m);
    d->writeIndex(metas);

    return sid;
}

void DAAgentSessionStore::deleteSession(const QString& id)
{
    DA_D(d);
    if (id.isEmpty()) return;

    // 删 jsonl 文件
    QFile f(d->sessionFilePath(id));
    if (f.exists() && !f.remove()) {
        qWarning() << "DAAgentSessionStore: failed to remove session file:" << f.fileName()
                  << "error:" << f.errorString();  // cn: 删除会话文件失败
    }

    // 从 index 移除
    QVector<SessionMeta> metas = d->readIndexInternal();
    for (int i = 0; i < metas.size(); ++i) {
        if (metas[i].id == id) {
            metas.removeAt(i);
            break;
        }
    }
    d->writeIndex(metas);
}

void DAAgentSessionStore::renameSession(const QString& id, const QString& title)
{
    DA_D(d);
    QVector<SessionMeta> metas = d->readIndexInternal();
    bool changed = false;
    for (auto& m : metas) {
        if (m.id == id) {
            m.title    = title;
            m.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            changed    = true;
            break;
        }
    }
    if (changed) d->writeIndex(metas);
}

QVector<DAAgentSessionStore::SessionMeta> DAAgentSessionStore::listSessions(const QString& projectPathFilter) const
{
    DA_DC(d);
    QVector<SessionMeta> metas = d->readIndexInternal();

    // 旧索引懒迁移：token 字段缺失（-1 哨兵）时扫 JSONL usage 记录回填并持久化。
    // 一次性成本（会话数受 max_sessions 约束），迁移后 sessionListChanged 高频调用不再触发扫描。
    bool migrated = false;
    for (auto& m : metas) {
        if (m.totalTokens >= 0) continue;
        d->computeTokenStats(m.id, m.inputTokens, m.outputTokens, m.totalTokens);
        migrated = true;
    }
    if (migrated) d->writeIndex(metas);

    // 按 projectPath 过滤
    if (!projectPathFilter.isEmpty()) {
        QVector<SessionMeta> filtered;
        filtered.reserve(metas.size());
        for (const auto& m : metas) {
            if (m.projectPath == projectPathFilter) filtered.append(m);
        }
        metas = filtered;
    }

    // 按 updatedAt 倒序
    std::sort(metas.begin(), metas.end(), [](const SessionMeta& a, const SessionMeta& b) {
        return a.updatedAt > b.updatedAt;
    });
    return metas;
}

bool DAAgentSessionStore::hasSession(const QString& id) const
{
    DA_DC(d);
    if (id.isEmpty()) return false;
    QVector<SessionMeta> metas = d->readIndexInternal();
    for (const auto& m : metas) {
        if (m.id == id) return true;
    }
    return false;
}

int DAAgentSessionStore::messageCount(const QString& id) const
{
    DA_DC(d);
    if (id.isEmpty()) return -1;
    QVector<SessionMeta> metas = d->readIndexInternal();
    for (const auto& m : metas) {
        if (m.id == id) return m.messageCount;
    }
    return -1;
}

// ===========================================================================
// 记录读写
// ===========================================================================
void DAAgentSessionStore::appendRecord(const QString& sessionId, const QJsonObject& record)
{
    DA_D(d);
    if (sessionId.isEmpty()) return;

    // 崩溃安全：Append|Text|WriteOnly，每条即写 flush
    QFile f(d->sessionFilePath(sessionId));
    if (!f.open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly)) {
        qWarning() << "DAAgentSessionStore: failed to open session file for append:" << f.fileName()
                  << "error:" << f.errorString();  // cn: 打开会话文件追加失败
        return;
    }
    QByteArray line = QJsonDocument(record).toJson(QJsonDocument::Compact);
    f.write(line);
    f.write("\n");
    f.flush();
    f.close();

    // 更新 index 的 updatedAt + messageCount（仅对 user/assistant/tool_result 计数，
    // usage 记录不计入对话消息数）+ token 累计（仅 usage 记录）
    QVector<SessionMeta> metas = d->readIndexInternal();
    bool changed = false;
    for (auto& m : metas) {
        if (m.id == sessionId) {
            m.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            QString type = record.value("type").toString();
            if (type == "user" || type == "assistant" || type == "tool_result") {
                m.messageCount += 1;
            } else if (type == "usage" && m.totalTokens >= 0) {
                // 索引缓存已累计（≥0）才就地累加；-1（旧索引未迁移）留给 listSessions 懒迁移全量回填
                QJsonObject um = record.value("usage_metadata").toObject();
                m.inputTokens  += static_cast<qint64>(um.value("input_tokens").toDouble(0));
                m.outputTokens += static_cast<qint64>(um.value("output_tokens").toDouble(0));
                m.totalTokens  += static_cast<qint64>(um.value("total_tokens").toDouble(0));
            }
            changed = true;
            break;
        }
    }
    if (changed) d->writeIndex(metas);
}

QJsonArray DAAgentSessionStore::readMessagesForLoad(const QString& sessionId) const
{
    DA_DC(d);
    QJsonArray out;
    if (sessionId.isEmpty()) return out;

    QFile f(d->sessionFilePath(sessionId));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return out;  // 文件不存在或不可读：空数组（容错）
    }

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.isEmpty()) continue;
        QJsonObject obj = d->parseLineTolerant(line, sessionId);
        if (obj.isEmpty()) continue;  // 跳过损坏行
        QString type = obj.value("type").toString();
        if (type != "user" && type != "assistant" && type != "tool_result") continue;  // 过滤 usage
        out.append(obj.value("message").toObject());
    }
    f.close();
    return out;
}

QVector<QJsonObject> DAAgentSessionStore::readAllRecords(const QString& sessionId) const
{
    DA_DC(d);
    QVector<QJsonObject> out;
    if (sessionId.isEmpty()) return out;

    QFile f(d->sessionFilePath(sessionId));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return out;
    }

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.isEmpty()) continue;
        QJsonObject obj = d->parseLineTolerant(line, sessionId);
        if (obj.isEmpty()) continue;
        out.append(obj);
    }
    f.close();
    return out;
}

// ===========================================================================
// 指针
// ===========================================================================
QString DAAgentSessionStore::lastActiveSession(const QString& projectPathFilter) const
{
    DA_DC(d);
    return d->readLastActive(projectPathFilter);
}

void DAAgentSessionStore::setLastActive(const QString& sessionId, const QString& projectPath)
{
    DA_D(d);
    d->writeLastActive(sessionId, projectPath);
}

// ===========================================================================
// 清理
// ===========================================================================
void DAAgentSessionStore::cleanupOldSessions(int maxCount, int retentionDays, const QString& skipSessionId)
{
    DA_D(d);
    // plan-06 边界防护：ini 被手改为 0/负时钳到合法下限，避免「保留 0 个」误删全部。
    // qMax(1, maxCount)：maxCount>=1，数量限制始终生效（至少保留 1 个）；
    // qMax(0, retentionDays)：负数→0，0 表示「不按时间清理」（见下方 if (retentionDays > 0)）。
    maxCount = qMax(1, maxCount);
    retentionDays = qMax(0, retentionDays);
    if (maxCount <= 0 && retentionDays <= 0) return;

    QVector<SessionMeta> metas = d->readIndexInternal();
    // 跳过 last_active 指针指向的会话（不过滤 projectPath——last_active 全局唯一，
    // cleanup 保护上次活跃，无论自由会话还是工程绑定会话；故用 raw 读取而非
    // readLastActive(filter)——后者空 filter 只返回自由会话会漏保工程绑定会话）
    QString lastActive;
    {
        QString tmpP;
        d->readLastActivePointer(lastActive, tmpP);
    }

    // 按 updatedAt 倒序排序，保留最新
    std::sort(metas.begin(), metas.end(), [](const SessionMeta& a, const SessionMeta& b) {
        return a.updatedAt > b.updatedAt;
    });

    QDateTime retentionBoundary;
    if (retentionDays > 0) {
        retentionBoundary = QDateTime::currentDateTimeUtc().addDays(-retentionDays);
    }

    QVector<SessionMeta> survivors;
    QSet<QString> idsToDelete;

    for (int i = 0; i < metas.size(); ++i) {
        const SessionMeta& m = metas[i];
        bool skip = (!skipSessionId.isEmpty() && m.id == skipSessionId)
                    || m.id == lastActive;
        if (skip) {
            survivors.append(m);
            continue;
        }
        bool tooOld = false;
        if (retentionDays > 0) {
            QDateTime updated = QDateTime::fromString(m.updatedAt, Qt::ISODateWithMs);
            if (!updated.isValid()) updated = QDateTime::fromString(m.updatedAt, Qt::ISODate);
            if (updated.isValid() && updated < retentionBoundary) tooOld = true;
        }
        bool overCount = false;
        if (maxCount > 0) {
            // survivors 中非 skip 的会话计数
            int nonSkipSurvivors = 0;
            for (const auto& s : survivors) {
                if (s.id == skipSessionId || s.id == lastActive) continue;
                ++nonSkipSurvivors;
            }
            if (nonSkipSurvivors >= maxCount) overCount = true;
        }
        if (tooOld || overCount) {
            idsToDelete.insert(m.id);
        } else {
            survivors.append(m);
        }
    }

    // 删除标记的会话文件
    for (const QString& id : idsToDelete) {
        QFile f(d->sessionFilePath(id));
        if (f.exists() && !f.remove()) {
            qWarning() << "DAAgentSessionStore: cleanup failed to remove file:" << f.fileName()
                      << "error:" << f.errorString();  // cn: 清理时删除文件失败
        }
    }

    // 重写 index：survivors 中去掉被删除的
    if (!idsToDelete.isEmpty()) {
        QVector<SessionMeta> finalMetas;
        for (const auto& m : survivors) {
            if (!idsToDelete.contains(m.id)) finalMetas.append(m);
        }
        d->writeIndex(finalMetas);
    }
}

// ===========================================================================
// 工程导入导出
// ===========================================================================
QHash<QString, QByteArray> DAAgentSessionStore::exportSessionFiles(const QStringList& sessionIds) const
{
    DA_DC(d);
    QHash<QString, QByteArray> out;
    for (const QString& id : sessionIds) {
        if (id.isEmpty()) continue;
        QFile f(d->sessionFilePath(id));
        if (!f.open(QIODevice::ReadOnly)) continue;
        out.insert(id, f.readAll());
        f.close();
    }
    return out;
}

void DAAgentSessionStore::importSessionFiles(const QHash<QString, QByteArray>& files, const QString& projectPath)
{
    DA_D(d);
    QVector<SessionMeta> metas = d->readIndexInternal();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    for (auto it = files.begin(); it != files.end(); ++it) {
        const QString& id = it.key();
        if (id.isEmpty()) continue;

        // 写 jsonl 文件
        QFile f(d->sessionFilePath(id));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "DAAgentSessionStore: import failed to write file:" << f.fileName()
                      << "error:" << f.errorString();  // cn: 导入会话文件写入失败
            continue;
        }
        f.write(it.value());
        f.close();

        // 统计消息数（仅 user/assistant/tool_result）与 token 消耗（usage 记录），同一遍扫描完成
        int msgCount = 0;
        qint64 inTokens = 0, outTokens = 0, totTokens = 0;
        {
            QFile rf(d->sessionFilePath(id));
            if (rf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!rf.atEnd()) {
                    QByteArray line = rf.readLine();
                    if (line.isEmpty()) continue;
                    QJsonObject obj = d->parseLineTolerant(line, id);
                    if (obj.isEmpty()) continue;
                    QString type = obj.value("type").toString();
                    if (type == "user" || type == "assistant" || type == "tool_result") {
                        ++msgCount;
                    } else if (type == "usage") {
                        QJsonObject um = obj.value("usage_metadata").toObject();
                        inTokens  += static_cast<qint64>(um.value("input_tokens").toDouble(0));
                        outTokens += static_cast<qint64>(um.value("output_tokens").toDouble(0));
                        totTokens += static_cast<qint64>(um.value("total_tokens").toDouble(0));
                    }
                }
                rf.close();
            }
        }

        // 取首条 user 的 timestamp 作为创建时间（若无则用 now）
        QString createdAt = now;
        {
            QFile rf(d->sessionFilePath(id));
            if (rf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!rf.atEnd()) {
                    QByteArray line = rf.readLine();
                    if (line.isEmpty()) continue;
                    QJsonObject obj = d->parseLineTolerant(line, id);
                    if (obj.isEmpty()) continue;
                    if (obj.value("type").toString() == "user") {
                        createdAt = obj.value("timestamp").toString(now);
                        break;
                    }
                }
                rf.close();
            }
        }

        // 更新或新增 index 项
        bool found = false;
        for (auto& m : metas) {
            if (m.id == id) {
                m.projectPath = projectPath;
                m.messageCount = msgCount;
                m.inputTokens  = inTokens;
                m.outputTokens = outTokens;
                m.totalTokens  = totTokens;
                m.updatedAt = now;
                found = true;
                break;
            }
        }
        if (!found) {
            SessionMeta m;
            m.id = id;
            m.title = QString();
            m.createdAt = createdAt;
            m.updatedAt = now;
            m.messageCount = msgCount;
            m.projectPath = projectPath;
            m.inputTokens  = inTokens;
            m.outputTokens = outTokens;
            m.totalTokens  = totTokens;
            metas.append(m);
        }
    }
    d->writeIndex(metas);
}

void DAAgentSessionStore::setSessionProjectPath(const QString& sessionId, const QString& projectPath)
{
    DA_D(d);
    QVector<SessionMeta> metas = d->readIndexInternal();
    bool changed = false;
    for (auto& m : metas) {
        if (m.id == sessionId) {
            m.projectPath = projectPath;
            m.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            changed = true;
            break;
        }
    }
    if (changed) d->writeIndex(metas);
}

// ===========================================================================
// 自动标题
// ===========================================================================
bool DAAgentSessionStore::ensureTitle(const QString& sessionId)
{
    DA_D(d);
    if (sessionId.isEmpty()) return false;

    QVector<SessionMeta> metas = d->readIndexInternal();
    SessionMeta* target = nullptr;
    for (auto& m : metas) {
        if (m.id == sessionId) { target = &m; break; }
    }
    if (!target) return false;
    if (!target->title.isEmpty()) return false;  // 已有标题，不动

    // 读首条 user 记录的 content
    QFile f(d->sessionFilePath(sessionId));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString firstUserContent;
    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.isEmpty()) continue;
        QJsonObject obj = d->parseLineTolerant(line, sessionId);
        if (obj.isEmpty()) continue;
        if (obj.value("type").toString() == "user") {
            firstUserContent = obj.value("message").toObject().value("content").toString();
            break;
        }
    }
    f.close();

    if (firstUserContent.isEmpty()) return false;

    // 取首行、trim、超长截断加省略号，生成简短会话名（首行更干净，避免多行堆砌）
    const int MaxTitleChars = 20;
    QString title = firstUserContent.section('\n', 0, 0).trimmed();
    if (title.isEmpty()) title = firstUserContent.trimmed();
    if (title.isEmpty()) return false;
    if (title.size() > MaxTitleChars) {
        title = title.left(MaxTitleChars) + QStringLiteral("…");
    }

    target->title = title;
    target->updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    d->writeIndex(metas);
    return true;
}

// ===========================================================================
// PrivateData 实现
// ===========================================================================
QVector<DAAgentSessionStore::SessionMeta> DAAgentSessionStore::PrivateData::readIndexInternal() const
{
    QVector<SessionMeta> out;
    QFile f(indexFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return out;  // 文件不存在：返回空（首次运行正常）
    }
    QByteArray data = f.readAll();
    f.close();
    if (data.trimmed().isEmpty()) return out;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "DAAgentSessionStore: index file corrupted:" << err.errorString()
                  << "at:" << indexFilePath();  // cn: 索引文件损坏
        return out;
    }
    QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        SessionMeta m;
        m.id          = o.value("id").toString();
        m.title       = o.value("title").toString();
        m.createdAt   = o.value("createdAt").toString();
        m.updatedAt   = o.value("updatedAt").toString();
        m.messageCount = o.value("messageCount").toInt(0);
        m.projectPath = o.value("projectPath").toString();
        // token 字段：旧索引缺失时读为 -1 哨兵（listSessions 懒迁移回填）
        m.inputTokens  = static_cast<qint64>(o.value("inputTokens").toDouble(-1));
        m.outputTokens = static_cast<qint64>(o.value("outputTokens").toDouble(-1));
        m.totalTokens  = static_cast<qint64>(o.value("totalTokens").toDouble(-1));
        if (!m.id.isEmpty()) out.append(m);
    }
    return out;
}

bool DAAgentSessionStore::PrivateData::writeIndex(const QVector<SessionMeta>& metas) const
{
    QJsonArray arr;
    for (const auto& m : metas) {
        QJsonObject o;
        o["id"]          = m.id;
        o["title"]       = m.title;
        o["createdAt"]   = m.createdAt;
        o["updatedAt"]   = m.updatedAt;
        o["messageCount"] = m.messageCount;
        o["projectPath"] = m.projectPath;
        // JSON 数值以 double 存储（Qt5 QJsonValue 无 qint64 构造），token 量级下无精度问题
        o["inputTokens"]  = static_cast<double>(m.inputTokens);
        o["outputTokens"] = static_cast<double>(m.outputTokens);
        o["totalTokens"]  = static_cast<double>(m.totalTokens);
        arr.append(o);
    }
    QJsonDocument doc(arr);

    // 原子写：tmp + rename
    QString tmpPath = indexFilePath() + ".tmp";
    QFile tf(tmpPath);
    if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentSessionStore: failed to open index tmp file for write:" << tmpPath
                  << "error:" << tf.errorString();  // cn: 打开索引临时文件失败
        return false;
    }
    tf.write(doc.toJson(QJsonDocument::Compact));
    tf.flush();
    tf.close();

    // rename 覆盖（QFile::rename 在目标存在时失败，先 remove）
    QFile::remove(indexFilePath());
    if (!tf.rename(indexFilePath())) {
        qWarning() << "DAAgentSessionStore: failed to rename index tmp file:" << tmpPath
                  << "->" << indexFilePath();  // cn: 重命名索引临时文件失败
        return false;
    }
    return true;
}

QString DAAgentSessionStore::PrivateData::readLastActive(const QString& projectPathFilter) const
{
    QString sid, p;
    if (!readLastActivePointer(sid, p)) return QString();
    // 精确匹配：空 filter 只返回自由会话（指针 projectPath 必须空）；
    // 非空 filter 精确匹配工程路径。避免启动恢复把工程绑定会话当自由会话。
    if (projectPathFilter.isEmpty()) {
        if (!p.isEmpty()) return QString();
    } else if (p != projectPathFilter) {
        return QString();
    }
    return sid;
}

bool DAAgentSessionStore::PrivateData::readLastActivePointer(QString& sid, QString& projectPath) const
{
    sid.clear();
    projectPath.clear();
    QFile f(lastActiveFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QByteArray data = f.readAll();
    f.close();
    if (data.trimmed().isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "DAAgentSessionStore: last_active.json corrupted:" << err.errorString();
        return false;
    }
    QJsonObject o = doc.object();
    sid         = o.value("sessionId").toString();
    projectPath = o.value("projectPath").toString();
    return true;
}

bool DAAgentSessionStore::PrivateData::writeLastActive(const QString& sessionId, const QString& projectPath) const
{
    QJsonObject o;
    o["sessionId"]   = sessionId;
    o["projectPath"] = projectPath;
    QJsonDocument doc(o);

    QString tmpPath = lastActiveFilePath() + ".tmp";
    QFile tf(tmpPath);
    if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentSessionStore: failed to open last_active tmp file:" << tmpPath
                  << "error:" << tf.errorString();  // cn: 打开 last_active 临时文件失败
        return false;
    }
    tf.write(doc.toJson(QJsonDocument::Compact));
    tf.flush();
    tf.close();

    QFile::remove(lastActiveFilePath());
    if (!tf.rename(lastActiveFilePath())) {
        qWarning() << "DAAgentSessionStore: failed to rename last_active tmp file:" << tmpPath
                  << "->" << lastActiveFilePath();  // cn: 重命名 last_active 临时文件失败
        return false;
    }
    return true;
}

QJsonObject DAAgentSessionStore::PrivateData::parseLineTolerant(const QByteArray& line, const QString& sessionId) const
{
    QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty()) return QJsonObject();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(trimmed, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "DAAgentSessionStore: skipping corrupted JSONL line in session" << sessionId
                  << ":" << err.errorString();  // cn: 跳过损坏的 JSONL 行
        return QJsonObject();
    }
    return doc.object();
}

void DAAgentSessionStore::PrivateData::computeTokenStats(const QString& sessionId, qint64& in, qint64& out, qint64& total) const
{
    in = 0;
    out = 0;
    total = 0;
    QFile f(sessionFilePath(sessionId));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;  // 文件不存在/不可读：视为 0 消耗（容错）
    }
    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.isEmpty()) continue;
        QJsonObject obj = parseLineTolerant(line, sessionId);
        if (obj.isEmpty()) continue;  // 跳过损坏行
        if (obj.value("type").toString() != QLatin1String("usage")) continue;
        QJsonObject um = obj.value("usage_metadata").toObject();
        in    += static_cast<qint64>(um.value("input_tokens").toDouble(0));
        out   += static_cast<qint64>(um.value("output_tokens").toDouble(0));
        total += static_cast<qint64>(um.value("total_tokens").toDouble(0));
    }
    f.close();
}

} // namespace DA
