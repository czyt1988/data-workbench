#include "DAAppProjectActionPolicy.h"
#include "DAAppWindowStateSerializer.h"
#include "DASettingContextProvider.h"
#include "DASettingContextProviderRegistry.h"
#include <QApplication>
#include <QUndoGroup>
#include <QUndoStack>
#include <QWidget>
#include <QtTest/QtTest>

namespace
{
/**
 * @brief 测试用的设置上下文提供者：记录激活/失活次数，可挂一个撤销栈
 */
class FakeSettingContextProvider : public DA::DASettingContextProvider
{
public:
    int activateCount { 0 };
    int deactivateCount { 0 };
    QUndoStack* undoStack { nullptr };

    void activateSettingContext() override
    {
        ++activateCount;
    }
    void deactivateSettingContext() override
    {
        ++deactivateCount;
    }
    QUndoStack* contextUndoStack() const override
    {
        return undoStack;
    }
};
}  // namespace

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
    // T6：设置上下文提供者注册表 + APP 路由
    void testSettingContextProvider_registerAndActivate();
    void testSettingContextProvider_contextUndoStackActivated();
    void testSettingContextProvider_switchDeactivatesPrevious();
    void testSettingContextProvider_repeatedActivateKeepsContext();
    void testSettingContextProvider_unknownWidgetFallsBack();
    void testSettingContextProvider_unregisterDeactivates();
    void testSettingContextProvider_destroyedContentCleanedUp();
    void testSettingContextProvider_replaceProviderForSameContent();
    void testSettingContextProvider_nullArguments();
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

// ===============================================================
// T6：设置上下文提供者注册表（APP 路由 activateContextCategoryForWidget 的核心）
// ===============================================================

// 注册后激活 dock 内容：命中返回 true（APP 路由据此在 hide-all 之前 return），回调被调用
void APPRefactorTest::testSettingContextProvider_registerAndActivate()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;

    registry.registerProvider(&content, &provider);
    QCOMPARE(registry.providerCount(), 1);
    QVERIFY(registry.isRegistered(&content));
    QCOMPARE(registry.providerFor(&content), &provider);

    // 命中：APP 在 activateContextCategoryForWidget 里直接 return，不执行固定窗路由与 hide-all
    QVERIFY(registry.activateFor(&content));
    QCOMPARE(provider.activateCount, 1);
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);
    QCOMPARE(registry.currentDockContent(), &content);
}

// 提供者的 contextUndoStack 被 setActive（用 QUndoGroup 模拟真实场景：
// 独立 QUndoStack 永远 active，只有属于 QUndoGroup 的栈才有 active/inactive 之分）
void APPRefactorTest::testSettingContextProvider_contextUndoStackActivated()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    QUndoGroup group;
    QUndoStack figureStack;
    QUndoStack otherStack;
    group.addStack(&figureStack);
    group.addStack(&otherStack);
    // 另一个 figure 的栈为当前活动栈 → figureStack 处于非激活状态
    group.setActiveStack(&otherStack);
    QVERIFY(!figureStack.isActive());
    provider.undoStack = &figureStack;

    registry.registerProvider(&content, &provider);
    QVERIFY(registry.activateFor(&content));
    QVERIFY(figureStack.isActive());
}

// 从提供者 A 切到提供者 B：先失活 A
void APPRefactorTest::testSettingContextProvider_switchDeactivatesPrevious()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget contentA;
    QWidget contentB;
    FakeSettingContextProvider providerA;
    FakeSettingContextProvider providerB;
    registry.registerProvider(&contentA, &providerA);
    registry.registerProvider(&contentB, &providerB);

    QVERIFY(registry.activateFor(&contentA));
    QVERIFY(registry.activateFor(&contentB));
    QCOMPARE(providerA.activateCount, 1);
    QCOMPARE(providerA.deactivateCount, 1);
    QCOMPARE(providerB.activateCount, 1);
    QCOMPARE(providerB.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &providerB);
}

