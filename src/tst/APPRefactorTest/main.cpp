#include "DAAppProjectActionPolicy.h"
#include "DAAppWindowStateSerializer.h"
#include "Controller/DAAppContextRoutePolicy.h"
#include "Controller/DAChartContextActionPolicy.h"
#include "DASettingContextProvider.h"
#include "DASettingContextProviderRegistry.h"
#include <QApplication>
#include <QUndoGroup>
#include <QUndoStack>
#include <QWidget>
#include <QtTest/QtTest>
#include <functional>

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
    // 激活回调内追加动作（用于验证重入保护/程序化 raise）
    std::function< void() > onActivate;
    // 失活回调内追加动作（用于验证重入保护/回调内注销）
    std::function< void() > onDeactivate;

    void activateSettingContext() override
    {
        ++activateCount;
        if (onActivate) {
            onActivate();
        }
    }
    void deactivateSettingContext() override
    {
        ++deactivateCount;
        if (onDeactivate) {
            onDeactivate();
        }
    }
    QUndoStack* contextUndoStack() const override
    {
        return undoStack;
    }
    DA::DAFigureWidget* chartContextFigure() const override
    {
        return chartFigure;
    }

    // 绘图上下文目标（默认 nullptr；本测试不构造真实 figure）
    DA::DAFigureWidget* chartFigure { nullptr };
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
    void testSettingContextProvider_chartContextFigureDefault();
    void testSettingContextProvider_contextUndoStackActivated();
    void testSettingContextProvider_switchDeactivatesPrevious();
    void testSettingContextProvider_repeatedActivateKeepsContext();
    void testSettingContextProvider_unknownWidgetFallsBack();
    void testSettingContextProvider_unregisterDeactivates();
    void testSettingContextProvider_destroyedContentCleanedUp();
    void testSettingContextProvider_replaceProviderForSameContent();
    void testSettingContextProvider_nullArguments();
    void testSettingContextProvider_sameProviderMultipleContents();
    void testSettingContextProvider_reentrantActivateGuarded();
    void testSettingContextProvider_nestedDeactivateDuringActivateRollsBack();
    void testSettingContextProvider_reentrantActivateDuringDeactivateIgnored();
    void testSettingContextProvider_unregisterInsideDeactivateSafe();
    // T6：APP 上下文路由判定（provider 优先 / 固定窗口 / 工具 dock）
    void testContextRoute_providerTakesPrecedence();
    void testContextRoute_fixedOperateWindows();
    void testContextRoute_utilityDockKeepsProvider();
    void testContextRoute_nullWidget();
    // 绘图动作可用性策略（DAChartContextActionPolicy，纯函数）
    void testChartActionPolicy_2DFreeLayout();
    void testChartActionPolicy_3DDisables2DInteraction();
    void testChartActionPolicy_gridDisablesGeometryEditing();
    void testChartActionPolicy_providerDisablesStructure();
    void testChartActionPolicy_provider3DDisablesSelectors();
    void testChartActionPolicy_noSubChartDisablesChartActions();
    void testChartActionPolicy_providerWithoutSubChart();
    void testChartActionPolicy_stateOfInvalidGroup();
    void testChartActionPolicy_matrixInvariants();
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

// 提供者默认不接管绘图上下文（chartContextFigure 返回 nullptr），注册/激活行为保持不变
void APPRefactorTest::testSettingContextProvider_chartContextFigureDefault()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);

    QVERIFY(registry.activateFor(&content));
    QCOMPARE(provider.chartContextFigure(), nullptr);
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

