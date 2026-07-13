#include "DAAppProjectActionPolicy.h"
#include "DAAppWindowStateSerializer.h"
#include <QtTest/QtTest>

class APPRefactorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCloseAction_cleanProject();
    void testCloseAction_dirtySave();
    void testCloseAction_dirtyDiscard();
    void testCloseAction_dirtyCancel();
    void testOpenPreparation_cleanEmpty();
    void testOpenPreparation_cleanExisting();
    void testOpenPreparation_dirtyWithContentSave();
    void testOpenPreparation_dirtyWithContentDiscard();
    void testOpenPreparation_dirtyWithContentCancel();
    void testOpenPreparation_dirtyNoContentSave();
    void testOpenPreparation_dirtyNoContentDiscard();
    void testOpenPreparation_dirtyNoContentCancel();
    void testWindowStateSerialization_roundTrip();
    void testWindowStateSerialization_withoutDocking();
    void testWindowStateSerialization_brokenPayload();
};

void APPRefactorTest::testCloseAction_cleanProject()
{
    using namespace DA;
    // 干净工程无需提示，Cancel 选择应被忽略
    QCOMPARE(resolveAppCloseAction(false, DAAppSavePromptChoice::Cancel),
             DAAppCloseAction::CloseDirectly);
}

void APPRefactorTest::testCloseAction_dirtySave()
{
    using namespace DA;
    QCOMPARE(resolveAppCloseAction(true, DAAppSavePromptChoice::Save),
             DAAppCloseAction::SaveThenClose);
}

void APPRefactorTest::testCloseAction_dirtyDiscard()
{
    using namespace DA;
    QCOMPARE(resolveAppCloseAction(true, DAAppSavePromptChoice::Discard),
             DAAppCloseAction::CloseDirectly);
}

void APPRefactorTest::testCloseAction_dirtyCancel()
{
    using namespace DA;
    QCOMPARE(resolveAppCloseAction(true, DAAppSavePromptChoice::Cancel),
             DAAppCloseAction::CancelClosing);
}

void APPRefactorTest::testOpenPreparation_cleanEmpty()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(false, false, DAAppSavePromptChoice::Discard),
             DAAppOpenPreparation::OpenDirectly);
}

void APPRefactorTest::testOpenPreparation_cleanExisting()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(true, false, DAAppSavePromptChoice::Discard),
             DAAppOpenPreparation::ConfirmReplaceThenOpen);
}

void APPRefactorTest::testOpenPreparation_dirtyWithContentSave()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Save),
             DAAppOpenPreparation::SaveThenOpen);
}

void APPRefactorTest::testOpenPreparation_dirtyWithContentDiscard()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Discard),
             DAAppOpenPreparation::OpenDirectly);
}

void APPRefactorTest::testOpenPreparation_dirtyWithContentCancel()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Cancel),
             DAAppOpenPreparation::CancelOpening);
}

// dirty 但无工程内容 —— isDirty 优先判断，结果与 dirty+有工程内容一致（边界用例 #18）
void APPRefactorTest::testOpenPreparation_dirtyNoContentSave()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(false, true, DAAppSavePromptChoice::Save),
             DAAppOpenPreparation::SaveThenOpen);
}

void APPRefactorTest::testOpenPreparation_dirtyNoContentDiscard()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(false, true, DAAppSavePromptChoice::Discard),
             DAAppOpenPreparation::OpenDirectly);
}

void APPRefactorTest::testOpenPreparation_dirtyNoContentCancel()
{
    using namespace DA;
    QCOMPARE(resolveAppOpenPreparation(false, true, DAAppSavePromptChoice::Cancel),
             DAAppOpenPreparation::CancelOpening);
}

void APPRefactorTest::testWindowStateSerialization_roundTrip()
{
    using namespace DA;
    DAAppWindowStateSnapshot state;
    state.geometry        = QByteArrayLiteral("geometry");
    state.mainWindowState = QByteArrayLiteral("state");
    state.dockingState    = QByteArrayLiteral("docking");

    const QByteArray bytes = serializeAppWindowState(state);
    QVERIFY(!bytes.isEmpty());

    DAAppWindowStateSnapshot decoded;
    QVERIFY(deserializeAppWindowState(bytes, &decoded));
    QCOMPARE(decoded.geometry, state.geometry);
    QCOMPARE(decoded.mainWindowState, state.mainWindowState);
    QCOMPARE(decoded.dockingState, state.dockingState);
}

void APPRefactorTest::testWindowStateSerialization_withoutDocking()
{
    using namespace DA;
    DAAppWindowStateSnapshot state;
    state.geometry        = QByteArrayLiteral("geometry");
    state.mainWindowState = QByteArrayLiteral("state");
    // dockingState 留空

    const QByteArray bytes = serializeAppWindowState(state);
    QVERIFY(!bytes.isEmpty());

    DAAppWindowStateSnapshot decoded;
    QVERIFY(deserializeAppWindowState(bytes, &decoded));
    QCOMPARE(decoded.geometry, state.geometry);
    QCOMPARE(decoded.mainWindowState, state.mainWindowState);
    QVERIFY(decoded.dockingState.isEmpty());
}

void APPRefactorTest::testWindowStateSerialization_brokenPayload()
{
    using namespace DA;
    DAAppWindowStateSnapshot decoded;
    // 损坏的 payload 可能导致反序列化抛出异常或返回 false，两者都表示反序列化失败
    bool result = false;
    try {
        result = deserializeAppWindowState(QByteArrayLiteral("broken-payload"), &decoded);
    } catch (...) {
        result = false;  // 异常也视为反序列化失败
    }
    QVERIFY(!result);
}

QTEST_APPLESS_MAIN(APPRefactorTest)
#include "main.moc"
