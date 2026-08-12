// DAAgentSessionStore.h
#pragma once
#include "DAAgentAPI.h"
#include <QString>
#include <QVector>
#include <QHash>
#include <QStringList>
#include <QJsonArray>
#include <QJsonObject>
#include "DAGlobals.h"

namespace DA
{
/**
 * @brief 会话持久化层：JSONL 读写 / 索引 / 清理 / 标题 / last_active 指针
 *
 * 非 QObject（纯文件 IO 工具，无信号槽需求）。PIMPL 模式。
 *
 * 路径（总纲 T7）：
 *   - 会话文件：<appData>/sessions/<sessionId>.jsonl
 *   - 全局索引：<appData>/sessions/sessions_index.json
 *   - 上次活跃指针：<appData>/sessions/last_active.json
 *
 * JSONL 记录格式（总纲 T6）：
 *   {"uuid":"...","parent_uuid":null,"session_id":"...","timestamp":"ISO8601",
 *    "type":"user|assistant|tool_result|usage",
 *    "message":{"role":"human|ai|tool","content":"...","tool_calls":[...]?,"tool_call_id":"..."?},
 *    "usage_metadata":{...}?}
 *
 * 一期 type 实际用 user/assistant/tool_result/usage；
 * question/answer 复用 tool_call/tool_result 语义（ask_user 作为 assistant 的 tool_call）；
 * parent_uuid 一期固定 null（二期 rewind 用）。
 *
 * @note 线程安全：Bridge 信号在主线程触发，appendRecord 同步写文件，无并发。
 *       若 IO 慢阻塞主线程，二期改异步队列；一期同步可接受。
 */
class DAAgent_API DAAgentSessionStore
{
public:
    /**
     * @brief 会话元数据（索引项）
     */
    struct SessionMeta {
        QString id;
        QString title;
        QString createdAt;   ///< ISO8601
        QString updatedAt;   ///< ISO8601
        int messageCount = 0;
        QString projectPath; ///< 空=自由会话；非空=绑定工程
    };

    DAAgentSessionStore();
    ~DAAgentSessionStore();

    // ---- 会话生命周期 ----
    // 创建新会话（写 index + 空 jsonl）
    QString createSession(const QString& projectPath = QString());
    // 删除会话（删 jsonl 文件 + 从 index 移除）
    void deleteSession(const QString& id);
    // 重命名会话（更新 index 的 title + updatedAt）
    void renameSession(const QString& id, const QString& title);
    // 列出会话（按 projectPath 过滤，按 updatedAt 倒序）
    QVector<SessionMeta> listSessions(const QString& projectPathFilter = QString()) const;
    // 判断 id 是否存在于 index
    bool hasSession(const QString& id) const;

    // ---- 记录读写 ----
    // 追加一条 JSONL 记录（崩溃安全：每条即写 flush），同时更新 index 的 updatedAt 与 messageCount
    void appendRecord(const QString& sessionId, const QJsonObject& record);
    // 读取会话消息用于 load_session 重建 state
    QJsonArray readMessagesForLoad(const QString& sessionId) const;
    // 读取会话全量记录（UI 重放用，含 usage 记录）
    QVector<QJsonObject> readAllRecords(const QString& sessionId) const;

    // ---- 指针 ----
    // 读取上次活跃会话 ID（按工程过滤；空=自由会话上次活跃）
    QString lastActiveSession(const QString& projectPathFilter = QString()) const;
    // 设置上次活跃会话指针
    void setLastActive(const QString& sessionId, const QString& projectPath = QString());

    // ---- 清理 ----
    // 按数量+时间双限清理旧会话
    void cleanupOldSessions(int maxCount, int retentionDays, const QString& skipSessionId = QString());

    // ---- 工程导入导出（plan-05 调用） ----
    // 导出指定会话的 JSONL 字节（供工程 zip 保存）
    QHash<QString, QByteArray> exportSessionFiles(const QStringList& sessionIds) const;
    // 导入工程内会话文件（写 sessions/ + 更新 index + 标记导入会话 projectPath）
    void importSessionFiles(const QHash<QString, QByteArray>& files, const QString& projectPath);
    // 更新 index 中该会话的 projectPath 字段（原子写 tmp+rename）
    void setSessionProjectPath(const QString& sessionId, const QString& projectPath);

    // ---- 自动标题 ----
    // 读首条 user 记录，若 title 空则取其 content 首行截断为简短标题
    bool ensureTitle(const QString& sessionId);

private:
    DA_DECLARE_PRIVATE(DAAgentSessionStore)
};
} // namespace DA