// 同一 provider 注册给多个 dock 内容：注销/替换非当前内容不得误失活当前内容
void APPRefactorTest::testSettingContextProvider_sameProviderMultipleContents()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget contentA;
    QWidget contentB;
    FakeSettingContextProvider provider;
    registry.registerProvider(&contentA, &provider);
    registry.registerProvider(&contentB, &provider);
    QCOMPARE(registry.providerCount(), 2);
    QVERIFY(registry.activateFor(&contentA));
    QCOMPARE(registry.currentDockContent(), &contentA);

    // 非当前内容注销：当前上下文保持不变、不失活
    registry.unregisterProvider(&contentB);
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);
    QCOMPARE(registry.currentDockContent(), &contentA);

    // 非当前内容销毁（QPointer 惰性清理）：同样不影响当前内容
    QWidget* contentC = new QWidget();
    registry.registerProvider(contentC, &provider);
    delete contentC;
    QCOMPARE(registry.providerCount(), 1);
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentDockContent(), &contentA);

    // 当前内容注销：失活一次
    registry.unregisterProvider(&contentA);
    QCOMPARE(provider.deactivateCount, 1);
    QCOMPARE(registry.currentProvider(), nullptr);
}

// 激活回调内重入（provider 程序化 raise 自己/已注册 dock）：不重复激活、不递归
void APPRefactorTest::testSettingContextProvider_reentrantActivateGuarded()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);
    int nestedResult = -1;
    provider.onActivate = [&registry, &content, &nestedResult]() {
        nestedResult = registry.activateFor(&content) ? 1 : 0;
    };

    QVERIFY(registry.activateFor(&content));
    QCOMPARE(provider.activateCount, 1);  // 重入不再触发 activateSettingContext
    QCOMPARE(nestedResult, 1);            // 重入调用返回「已命中/已接管」
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);
}

// 激活回调内焦点切到固定窗口（APP 路由会调用 deactivateSettingContextProvider）：
// 激活视为已被取代——不回写撤销栈、返回 false、当前项保持为空，随后可正常重新激活。
void APPRefactorTest::testSettingContextProvider_nestedDeactivateDuringActivateRollsBack()
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
    group.setActiveStack(&otherStack);
    provider.undoStack = &figureStack;
    registry.registerProvider(&content, &provider);
    provider.onActivate = [ &registry ]() { registry.deactivateCurrent(); };

    QVERIFY2(!registry.activateFor(&content), "activation superseded by a nested deactivation must report failure");
    QCOMPARE(provider.activateCount, 1);
    // 重入保护：嵌套失活只清当前项，不重复回调失活（不与正在进行的激活交叉执行）
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), nullptr);
    QCOMPARE(registry.currentDockContent(), nullptr);
    // 状态不分叉：撤销栈没有被切到被取代的提供者
    QVERIFY2(otherStack.isActive(), "the host undo stack must stay active when the activation is rolled back");
    QVERIFY2(!figureStack.isActive(), "a rolled back activation must not activate the provider undo stack");

    // 回调结束后可正常重新激活：这次回写撤销栈并返回成功
    provider.onActivate = nullptr;
    QVERIFY(registry.activateFor(&content));
    QCOMPARE(registry.currentProvider(), &provider);
    QVERIFY2(figureStack.isActive(), "a successful activation must activate the provider undo stack");
}

// 失活回调内再次激活（APP 路由竞态）：重入保护挡住状态改写，失活结果保持
void APPRefactorTest::testSettingContextProvider_reentrantActivateDuringDeactivateIgnored()
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
    // 失活 A 期间路由又把 B 激活
    providerA.onDeactivate = [ &registry, &contentB ]() { registry.activateFor(&contentB); };

    registry.deactivateCurrent();
    QCOMPARE(providerA.deactivateCount, 1);
    QCOMPARE(providerB.activateCount, 0);
    QCOMPARE(registry.currentProvider(), nullptr);
    QCOMPARE(registry.currentDockContent(), nullptr);
}

// 失活回调内注销自身（插件关页/卸载路径）：不 UAF、条目被清理、不再命中
void APPRefactorTest::testSettingContextProvider_unregisterInsideDeactivateSafe()
{
    using namespace DA;
    DASettingContextProviderRegistry registry;
    QWidget content;
    FakeSettingContextProvider provider;
    registry.registerProvider(&content, &provider);
    provider.onDeactivate = [ &registry, &content ]() { registry.unregisterProvider(&content); };
    QVERIFY(registry.activateFor(&content));

    registry.deactivateCurrent();
    QCOMPARE(provider.deactivateCount, 1);
    QCOMPARE(registry.providerCount(), 0);
    QCOMPARE(registry.currentProvider(), nullptr);
    QVERIFY(!registry.isRegistered(&content));
    QVERIFY(!registry.activateFor(&content));
}

