// DAAgentSessionStoreTest/main.cpp
// 单元测试：DAAgentSessionStore 的 C++ 持久化逻辑（JSONL 读写 / 索引 / 清理 / last_active）
//
// 隔离策略：main() 起手调 QStandardPaths::setTestModeEnabled(true)，
// 把 AppDataLocation 重定向到临时目录（见 DADir::getAppDataPath），
// 绝不污染真实 %APPDATA%/DAWorkBench/sessions/。
// DADir::getAppDataPath() 用 const static 缓存路径，故必须在任何 SessionStore
// 调用之前打开 test 模式（test exe 进程内首次路径查询发生在此处之后）。
// 每个用例的 init() 再清空 sessions 目录，确保用例间相互独立。
//
// 不在此放 testMessageRoundTrip：message_to_json/json_to_message 是 plan-01 在
// Python 侧（agent_runner.py）定义的函数，HumanMessage/AIMessage 等是 langchain
// Python 类，不在 C++ DAWorkbench::DAAgent 库导出符号中。该序列化往返归 plan-01
// 的 Python 测试，本计划步骤 7 仅作手测兜底。

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUuid>
#include <QDateTime>

#include "DAAgentSessionStore.h"
#include "DADir.h"

class DAAgentSessionStoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();  // 每个用例前清空 sessions 目录（用例间隔离）
    void testAppendAndRead();
    void testParseTolerant();
    void testIndexAtomicWrite();
    void testCleanup();
    void testLastActive();
    void testImportSessionFiles();  // Bug1 回归：导入会话标记 projectPath + 合并
    void testProjectPathFiltering();  // Bug1 回归：listSessions(filter) 过滤 + 倒序
    void testEnsureTitle();
    void testMessageCount();  // 按会话 ID 查消息计数（newSession 空会话复用守卫依赖）

private:
    static QJsonObject makeRecord(const QString& sessionId, const QString& type, const QString& content);
    static void backdateUpdatedAt(const QString& sessionId, int daysAgo);
    static QString sessionsDir();
};

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

QString DAAgentSessionStoreTest::sessionsDir()
{
    // test 模式下返回临时目录下的 sessions 路径（不污染真实 AppData）
    return DA::DADir::getAppDataPath(QStringLiteral("sessions"));
}

void DAAgentSessionStoreTest::init()
{
    // 清空 sessions 目录内文件，确保每个用例从干净状态开始
    QDir qd(sessionsDir());
    if (qd.exists()) {
        const auto files = qd.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QFileInfo& fi : files) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}

QJsonObject DAAgentSessionStoreTest::makeRecord(const QString& sessionId, const QString& type, const QString& content)
{
    // 构造一条总纲 T6 格式记录
    QString role = QStringLiteral("ai");
    if (type == QStringLiteral("user")) role = QStringLiteral("human");
    else if (type == QStringLiteral("tool_result")) role = QStringLiteral("tool");

    QJsonObject message;
    message["role"]    = role;
    message["content"] = content;

    QJsonObject rec;
    rec["uuid"]        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rec["parent_uuid"] = QJsonValue(QJsonValue::Null);  // 一期固定 null
    rec["session_id"]  = sessionId;
    rec["timestamp"]   = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    rec["type"]        = type;
    rec["message"]     = message;
    return rec;
}

void DAAgentSessionStoreTest::backdateUpdatedAt(const QString& sessionId, int daysAgo)
{
    // 直接改写 sessions_index.json，把指定会话的 updatedAt 倒退 N 天，
    // 用于触发 cleanupOldSessions 的 retentionDays 删除分支。
    QString idxPath = sessionsDir() + QStringLiteral("/sessions_index.json");
    QFile f(idxPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QFAIL("cannot open sessions_index.json for backdate");
        return;
    }
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    f.close();

    QString oldTs = QDateTime::currentDateTimeUtc().addDays(-daysAgo).toString(Qt::ISODateWithMs);
    for (QJsonValueRef v : arr) {
        QJsonObject o = v.toObject();
        if (o.value("id").toString() == sessionId) {
            o["updatedAt"] = oldTs;
            v = o;
        }
    }

    QFile tf(idxPath);
    if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QFAIL("cannot rewrite sessions_index.json for backdate");
        return;
    }
    tf.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    tf.close();
}

