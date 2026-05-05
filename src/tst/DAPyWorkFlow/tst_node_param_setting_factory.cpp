#include "tst_node_param_setting_factory.h"
#include "DANodeParamSettingPanelFactory.h"
#include "DANodeParamSettingPanel.h"
#include <QtTest/QtTest>

namespace DA
{

// 每个测试执行前：无需清理
void TestNodeParamSettingFactory::init()
{
}

// 每个测试执行后：无需清理
void TestNodeParamSettingFactory::cleanup()
{
}

/**
 * @brief 验证单例模式：多次调用 instance() 返回同一引用
 */
void TestNodeParamSettingFactory::testSingletonInstance()
{
    auto& instance1 = DANodeParamSettingPanelFactory::instance();
    auto& instance2 = DANodeParamSettingPanelFactory::instance();

    // 验证两次获取的是同一个对象
    QCOMPARE(&instance1, &instance2);
}

/**
 * @brief 验证注册和 hasPanel 基本功能
 */
void TestNodeParamSettingFactory::testRegisterAndHasPanel()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    QString testName = "test::registerAndHasPanel";

    // 初始时未注册
    QVERIFY(!factory.hasPanel(testName));

    // 注册
    factory.registerPanel(testName, [](QWidget* parent) {
        return new DANodeParamSettingPanel(parent);
    });

    // 注册后存在
    QVERIFY(factory.hasPanel(testName));
}

/**
 * @brief 验证 createPanel 正确调用创建器并返回非空指针
 */
void TestNodeParamSettingFactory::testCreatePanelCallsCreator()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    QString testName = "test::createPanelCallsCreator";
    bool creatorCalled = false;

    factory.registerPanel(testName, [&creatorCalled](QWidget* parent) {
        creatorCalled = true;
        return new DANodeParamSettingPanel(parent);
    });

    // 创建面板
    DANodeParamSettingPanel* panel = factory.createPanel(testName, nullptr);

    // 验证创建器被调用且返回非空
    QVERIFY(creatorCalled);
    QVERIFY(panel != nullptr);

    // 清理（避免内存泄漏）
    delete panel;
}

/**
 * @brief 验证对未注册类型调用 createPanel 返回 nullptr
 */
void TestNodeParamSettingFactory::testCreatePanelReturnsNullForUnregistered()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    QString testName = "test::unregisteredPanel_shouldNotExist";

    // 确保未注册
    QVERIFY(!factory.hasPanel(testName));

    // 创建未注册类型应返回 nullptr
    DANodeParamSettingPanel* panel = factory.createPanel(testName, nullptr);
    QCOMPARE(panel, static_cast<DANodeParamSettingPanel*>(nullptr));
}

/**
 * @brief 验证重复注册同名项会覆盖原有创建器
 */
void TestNodeParamSettingFactory::testRegisterOverwritesExisting()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    QString testName = "test::registerOverwrites";
    int firstCreatorCalled = 0;
    int secondCreatorCalled = 0;

    // 第一次注册
    factory.registerPanel(testName, [&firstCreatorCalled](QWidget* parent) {
        firstCreatorCalled++;
        return new DANodeParamSettingPanel(parent);
    });

    // 第二次注册（覆盖）
    factory.registerPanel(testName, [&secondCreatorCalled](QWidget* parent) {
        secondCreatorCalled++;
        return new DANodeParamSettingPanel(parent);
    });

    // 创建面板
    DANodeParamSettingPanel* panel = factory.createPanel(testName, nullptr);

    // 验证只有第二个创建器被调用
    QCOMPARE(firstCreatorCalled, 0);
    QCOMPARE(secondCreatorCalled, 1);
    QVERIFY(panel != nullptr);

    delete panel;
}

/**
 * @brief 验证 registeredNames 返回所有已注册的名称
 */
void TestNodeParamSettingFactory::testRegisteredNames()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    // 记录注册前的名称数量
    int beforeCount = factory.registeredNames().size();

    // 注册两个唯一名称
    QString name1 = "test::registeredNames_1";
    QString name2 = "test::registeredNames_2";

    factory.registerPanel(name1, [](QWidget* parent) {
        return new DANodeParamSettingPanel(parent);
    });
    factory.registerPanel(name2, [](QWidget* parent) {
        return new DANodeParamSettingPanel(parent);
    });

    QStringList names = factory.registeredNames();

    // 验证名称数量增加了 2
    QCOMPARE(names.size(), beforeCount + 2);
    // 验证包含新注册的两个名称
    QVERIFY(names.contains(name1));
    QVERIFY(names.contains(name2));
}

/**
 * @brief 验证注册 nullptr 创建器不会产生任何效果
 */
void TestNodeParamSettingFactory::testRegisterWithNullCreatorDoesNothing()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    QString testName = "test::nullCreator_shouldNotRegister";
    int beforeCount = factory.registeredNames().size();

    // 注册 null 创建器
    factory.registerPanel(testName, DANodeParamSettingPanelFactory::FpCreatePanel());

    // 验证未注册
    QVERIFY(!factory.hasPanel(testName));
    QCOMPARE(factory.registeredNames().size(), beforeCount);
}

/**
 * @brief 验证 registerDefaultPanels 可正常调用（当前为空实现）
 */
void TestNodeParamSettingFactory::testRegisterDefaultPanels()
{
    auto& factory = DANodeParamSettingPanelFactory::instance();

    // 记录调用前的数量
    int beforeCount = factory.registeredNames().size();

    // 调用默认注册（当前为空实现）
    factory.registerDefaultPanels();

    // 验证没有新增任何面板
    QCOMPARE(factory.registeredNames().size(), beforeCount);
}

}  // namespace DA