// 同一 dock 内容内部焦点变化会重复激活：不重复失活（activate 实现需幂等）
void APPRefactorTest::testSettingContextProvider_repeatedActivateKeepsContext()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);

    QVERIFY(registry.activateFor(&content));
    QVERIFY(registry.activateFor(&content));
    QCOMPARE(provider.activateCount, 2);
    QCOMPARE(provider.deactivateCount, 0);
}

// 未注册 dock 内容：返回 false 且不改变当前状态（APP 只在焦点回到固定操作窗口/
// 丢失时才失活，避免程序化 raise 设置面板等工具 dock 时误清插件上下文）
void APPRefactorTest::testSettingContextProvider_unknownWidgetFallsBack()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    QWidget unknownContent;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);
    QVERIFY(registry.activateFor(&content));
    QCOMPARE(registry.currentProvider(), &provider);

    QVERIFY(!registry.activateFor(&unknownContent));
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);

    // 显式失活（由 APP 在固定操作窗口/焦点丢失分支调用）
    registry.deactivateCurrent();
    QCOMPARE(provider.deactivateCount, 1);
    QCOMPARE(registry.currentProvider(), nullptr);
    QCOMPARE(registry.currentDockContent(), nullptr);
    // 重复失活无副作用
    registry.deactivateCurrent();
    QCOMPARE(provider.deactivateCount, 1);
}

// 注销当前提供者：先失活再移除，之后不再命中
void APPRefactorTest::testSettingContextProvider_unregisterDeactivates()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);
    QVERIFY(registry.activateFor(&content));

    registry.unregisterProvider(&content);
    QCOMPARE(provider.deactivateCount, 1);
    QCOMPARE(registry.providerCount(), 0);
    QVERIFY(!registry.isRegistered(&content));
    QVERIFY(!registry.activateFor(&content));
}

// dock 内容先被销毁：惰性清理映射，不回调已失效的提供者
void APPRefactorTest::testSettingContextProvider_destroyedContentCleanedUp()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    FakeSettingContextProvider provider;
    QWidget* content = new QWidget();
    registry.registerProvider(content, &provider);
    QVERIFY(registry.activateFor(content));
    QCOMPARE(registry.currentProvider(), &provider);

    delete content;
    // 不 deref 已销毁指针，仅按地址清理映射；dock 内容已销毁时不回调 deactivate
    QCOMPARE(registry.providerCount(), 0);
    QVERIFY(!registry.isRegistered(content));
    QCOMPARE(registry.currentProvider(), nullptr);
    QCOMPARE(provider.deactivateCount, 0);
}

// 同一 dock 内容换提供者：旧的当前提供者先失活
void APPRefactorTest::testSettingContextProvider_replaceProviderForSameContent()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider providerA;
    FakeSettingContextProvider providerB;
    registry.registerProvider(&content, &providerA);
    QVERIFY(registry.activateFor(&content));

    registry.registerProvider(&content, &providerB);
    QCOMPARE(providerA.deactivateCount, 1);
    QCOMPARE(registry.providerCount(), 1);
    QCOMPARE(registry.providerFor(&content), &providerB);
    QVERIFY(registry.activateFor(&content));
    QCOMPARE(providerB.activateCount, 1);
}

// 空参数：注册/注销 null 不崩溃、不影响已有条目
void APPRefactorTest::testSettingContextProvider_nullArguments()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;

    registry.registerProvider(nullptr, &provider);
    registry.registerProvider(&content, nullptr);
    registry.registerProvider(nullptr, nullptr);
    QCOMPARE(registry.providerCount(), 0);
    QVERIFY(!registry.activateFor(nullptr));
    QVERIFY(!registry.providerFor(nullptr));
    registry.unregisterProvider(nullptr);
    QCOMPARE(registry.providerCount(), 0);

    registry.registerProvider(&content, &provider);
    registry.unregisterProvider(nullptr);
    QCOMPARE(registry.providerCount(), 1);
}

QTEST_MAIN(APPRefactorTest)
#include "main.moc"