// 路由判定：插件设置上下文提供者优先，命中即由 provider 接管（不进入 hide-all 分支）
void APPRefactorTest::testContextRoute_providerTakesPrecedence()
{
    using namespace DA;
    QWidget pluginPage;
    QWidget dataOperate;
    QWidget workflowOperate;
    QWidget chartOperate;
    QWidget dataManage;

    // 注册 + 激活：模拟 dock 激活后的路由路径（命中 → 回调被调用 → 控制器 return，不 hide-all）
    DASettingContextProviderRegistry registry;
    FakeSettingContextProvider provider;
    registry.registerProvider(&pluginPage, &provider);
    const DAAppContextRoutePolicy route = resolveAppContextRoutePolicy(
        &pluginPage, registry.providerFor(&pluginPage) != nullptr, &dataOperate, &workflowOperate, &chartOperate, &dataManage);
    QCOMPARE(static_cast< int >(route), static_cast< int >(DAAppContextRoutePolicy::SettingContextProvider));
    QVERIFY(registry.activateFor(&pluginPage));
    QCOMPARE(provider.activateCount, 1);
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);
}

// 路由判定：三个固定操作窗口 + 数据管理窗口；焦点回到固定窗口时才失活插件上下文
void APPRefactorTest::testContextRoute_fixedOperateWindows()
{
    using namespace DA;
    QWidget dataOperate;
    QWidget workflowOperate;
    QWidget chartOperate;
    QWidget dataManage;
    QWidget pluginPage;

    QCOMPARE(static_cast< int >(resolveAppContextRoutePolicy(
                 &dataOperate, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage)),
             static_cast< int >(DAAppContextRoutePolicy::DataOperate));
    QCOMPARE(static_cast< int >(resolveAppContextRoutePolicy(
                 &workflowOperate, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage)),
             static_cast< int >(DAAppContextRoutePolicy::WorkFlowOperate));
    QCOMPARE(static_cast< int >(resolveAppContextRoutePolicy(
                 &chartOperate, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage)),
             static_cast< int >(DAAppContextRoutePolicy::ChartOperate));
    QCOMPARE(static_cast< int >(resolveAppContextRoutePolicy(
                 &dataManage, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage)),
             static_cast< int >(DAAppContextRoutePolicy::DataManage));

    // 插件上下文激活后切回绘图窗口：控制器会对固定窗口调用 deactivateSettingContextProvider()
    DASettingContextProviderRegistry registry;
    FakeSettingContextProvider provider;
    registry.registerProvider(&pluginPage, &provider);
    QVERIFY(registry.activateFor(&pluginPage));
    const DAAppContextRoutePolicy route =
        resolveAppContextRoutePolicy(&chartOperate, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage);
    QCOMPARE(static_cast< int >(route), static_cast< int >(DAAppContextRoutePolicy::ChartOperate));
    registry.deactivateCurrent();
    QCOMPARE(provider.deactivateCount, 1);
}

// 路由判定：工具 dock（设置/日志/图元树等）激活不改变插件上下文
void APPRefactorTest::testContextRoute_utilityDockKeepsProvider()
{
    using namespace DA;
    QWidget dataOperate;
    QWidget workflowOperate;
    QWidget chartOperate;
    QWidget dataManage;
    QWidget settingDock;
    QWidget pluginPage;

    const DAAppContextRoutePolicy route = resolveAppContextRoutePolicy(
        &settingDock, false, &dataOperate, &workflowOperate, &chartOperate, &dataManage);
    QCOMPARE(static_cast< int >(route), static_cast< int >(DAAppContextRoutePolicy::UtilityDock));

    // 工具 dock 激活后（控制器不调用 deactivate），插件上下文仍保持
    DASettingContextProviderRegistry registry;
    FakeSettingContextProvider provider;
    registry.registerProvider(&pluginPage, &provider);
    QVERIFY(registry.activateFor(&pluginPage));
    QCOMPARE(static_cast< int >(route), static_cast< int >(DAAppContextRoutePolicy::UtilityDock));
    QCOMPARE(provider.deactivateCount, 0);
    QCOMPARE(registry.currentProvider(), &provider);
    QCOMPARE(registry.currentDockContent(), &pluginPage);
}

