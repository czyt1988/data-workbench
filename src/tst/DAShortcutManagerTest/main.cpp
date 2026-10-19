#include "DAShortcutManager.h"
#include <QtTest/QtTest>
#include <QApplication>
#include <QAction>
#include <QTemporaryDir>
#include <QFile>

/**
 * @brief DAShortcutManager 单元测试
 *
 * 覆盖：命令注册与默认值收集、用户覆盖与显式清除、重置语义、冲突检测、shortcuts.json 往返
 */
class DAShortcutManagerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAttachCollectsDefault();
    void testOverrideAndClear();
    void testResetSemantics();
    void testConflictDetection();
    void testJsonRoundTrip();

private:
    QObject mActionHost;  ///< action 宿主（随测试对象销毁）
};

void DAShortcutManagerTest::testAttachCollectsDefault()
{
    DA::DAShortcutManager mgr;
    QAction a1(&mActionHost);
    a1.setObjectName("actionTestOpen");
    a1.setShortcut(QKeySequence("Ctrl+O"));
    QAction a2(&mActionHost);
    a2.setObjectName("actionTestMulti");
    a2.setShortcuts({ QKeySequence("Ctrl+="), QKeySequence("Ctrl+Shift+Z") });
    QAction aNoName(&mActionHost);

    // 合法注册
    QVERIFY(mgr.attachAction(&a1, "Main"));
    QVERIFY(mgr.attachAction(&a2, "Main"));
    QCOMPARE(mgr.defaultKeysOf("actionTestOpen"), QKeySequence("Ctrl+O"));
    QCOMPARE(mgr.effectiveKeysOf("actionTestOpen"), QKeySequence("Ctrl+O"));
    QCOMPARE(mgr.categoryOf("actionTestOpen"), QString("Main"));
    QVERIFY(mgr.contains("actionTestOpen"));
    QVERIFY(mgr.commandNames().contains("actionTestMulti"));
    // 显示名：去掉助记符与换行
    a1.setText("Open\nProject");
    QCOMPARE(mgr.displayNameOf("actionTestOpen"), QString("Open Project"));
    // 未注册命令
    QVERIFY(!mgr.contains("actionUnknown"));
    QVERIFY(mgr.defaultKeysOf("actionUnknown").isEmpty());

    // 非法注册：空指针与无 objectName
    QVERIFY(!mgr.attachAction(nullptr));
    QVERIFY(!mgr.attachAction(&aNoName));

    // applyAll 后多绑定 action 恢复为两个单绑定
    mgr.applyAll();
    QCOMPARE(a1.shortcuts().size(), 1);
    QCOMPARE(a2.shortcuts().size(), 2);
}

void DAShortcutManagerTest::testOverrideAndClear()
{
    DA::DAShortcutManager mgr;
    QAction a1(&mActionHost);
    a1.setObjectName("actionTestOpen");
    a1.setShortcut(QKeySequence("Ctrl+O"));
    mgr.attachAction(&a1, "Main");

    // 覆盖
    mgr.setOverride("actionTestOpen", QKeySequence("Ctrl+P"));
    QVERIFY(mgr.hasOverride("actionTestOpen"));
    QCOMPARE(mgr.effectiveKeysOf("actionTestOpen"), QKeySequence("Ctrl+P"));
    mgr.applyAll();
    QCOMPARE(a1.shortcuts().size(), 1);
    QCOMPARE(a1.shortcuts().first(), QKeySequence("Ctrl+P"));

    // 显式清除（空序列也是一种覆盖）
    mgr.setOverride("actionTestOpen", QKeySequence());
    QVERIFY(mgr.hasOverride("actionTestOpen"));
    QVERIFY(mgr.effectiveKeysOf("actionTestOpen").isEmpty());
    mgr.applyAll();
    QVERIFY(a1.shortcuts().isEmpty());

    // 默认值不受影响
    QCOMPARE(mgr.defaultKeysOf("actionTestOpen"), QKeySequence("Ctrl+O"));
}

void DAShortcutManagerTest::testResetSemantics()
{
    DA::DAShortcutManager mgr;
    QAction a1(&mActionHost);
    a1.setObjectName("actionTestOpen");
    a1.setShortcut(QKeySequence("Ctrl+O"));
    QAction a2(&mActionHost);
    a2.setObjectName("actionTestSave");
    a2.setShortcut(QKeySequence("Ctrl+S"));
    mgr.attachAction(&a2, "Main");
    mgr.attachAction(&a1, "Main");

    // 单个重置：无覆盖时返回 false
    QVERIFY(!mgr.resetToDefault("actionTestOpen"));
    mgr.setOverride("actionTestOpen", QKeySequence("Ctrl+P"));
    QVERIFY(mgr.resetToDefault("actionTestOpen"));
    QVERIFY(!mgr.hasOverride("actionTestOpen"));
    QCOMPARE(mgr.effectiveKeysOf("actionTestOpen"), QKeySequence("Ctrl+O"));

    // 整表替换与全量重置
    QHash< QString, QKeySequence > snapshot;
    snapshot.insert("actionTestOpen", QKeySequence("F9"));
    snapshot.insert("actionTestSave", QKeySequence());  // 显式清除
    mgr.setOverrides(snapshot);
    QCOMPARE(mgr.overrides().size(), 2);
    QCOMPARE(mgr.effectiveKeysOf("actionTestOpen"), QKeySequence("F9"));
    QVERIFY(mgr.effectiveKeysOf("actionTestSave").isEmpty());
    mgr.resetAll();
    QVERIFY(mgr.overrides().isEmpty());
    QCOMPARE(mgr.effectiveKeysOf("actionTestSave"), QKeySequence("Ctrl+S"));
}

