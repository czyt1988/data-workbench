// DAAgentSubagentManagerTest/main.cpp
// 单元测试：子 agent 定义库（subagent-phase1 P1 验收，仿 DAAgentSessionStoreTest 隔离模式）
//
// 覆盖：
//  - frontmatter 往返（含 permissions 原样存留）
//  - ensureDefaultSubagents 播种（仅缺失时写入）
//  - registerBuiltin 已存在跳过
//  - 删除 / 重命名（save oldName）
//
// 隔离策略：main() 起手 QStandardPaths::setTestModeEnabled(true)（计划验收要求）。
// 注意：子 agent 定义目录为 <exe>/daAgent/subagents（基于 applicationDirPath，
// 不受 test 模式重定向），测试 exe 输出在 ${CMAKE_BINARY_DIR}/bin，故 init()/cleanup()
// 主动清空该子目录——仅清测试产物，不触碰 daAgent/*.md 提示词库文件。

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "DAAgentSubagentDef.h"
#include "DAAgentSubagentManager.h"

class DAAgentSubagentManagerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();     // 每个用例前清空 subagents 目录（用例间隔离）
    void cleanup();  // 每个用例后清空，保持构建目录整洁
    void testFrontmatterRoundTrip();
    void testMarkdownParseVariants();
    void testJsonRoundTrip();
    void testEnsureDefaultSubagents();
    void testRegisterBuiltinSkipsExisting();
    void testDelete();
    void testRenameSaveOldName();
    void testPermissionsPreservedOnFileRoundTrip();

private:
    static QString subagentsDir();
    static QString exploreFilePath();
    static void removeSubagentsDir();
};

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

QString DAAgentSubagentManagerTest::subagentsDir()
{
    return QDir(QCoreApplication::applicationDirPath() + "/daAgent/subagents").absolutePath();
}

QString DAAgentSubagentManagerTest::exploreFilePath()
{
    return QFileInfo(subagentsDir(), "explore.md").absoluteFilePath();
}