// 路由判定：nullptr（焦点离开全部 dock 内容）由控制器先走 hide-all，本函数返回工具 dock
void APPRefactorTest::testContextRoute_nullWidget()
{
    using namespace DA;
    QWidget dataOperate;
    QCOMPARE(static_cast< int >(resolveAppContextRoutePolicy(nullptr, false, &dataOperate, nullptr, nullptr, nullptr)),
             static_cast< int >(DAAppContextRoutePolicy::UtilityDock));
}

// ===============================================================
// 绘图动作可用性策略（DAChartContextActionPolicy）
// ===============================================================

namespace
{
// 构造策略输入，避免每个用例重复写结构体初始化
DA::DAChartContextActionInput makeActionInput(DA::DAChartSubChartKind kind, bool fromProvider, bool gridLayout)
{
    DA::DAChartContextActionInput input;
    input.subChart     = kind;
    input.fromProvider = fromProvider;
    input.gridLayout   = gridLayout;
    return input;
}
}  // namespace

// 宿主 2D 自由布局：所有绘图动作可用（与改造前的默认行为等价）
void APPRefactorTest::testChartActionPolicy_2DFreeLayout()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::Chart2D, false, false));
    for (int group = 0; group < static_cast< int >(DAChartActionGroup::Count); ++group) {
        QVERIFY2(plan.isEnabled(static_cast< DAChartActionGroup >(group)),
                 qPrintable(QStringLiteral("host 2D free layout must enable action group %1").arg(group)));
    }
}

// 宿主 3D 子图：2D 交互/编辑器/几何编辑不可用，主题/复制/导出与结构入口保持可用
void APPRefactorTest::testChartActionPolicy_3DDisables2DInteraction()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::Chart3D, false, false));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::Chart2DInteraction));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::ChartEditorTools));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::ResizeSubChart));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::DataProbe));
    // figure 级动作 2D/3D 都可用（3D 导出为 PNG）
    QVERIFY(plan.isEnabled(DAChartActionGroup::FigureTheme));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ExportChart));
    // 宿主结构动作不受 3D 影响
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddFigure));
    QVERIFY(plan.isEnabled(DAChartActionGroup::NewXYAxis));
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddSeries));
}

// 宿主网格布局：调整子图大小/新建坐标系不可用，其它 2D 动作（含数据探针）保持可用
void APPRefactorTest::testChartActionPolicy_gridDisablesGeometryEditing()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::Chart2D, false, true));
    QVERIFY(plan.isEnabled(DAChartActionGroup::Chart2DInteraction));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ChartEditorTools));
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::ResizeSubChart), "grid layout must disable resize sub-chart");
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::NewXYAxis), "grid layout must disable new axes");
    QVERIFY(plan.isEnabled(DAChartActionGroup::DataProbe));
    QVERIFY(plan.isEnabled(DAChartActionGroup::FigureTheme));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ExportChart));
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddFigure));
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddSeries));
}

// provider 2D：结构动作与主「绘图」添加入口禁用，2D 交互/figure 级动作可用
void APPRefactorTest::testChartActionPolicy_providerDisablesStructure()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::Chart2D, true, false));
    QVERIFY(plan.isEnabled(DAChartActionGroup::Chart2DInteraction));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ChartEditorTools));
    QVERIFY(plan.isEnabled(DAChartActionGroup::FigureTheme));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ExportChart));
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::AddFigure), "provider context must disable new figure");
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::NewXYAxis), "provider context must disable new axes");
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::ResizeSubChart), "provider context must disable resize");
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::DataProbe), "provider context must disable data probe");
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::AddSeries), "provider context must disable add-curve/scatter/bar/3D");
}