void DAShortcutManagerTest::testConflictDetection()
{
    DA::DAShortcutManager mgr;
    QAction aOpen(&mActionHost);
    aOpen.setObjectName("actionTestOpen");
    aOpen.setShortcut(QKeySequence("Ctrl+O"));
    QAction aSave(&mActionHost);
    aSave.setObjectName("actionTestSave");
    QAction aZoom(&mActionHost);
    aZoom.setObjectName("actionTestZoom");
    // 多绑定：标准 ZoomIn（PortableText 为 "Ctrl++"）与 "Ctrl+="，与工作流编辑器 ZoomIn 默认值一致
    aZoom.setShortcuts({ QKeySequence(QKeySequence::ZoomIn), QKeySequence("Ctrl+=") });
    mgr.attachActions({ &aOpen, &aSave, &aZoom }, "Main");

    // 多绑定默认值的 PortableText 往返无损（splitSequence/joinSequences 正确性）
    QCOMPARE(mgr.defaultKeysOf("actionTestZoom").toString(QKeySequence::PortableText),
             QString("Ctrl++, Ctrl+="));

    // 与默认值冲突
    QCOMPARE(mgr.conflictingCommands("actionTestSave", QKeySequence("Ctrl+O")),
             QStringList { QString("actionTestOpen") });
    // 与覆盖值冲突
    mgr.setOverride("actionTestSave", QKeySequence("Ctrl+P"));
    QCOMPARE(mgr.conflictingCommands("actionTestOpen", QKeySequence("Ctrl+P")),
             QStringList { QString("actionTestSave") });
    // 排除自身
    QVERIFY(mgr.conflictingCommands("actionTestOpen", QKeySequence("Ctrl+O")).isEmpty());
    // 多绑定交集中一者命中即冲突（ZoomIn 的 "Ctrl++" 绑定）
    QCOMPARE(mgr.conflictingCommands("actionTestSave", QKeySequence(QKeySequence::ZoomIn)),
             QStringList { QString("actionTestZoom") });
    // 命中多绑定中的另一个绑定 "Ctrl+="
    QCOMPARE(mgr.conflictingCommands("actionTestSave", QKeySequence("Ctrl+=")),
             QStringList { QString("actionTestZoom") });
    // 无冲突与空序列
    QVERIFY(mgr.conflictingCommands("actionTestSave", QKeySequence("F5")).isEmpty());
    QVERIFY(mgr.conflictingCommands("actionTestSave", QKeySequence()).isEmpty());
}

void DAShortcutManagerTest::testJsonRoundTrip()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString filePath = tmpDir.path() + "/shortcuts.json";

    DA::DAShortcutManager mgr;
    mgr.setShortcutConfigFilePath(filePath);
    // 文件不存在视为无覆盖
    QVERIFY(!mgr.load());

    mgr.setOverride("actionTestOpen", QKeySequence("Ctrl+P"));
    mgr.setOverride("actionTestMulti",
                    QKeySequence::fromString("Ctrl+=, Ctrl+Shift+Z", QKeySequence::PortableText));
    mgr.setOverride("actionTestCleared", QKeySequence());  // 显式清除
    QVERIFY(mgr.save());
    QVERIFY(QFile::exists(filePath));

    // 新管理器加载同一文件
    DA::DAShortcutManager mgr2;
    mgr2.setShortcutConfigFilePath(filePath);
    QVERIFY(mgr2.load());
    QCOMPARE(mgr2.overrideKeysOf("actionTestOpen"), QKeySequence("Ctrl+P"));
    QCOMPARE(mgr2.overrideKeysOf("actionTestMulti").toString(QKeySequence::PortableText),
             QString("Ctrl+=, Ctrl+Shift+Z"));
    QVERIFY(mgr2.hasOverride("actionTestCleared"));
    QVERIFY(mgr2.overrideKeysOf("actionTestCleared").isEmpty());
    QCOMPARE(mgr2.overrides().size(), 3);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    DAShortcutManagerTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "main.moc"