void DAAgentSubagentManagerTest::removeSubagentsDir()
{
    QDir dir(subagentsDir());
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

void DAAgentSubagentManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DAAgentSubagentManagerTest::init()
{
    removeSubagentsDir();
}

void DAAgentSubagentManagerTest::cleanup()
{
    removeSubagentsDir();
}

// ---------------------------------------------------------------------------
// frontmatter 往返（含 permissions 原样存留）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testFrontmatterRoundTrip()
{
    DA::DAAgentSubagentDef def;
    def.name         = QStringLiteral("explore");
    def.description  = QStringLiteral("Read-only data exploration - inspect datasets");
    def.tools        = QStringList{QStringLiteral("list_data"),
                                   QStringLiteral("get_data_info"),
                                   QStringLiteral("query_data")};
    def.systemPrompt = QStringLiteral("Line 1\n\n- bullet\nLine 3");
    QJsonObject perms;
    perms[QStringLiteral("mode")]  = QStringLiteral("manual");
    perms[QStringLiteral("extra")] = QJsonObject{{QStringLiteral("deny"), QJsonArray{QStringLiteral("/sys/**")}}};
    def.permissions = perms;

    const QString md = def.toMarkdown();
    QVERIFY(md.startsWith(QStringLiteral("---\n")));
    QVERIFY(md.contains(QStringLiteral("name: explore")));
    QVERIFY(md.contains(QStringLiteral("tools: [list_data, get_data_info, query_data]")));
    QVERIFY(md.contains(QStringLiteral("permissions: ")));

    const DA::DAAgentSubagentDef parsed = DA::DAAgentSubagentDef::fromMarkdown(md);
    QVERIFY(parsed.isValid());
    QCOMPARE(parsed.name, def.name);
    QCOMPARE(parsed.description, def.description);
    QCOMPARE(parsed.tools, def.tools);
    QCOMPARE(parsed.systemPrompt, def.systemPrompt);
    // permissions 原样存留（解析为 QJsonObject，内容逐字段一致）
    QCOMPARE(parsed.permissions, def.permissions);

    // 二次往返幂等
    const DA::DAAgentSubagentDef parsed2 = DA::DAAgentSubagentDef::fromMarkdown(parsed.toMarkdown());
    QCOMPARE(parsed2.name, def.name);
    QCOMPARE(parsed2.tools, def.tools);
    QCOMPARE(parsed2.permissions, def.permissions);
}

// ---------------------------------------------------------------------------
// frontmatter 解析变体（块级 tools / 无 frontmatter / permissions 非法降级）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testMarkdownParseVariants()
{
    // 块级 tools 列表（"- item" 行）
    const QString md1 = QStringLiteral("---\n"
                                       "name: explorer\n"
                                       "description: d1\n"
                                       "tools:\n"
                                       "  - list_data\n"
                                       "  - read_file\n"
                                       "---\n"
                                       "body text");
    const DA::DAAgentSubagentDef p1 = DA::DAAgentSubagentDef::fromMarkdown(md1);
    QCOMPARE(p1.name, QStringLiteral("explorer"));
    QCOMPARE(p1.description, QStringLiteral("d1"));
    QCOMPARE(p1.tools, QStringList({QStringLiteral("list_data"), QStringLiteral("read_file")}));
    QCOMPARE(p1.systemPrompt, QStringLiteral("body text"));

    // 无 frontmatter：整体作为正文，name 为空（loadFromFile 回退文件名）
    const QString md2 = QStringLiteral("plain body\ntext");
    const DA::DAAgentSubagentDef p2 = DA::DAAgentSubagentDef::fromMarkdown(md2);
    QVERIFY(!p2.isValid());
    QCOMPARE(p2.systemPrompt, md2);

    // permissions 非法 JSON：丢弃并告警，不影响其余字段
    const QString md3 = QStringLiteral("---\nname: x\npermissions: not-json\n---\nb");
    const DA::DAAgentSubagentDef p3 = DA::DAAgentSubagentDef::fromMarkdown(md3);
    QCOMPARE(p3.name, QStringLiteral("x"));
    QVERIFY(p3.permissions.isEmpty());
    QCOMPARE(p3.systemPrompt, QStringLiteral("b"));

    // description 含冒号（仅首个冒号分隔键值）
    const QString md4 = QStringLiteral("---\nname: y\ndescription: A: B\n---\n");
    const DA::DAAgentSubagentDef p4 = DA::DAAgentSubagentDef::fromMarkdown(md4);
    QCOMPARE(p4.description, QStringLiteral("A: B"));
}

// ---------------------------------------------------------------------------
// JSON 协议往返（接口层 CRUD 载荷）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testJsonRoundTrip()
{
    DA::DAAgentSubagentDef def;
    def.name         = QStringLiteral("explore");
    def.description  = QStringLiteral("desc");
    def.tools        = QStringList{QStringLiteral("list_data")};
    def.systemPrompt = QStringLiteral("prompt");
    def.permissions  = QJsonObject{{QStringLiteral("mode"), QStringLiteral("manual")}};

    // 完整 JSON（含 permissions）
    const DA::DAAgentSubagentDef parsed = DA::DAAgentSubagentDef::fromJsonObject(def.toJsonObject());
    QCOMPARE(parsed.name, def.name);
    QCOMPARE(parsed.description, def.description);
    QCOMPARE(parsed.tools, def.tools);
    QCOMPARE(parsed.systemPrompt, def.systemPrompt);
    QCOMPARE(parsed.permissions, def.permissions);

    // 协议载荷仅四字段（母文档 §7 契约），不含 permissions
    const QJsonObject wire = def.toProtocolJson();
    QCOMPARE(wire.value(QStringLiteral("name")).toString(), def.name);
    QCOMPARE(wire.value(QStringLiteral("system_prompt")).toString(), def.systemPrompt);
    QVERIFY(wire.contains(QStringLiteral("tools")));
    QVERIFY(!wire.contains(QStringLiteral("permissions")));
}

// ---------------------------------------------------------------------------
// ensureDefaultSubagents 播种（仅文件缺失时写入）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testEnsureDefaultSubagents()
{
    DA::DAAgentSubagentManager mgr;
    QVERIFY(!QFileInfo::exists(exploreFilePath()));

    mgr.ensureDefaultSubagents();
    QVERIFY(QFileInfo::exists(exploreFilePath()));

    // 播种内容可解析为内置 explore 定义
    const DA::DAAgentSubagentDef seeded = DA::DAAgentSubagentDef::loadFromFile(exploreFilePath());
    QCOMPARE(seeded.name, QStringLiteral("explore"));
    QVERIFY(!seeded.description.isEmpty());
    QVERIFY(seeded.tools.contains(QStringLiteral("list_data")));
    QVERIFY(seeded.tools.contains(QStringLiteral("list_figures")));
    QVERIFY(!seeded.systemPrompt.isEmpty());

    // 用户修改后再次播种不覆盖（尊重用户编辑）
    QFile f(exploreFilePath());
    QVERIFY(f.open(QIODevice::Append | QIODevice::Text));
    f.write("\n# user edit marker\n");
    f.close();
    mgr.ensureDefaultSubagents();
    QFile f2(exploreFilePath());
    QVERIFY(f2.open(QIODevice::ReadOnly));
    const QString after = QString::fromUtf8(f2.readAll());
    f2.close();
    QVERIFY(after.contains(QStringLiteral("user edit marker")));
}

// ---------------------------------------------------------------------------
// registerBuiltin 已存在跳过
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testRegisterBuiltinSkipsExisting()
{
    DA::DAAgentSubagentManager mgr;
    const QString contentA = QStringLiteral("---\nname: custom\ndescription: A\ntools: [list_data]\n---\nbody A");
    mgr.registerBuiltin(QStringLiteral("custom"), contentA);
    const QString path = QFileInfo(subagentsDir(), QStringLiteral("custom.md")).absoluteFilePath();
    QVERIFY(QFileInfo::exists(path));
    mgr.loadSubagents();
    QVERIFY(mgr.findSubagent(QStringLiteral("custom")) != nullptr);

    // 已存在时跳过：内容保持 A
    const QString contentB = QStringLiteral("---\nname: custom\ndescription: B\ntools: []\n---\nbody B");
    mgr.registerBuiltin(QStringLiteral("custom"), contentB);
    const DA::DAAgentSubagentDef s = DA::DAAgentSubagentDef::loadFromFile(path);
    QCOMPARE(s.description, QStringLiteral("A"));
    QCOMPARE(s.systemPrompt, QStringLiteral("body A"));
}

// ---------------------------------------------------------------------------
// 删除
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testDelete()
{
    DA::DAAgentSubagentManager mgr;
    DA::DAAgentSubagentDef def;
    def.name         = QStringLiteral("tmp_agent");
    def.description  = QStringLiteral("d");
    def.tools        = QStringList{QStringLiteral("list_data")};
    def.systemPrompt = QStringLiteral("b");
    QVERIFY(mgr.saveSubagent(def));
    QVERIFY(mgr.findSubagent(QStringLiteral("tmp_agent")) != nullptr);
    QVERIFY(QFileInfo::exists(QFileInfo(subagentsDir(), QStringLiteral("tmp_agent.md")).absoluteFilePath()));

    QVERIFY(mgr.deleteSubagent(QStringLiteral("tmp_agent")));
    QVERIFY(mgr.findSubagent(QStringLiteral("tmp_agent")) == nullptr);
    QVERIFY(!QFileInfo::exists(QFileInfo(subagentsDir(), QStringLiteral("tmp_agent.md")).absoluteFilePath()));
    // 重复删除返回 false
    QVERIFY(!mgr.deleteSubagent(QStringLiteral("tmp_agent")));
}

// ---------------------------------------------------------------------------
// 重命名（save oldName）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testRenameSaveOldName()
{
    DA::DAAgentSubagentManager mgr;
    DA::DAAgentSubagentDef def;
    def.name         = QStringLiteral("old_name");
    def.description  = QStringLiteral("before");
    def.tools        = QStringList{QStringLiteral("list_data")};
    def.systemPrompt = QStringLiteral("body");
    QVERIFY(mgr.saveSubagent(def));
    const QString oldPath = QFileInfo(subagentsDir(), QStringLiteral("old_name.md")).absoluteFilePath();
    const QString newPath = QFileInfo(subagentsDir(), QStringLiteral("new_name.md")).absoluteFilePath();
    QVERIFY(QFileInfo::exists(oldPath));

    // 重命名：新名保存 + 旧文件删除
    DA::DAAgentSubagentDef renamed = def;
    renamed.name        = QStringLiteral("new_name");
    renamed.description = QStringLiteral("after");
    QVERIFY(mgr.saveSubagent(renamed, QStringLiteral("old_name")));

    QVERIFY(!QFileInfo::exists(oldPath));
    QVERIFY(QFileInfo::exists(newPath));
    QVERIFY(mgr.findSubagent(QStringLiteral("old_name")) == nullptr);
    const DA::DAAgentSubagentDef* found = mgr.findSubagent(QStringLiteral("new_name"));
    QVERIFY(found != nullptr);
    QCOMPARE(found->description, QStringLiteral("after"));
    QCOMPARE(found->tools, def.tools);
}

// ---------------------------------------------------------------------------
// permissions 文件往返原样存留（解析存留、不解释不生效）
// ---------------------------------------------------------------------------

void DAAgentSubagentManagerTest::testPermissionsPreservedOnFileRoundTrip()
{
    DA::DAAgentSubagentManager mgr;
    DA::DAAgentSubagentDef def;
    def.name         = QStringLiteral("perm_agent");
    def.description  = QStringLiteral("d");
    def.tools        = QStringList{QStringLiteral("read_file")};
    def.systemPrompt = QStringLiteral("body");
    QJsonObject perms;
    perms[QStringLiteral("mode")]  = QStringLiteral("manual");
    perms[QStringLiteral("extra")] = QJsonObject{{QStringLiteral("k"), QStringLiteral("v")}};
    def.permissions = perms;

    QVERIFY(mgr.saveSubagent(def));
    mgr.loadSubagents();
    const DA::DAAgentSubagentDef* found = mgr.findSubagent(QStringLiteral("perm_agent"));
    QVERIFY(found != nullptr);
    QCOMPARE(found->permissions, perms);  // 原样存留
    QCOMPARE(found->systemPrompt, QStringLiteral("body"));
}

QTEST_GUILESS_MAIN(DAAgentSubagentManagerTest)
#include "main.moc"