// provider + 3D：选择器/辅助标记（编辑器工具）与 2D 交互一并禁用
void APPRefactorTest::testChartActionPolicy_provider3DDisablesSelectors()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::Chart3D, true, false));
    QVERIFY2(!plan.isEnabled(DAChartActionGroup::ChartEditorTools), "3D must disable selectors and markers");
    QVERIFY(!plan.isEnabled(DAChartActionGroup::Chart2DInteraction));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::DataProbe));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::AddSeries));
    QVERIFY(plan.isEnabled(DAChartActionGroup::FigureTheme));
    QVERIFY(plan.isEnabled(DAChartActionGroup::ExportChart));
}

// 无激活子图（宿主）：2D 与 figure 级动作置灰，结构入口仍可用
void APPRefactorTest::testChartActionPolicy_noSubChartDisablesChartActions()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::None, false, false));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::Chart2DInteraction));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::ChartEditorTools));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::ResizeSubChart));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::DataProbe));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::FigureTheme));
    QVERIFY(!plan.isEnabled(DAChartActionGroup::ExportChart));
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddFigure));
    QVERIFY(plan.isEnabled(DAChartActionGroup::NewXYAxis));
    QVERIFY(plan.isEnabled(DAChartActionGroup::AddSeries));
}

// provider 但没声明 figure（无激活子图）：全部绘图动作禁用（含结构入口）
void APPRefactorTest::testChartActionPolicy_providerWithoutSubChart()
{
    using namespace DA;
    const DAChartContextActionPlan plan =
        resolveChartContextActionPlan(makeActionInput(DAChartSubChartKind::None, true, false));
    for (int group = 0; group < static_cast< int >(DAChartActionGroup::Count); ++group) {
        QVERIFY2(!plan.isEnabled(static_cast< DAChartActionGroup >(group)),
                 qPrintable(QStringLiteral("provider without figure must disable action group %1").arg(group)));
    }
}

// 非法分组下标：stateOf 返回 Disabled（防御性，不让越界读未定义值）
void APPRefactorTest::testChartActionPolicy_stateOfInvalidGroup()
{
    using namespace DA;
    const DAChartContextActionPlan plan;
    QCOMPARE(static_cast< int >(plan.stateOf(static_cast< DAChartActionGroup >(999))),
             static_cast< int >(DAChartActionState::Disabled));
}

// 表驱动不变量：3 种子图 × provider/宿主 × 网格/自由 全部组合
void APPRefactorTest::testChartActionPolicy_matrixInvariants()
{
    using namespace DA;
    const DAChartSubChartKind kinds[] = { DAChartSubChartKind::None, DAChartSubChartKind::Chart2D,
                                          DAChartSubChartKind::Chart3D };
    for (DAChartSubChartKind kind : kinds) {
        for (int providerFlag = 0; providerFlag <= 1; ++providerFlag) {
            for (int gridFlag = 0; gridFlag <= 1; ++gridFlag) {
                const bool fromProvider = (providerFlag != 0);
                const bool gridLayout   = (gridFlag != 0);
                const bool is2D         = (DAChartSubChartKind::Chart2D == kind);
                const bool hasSubChart  = (DAChartSubChartKind::None != kind);
                const DAChartContextActionPlan plan =
                    resolveChartContextActionPlan(makeActionInput(kind, fromProvider, gridLayout));

                QCOMPARE(plan.isEnabled(DAChartActionGroup::Chart2DInteraction), is2D);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::ChartEditorTools), is2D);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::ResizeSubChart), is2D && !gridLayout && !fromProvider);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::DataProbe), is2D && !fromProvider);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::FigureTheme), hasSubChart);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::ExportChart), hasSubChart);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::AddFigure), !fromProvider);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::AddSeries), !fromProvider);
                QCOMPARE(plan.isEnabled(DAChartActionGroup::NewXYAxis), !fromProvider && !gridLayout);
            }
        }
    }
}

QTEST_MAIN(APPRefactorTest)
#include "main.moc"