// ---------------------------------------------------------------------------
// 1. appendRecord + readMessagesForLoad
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testAppendAndRead()
{
    DA::DAAgentSessionStore store;
    QString sid = store.createSession();
    QVERIFY(!sid.isEmpty());

    store.appendRecord(sid, makeRecord(sid, "user", "hello"));
    store.appendRecord(sid, makeRecord(sid, "assistant", "hi there"));
    store.appendRecord(sid, makeRecord(sid, "tool_result", "42"));
    // usage 记录不计入对话消息，readMessagesForLoad 应过滤
    QJsonObject usageRec = makeRecord(sid, "usage", QString());
    store.appendRecord(sid, usageRec);

    QJsonArray msgs = store.readMessagesForLoad(sid);
    // 仅 user/assistant/tool_result 三条，usage 被过滤
    QCOMPARE(msgs.size(), 3);
    QCOMPARE(msgs.at(0).toObject().value("content").toString(), QString("hello"));
    QCOMPARE(msgs.at(1).toObject().value("content").toString(), QString("hi there"));
    QCOMPARE(msgs.at(2).toObject().value("content").toString(), QString("42"));

    // 索引 messageCount 同样只计三种对话类型（usage 不计）
    auto metas = store.listSessions();
    QCOMPARE(metas.size(), 1);
    QCOMPARE(metas.at(0).id, sid);
    QCOMPARE(metas.at(0).messageCount, 3);
}

// ---------------------------------------------------------------------------
// 2. JSONL 容错：损坏行被跳过，不整体丢弃
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testParseTolerant()
{
    DA::DAAgentSessionStore store;
    QString sid = store.createSession();
    QVERIFY(!sid.isEmpty());

    // 直接写 JSONL，在合法记录间插入损坏行
    QFile f(sessionsDir() + QStringLiteral("/") + sid + QStringLiteral(".jsonl"));
    QVERIFY(f.open(QIODevice::Append | QIODevice::Text));
    f.write(QJsonDocument(makeRecord(sid, "user", "first")).toJson(QJsonDocument::Compact));
    f.write("\n");
    f.write("{this is not valid json\n");     // 损坏行 1
    f.write("totally not json at all\n");      // 损坏行 2
    f.write(QJsonDocument(makeRecord(sid, "assistant", "second")).toJson(QJsonDocument::Compact));
    f.write("\n");
    f.write("\n");                              // 空行
    f.close();

    QJsonArray msgs = store.readMessagesForLoad(sid);
    // 损坏/空行被跳过，仅两条合法记录
    QCOMPARE(msgs.size(), 2);
    QCOMPARE(msgs.at(0).toObject().value("content").toString(), QString("first"));
    QCOMPARE(msgs.at(1).toObject().value("content").toString(), QString("second"));
}

// ---------------------------------------------------------------------------
// 3. 索引原子写：listSessions 倒序，renameSession 更新 index
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testIndexAtomicWrite()
{
    DA::DAAgentSessionStore store;
    // msleep 间隔确保 updatedAt（毫秒精度）严格递增，倒序确定
    QString s1 = store.createSession();
    QThread::msleep(5);
    QString s2 = store.createSession();
    QThread::msleep(5);
    QString s3 = store.createSession();

    auto metas = store.listSessions();
    QCOMPARE(metas.size(), 3);
    // 按 updatedAt 倒序：最新（s3）在前
    QCOMPARE(metas.at(0).id, s3);
    QCOMPARE(metas.at(1).id, s2);
    QCOMPARE(metas.at(2).id, s1);

    // renameSession 刷新 updatedAt，使 s1 成为最新；title 同步更新
    QThread::msleep(5);
    store.renameSession(s1, QStringLiteral("Renamed Session"));
    auto metas2 = store.listSessions();
    QCOMPARE(metas2.size(), 3);
    QCOMPARE(metas2.at(0).id, s1);
    QCOMPARE(metas2.at(0).title, QString("Renamed Session"));

    // 原子写：索引文件应存在且为合法 JSON 数组
    QFile idx(sessionsDir() + QStringLiteral("/sessions_index.json"));
    QVERIFY(idx.exists());
    QVERIFY(idx.open(QIODevice::ReadOnly));
    QVERIFY(QJsonDocument::fromJson(idx.readAll()).isArray());
}

// ---------------------------------------------------------------------------
// 4. 清理：数量上限删最旧；时间上限删超期；skipSessionId 保护当前活跃
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testCleanup()
{
    DA::DAAgentSessionStore store;

    // 4a. 数量限制：建 6 个，保留 3 个 → 删最旧 3 个
    QString ids[6];
    for (int i = 0; i < 6; ++i) {
        ids[i] = store.createSession();
        if (i < 5) QThread::msleep(5);  // 保证 updatedAt 严格递增
    }
    store.cleanupOldSessions(3, 0, QString());  // maxCount=3, retentionDays=0（不按时间）
    QVERIFY(store.hasSession(ids[3]));
    QVERIFY(store.hasSession(ids[4]));
    QVERIFY(store.hasSession(ids[5]));
    QVERIFY(!store.hasSession(ids[0]));
    QVERIFY(!store.hasSession(ids[1]));
    QVERIFY(!store.hasSession(ids[2]));

    // 4b. 时间限制：backdate 一个会话的 updatedAt 到 40 天前，retentionDays=30 应删
    QString s = store.createSession();
    backdateUpdatedAt(s, 40);
    store.cleanupOldSessions(100, 30, QString());  // maxCount 大到不触发，仅按时间
    QVERIFY(!store.hasSession(s));

    // 4c. skipSessionId 保护当前活跃会话不被删（即使超 maxCount）
    QString s2 = store.createSession();
    // maxCount=1 会试图只留 1 个，但 s2 被跳过保护
    store.cleanupOldSessions(1, 0, s2);
    QVERIFY(store.hasSession(s2));

    // 4d. qMax 边界防护：maxCount=0 / 负数不应清空全部（plan-06 边界）
    //     qMax(1, 0)=1 → 至少保留 1 个非活跃会话，不会误删到 0
    int before = store.listSessions().size();
    QVERIFY(before >= 2);  // 至少 s2 + 一个旧会话
    store.cleanupOldSessions(0, -5, QString());
    int after = store.listSessions().size();
    // 不会全部删除：至少保留 1 个非活跃会话 + s2（若有）
    QVERIFY(after >= 1);
}

// ---------------------------------------------------------------------------
// 5. last_active 指针：往返 + projectPath 过滤（plan-05 MAJOR-4 回归保护）
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testLastActive()
{
    DA::DAAgentSessionStore store;

    // 创建两个会话：自由会话 sidA、工程绑定会话 sidB
    QString sidA = store.createSession();                      // projectPath 空（自由）
    QString sidB = store.createSession(QStringLiteral("C:/proj"));  // projectPath 非空

    // 5a. 自由会话往返：setLastActive(sidA, "") → 空 filter 返回 sidA
    store.setLastActive(sidA, QString());
    QCOMPARE(store.lastActiveSession(QString()), sidA);

    // 5b. MAJOR-4 回归：指针指向工程绑定会话时，空 filter 必须返回空
    //     （避免启动恢复把工程绑定会话当自由会话恢复）
    store.setLastActive(sidB, QStringLiteral("C:/proj"));
    QCOMPARE(store.lastActiveSession(QString()), QString());

    // 5c. 非空 filter 精确匹配 → 返回 sidB
    QCOMPARE(store.lastActiveSession(QStringLiteral("C:/proj")), sidB);

    // 5d. 非空 filter 不匹配 → 返回空
    QCOMPARE(store.lastActiveSession(QStringLiteral("D:/other")), QString());

    // 5e. 切回自由会话指针，空 filter 再次生效
    store.setLastActive(sidA, QString());
    QCOMPARE(store.lastActiveSession(QString()), sidA);
}

// ---------------------------------------------------------------------------
// 5f. Bug1 回归：导入工程会话标记 projectPath + 同 id 合并覆盖
//     （支撑 DAAgentModule::loadSessionsFromProject 的指针修正与 fallback）
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testImportSessionFiles()
{
    DA::DAAgentSessionStore store;

    // 先建一个自由会话 sidFree，把全局指针指向它（模拟打开工程前的游离会话状态）
    QString sidFree = store.createSession();  // projectPath 空
    store.setLastActive(sidFree, QString());

    // 构造工程内嵌会话 sidP 的 jsonl 字节：一条 user + 一条 assistant
    QString sidP = QStringLiteral("imported-proj-session-id");
    QByteArray jsonl;
    {
        QJsonObject u = makeRecord(sidP, "user", QStringLiteral("in project"));
        QJsonObject a = makeRecord(sidP, "assistant", QStringLiteral("reply in project"));
        jsonl += QJsonDocument(u).toJson(QJsonDocument::Compact) + "\n";
        jsonl += QJsonDocument(a).toJson(QJsonDocument::Compact) + "\n";
    }

    // 导入：标记 projectPath = C:/proj
    QHash< QString, QByteArray > files;
    files.insert(sidP, jsonl);
    store.importSessionFiles(files, QStringLiteral("C:/proj"));

    // 导入后 hasSession 命中，listSessions 全量可见
    QVERIFY(store.hasSession(sidP));
    auto all = store.listSessions();  // 不过滤
    QCOMPARE(all.size(), 2);  // sidFree + sidP

    // 导入会话的 projectPath 必须被标记为 C:/proj（Bug1 修复依赖此标记做 fallback）
    QString pPath;
    for (const auto& m : std::as_const(all)) {
        if (m.id == sidP) { pPath = m.projectPath; break; }
    }
    QCOMPARE(pPath, QStringLiteral("C:/proj"));

    // 导入会话的记录可读，readAllRecords 含两条
    auto recs = store.readAllRecords(sidP);
    QCOMPARE(recs.size(), 2);

    // 同 id 再次导入：合并覆盖（旧 jsonl 被新内容取代），不产生重复条目
    QByteArray jsonl2;
    {
        QJsonObject u = makeRecord(sidP, "user", QStringLiteral("overwritten"));
        jsonl2 += QJsonDocument(u).toJson(QJsonDocument::Compact) + "\n";
    }
    QHash< QString, QByteArray > files2;
    files2.insert(sidP, jsonl2);
    store.importSessionFiles(files2, QStringLiteral("C:/proj"));
    auto all2 = store.listSessions();
    QCOMPARE(all2.size(), 2);  // 仍是 sidFree + sidP，未新增
    auto recs2 = store.readAllRecords(sidP);
    QCOMPARE(recs2.size(), 1);  // 被覆盖为单条
    QCOMPARE(recs2.at(0).value("message").toObject().value("content").toString(),
             QStringLiteral("overwritten"));

    // 空 files 导入为 no-op：不写、不改 index（仅重写不变内容）
    QHash< QString, QByteArray > emptyFiles;
    store.importSessionFiles(emptyFiles, QStringLiteral("C:/proj"));
    QVERIFY(store.hasSession(sidP));
    QVERIFY(store.hasSession(sidFree));
}

// ---------------------------------------------------------------------------
// 5g. Bug1 回归：listSessions(projectPath) 按工程过滤 + updatedAt 倒序
//     （支撑会话下拉按当前工程过滤显示）
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testProjectPathFiltering()
{
    DA::DAAgentSessionStore store;

    // 工程绑定会话 s1/s2/s3（按时间递增创建），自由会话 sFree
    QString sFree = store.createSession();  // projectPath 空
    QThread::msleep(5);
    QString s1 = store.createSession(QStringLiteral("C:/projA"));
    QThread::msleep(5);
    QString s2 = store.createSession(QStringLiteral("C:/projA"));
    QThread::msleep(5);
    QString s3 = store.createSession(QStringLiteral("C:/projA"));
    QThread::msleep(5);
    QString sOther = store.createSession(QStringLiteral("C:/projB"));

    // 按 projA 过滤：仅 s1/s2/s3，按 updatedAt 倒序（s3 在前）
    auto projA = store.listSessions(QStringLiteral("C:/projA"));
    QCOMPARE(projA.size(), 3);
    QCOMPARE(projA.at(0).id, s3);
    QCOMPARE(projA.at(1).id, s2);
    QCOMPARE(projA.at(2).id, s1);
    for (const auto& m : std::as_const(projA)) {
        QCOMPARE(m.projectPath, QStringLiteral("C:/projA"));
    }

    // 按 projB 过滤：仅 sOther
    auto projB = store.listSessions(QStringLiteral("C:/projB"));
    QCOMPARE(projB.size(), 1);
    QCOMPARE(projB.at(0).id, sOther);

    // 自由会话不被任何非空 filter 返回
    for (const auto& m : std::as_const(projA)) QVERIFY(m.id != sFree);
    for (const auto& m : std::as_const(projB)) QVERIFY(m.id != sFree);

    // 不传 filter 返回全部（含自由会话与所有工程会话）
    auto all = store.listSessions();
    QCOMPARE(all.size(), 5);
}

// ---------------------------------------------------------------------------
// 6. ensureTitle 自动简短命名 + 幂等 + UI 刷新信号契约（ensureTitle 返回是否变更）
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testEnsureTitle()
{
    DA::DAAgentSessionStore store;
    QString sid = store.createSession();

    // 6a. 新会话无标题，且无 user 记录时 ensureTitle 返回 false
    {
        auto metas = store.listSessions();
        QCOMPARE(metas.size(), 1);
        QVERIFY(metas.at(0).title.isEmpty());
    }
    QVERIFY(!store.ensureTitle(sid));

    // 6b. 首条 user 消息（多行、首行超 20 字符）→ ensureTitle 返回 true，
    //     title 取首行前 20 字符 + 省略号，不含第二行、不含被截掉的尾部
    //     首行 24 字符："一二三四五六七八九十一二三四五六七八九十尾部标记"
    QString longMsg = QStringLiteral("一二三四五六七八九十一二三四五六七八九十尾部标记\n第二行不该进标题");
    store.appendRecord(sid, makeRecord(sid, "user", longMsg));
    QVERIFY(store.ensureTitle(sid));

    auto metas = store.listSessions();
    QCOMPARE(metas.size(), 1);
    QString title = metas.at(0).title;
    QVERIFY(title.startsWith(QStringLiteral("一二三四五六七八九十一二三四五六七八九十")));
    QVERIFY(title.endsWith(QStringLiteral("…")));
    QCOMPARE(title.size(), 21);  // 20 字符 + 省略号
    QVERIFY(!title.contains(QStringLiteral("尾部标记")));  // 超出 20 的尾部被截
    QVERIFY(!title.contains(QStringLiteral("第二行")));     // 非首行不进标题

    // 6c. 再次 ensureTitle 幂等：已有标题不动，返回 false
    QVERIFY(!store.ensureTitle(sid));

    // 6d. 短消息（< 20 字符）不截断、无省略号（按 id 查找，不依赖 listSessions 的排序）
    QString sid2 = store.createSession();
    store.appendRecord(sid2, makeRecord(sid2, "user", QStringLiteral("短消息")));
    QVERIFY(store.ensureTitle(sid2));
    QString sid2Title;
    for (const auto& m : store.listSessions()) {
        if (m.id == sid2) { sid2Title = m.title; break; }
    }
    QCOMPARE(sid2Title, QString("短消息"));
}

// ---------------------------------------------------------------------------
// 7. 按会话 ID 查消息计数：支撑 DAAgentModule::newSession 空会话复用守卫
//    （守卫靠 messageCount(id)==0 判定"当前会话仍为全新，无需再建一个"）
// ---------------------------------------------------------------------------
void DAAgentSessionStoreTest::testMessageCount()
{
    DA::DAAgentSessionStore store;
    QString sid = store.createSession();

    // 7a. 全新会话无任何消息记录 → 0（newSession 守卫据此判定可复用）
    QCOMPARE(store.messageCount(sid), 0);

    // 7b. 仅计 user/assistant/tool_result；usage 不计（与 listSessions 的 messageCount 语义一致）
    store.appendRecord(sid, makeRecord(sid, "user", "hello"));
    QCOMPARE(store.messageCount(sid), 1);
    store.appendRecord(sid, makeRecord(sid, "assistant", "hi there"));
    QCOMPARE(store.messageCount(sid), 2);
    store.appendRecord(sid, makeRecord(sid, "tool_result", "42"));
    QCOMPARE(store.messageCount(sid), 3);
    store.appendRecord(sid, makeRecord(sid, "usage", QString()));  // usage 不计入
    QCOMPARE(store.messageCount(sid), 3);

    // 7c. 与 listSessions 返回的同名字段一致（索引维护值的单一真相源）
    auto metas = store.listSessions();
    QCOMPARE(metas.size(), 1);
    QCOMPARE(metas.at(0).messageCount, store.messageCount(sid));

    // 7d. 不存在的 id → -1（守卫据此回退到 createSession，不误判为空会话）
    QCOMPARE(store.messageCount(QStringLiteral("not-a-real-session-id")), -1);

    // 7e. 空 id → -1
    QCOMPARE(store.messageCount(QString()), -1);
}

// ---------------------------------------------------------------------------
// main：在 QCoreApplication 构造前打开 test 模式（重定向 AppData 到临时目录）
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // 必须在 QCoreApplication 与任何 DADir::getAppDataPath() 调用之前：
    // 使 QStandardPaths::writableLocation(AppDataLocation) 指向临时目录而非真实 %APPDATA%
    QStandardPaths::setTestModeEnabled(true);

    QCoreApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("DA"));
    app.setApplicationName(QStringLiteral("DAWorkBench"));

    DAAgentSessionStoreTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "main.moc"
