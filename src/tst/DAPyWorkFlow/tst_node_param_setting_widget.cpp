#include "tst_node_param_setting_widget.h"
#include "DANodeParamSettingPanelWidget.h"
#include "DANodeParamSettingPanel.h"
#include "DANodeParamSettingPanelFactory.h"
#include <QtTest/QtTest>
#include <QStackedWidget>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

/**
 * @brief 测试初始化
 */
void TestDANodeParamSettingPanelWidget::initTestCase()
{
    mWidget = new DANodeParamSettingPanelWidget();
}

/**
 * @brief 验证控件创建后持有 QStackedWidget
 */
void TestDANodeParamSettingPanelWidget::test_createWidget_hasStackedWidget()
{
    auto* stacked = mWidget->findChild<QStackedWidget*>();
    QVERIFY(stacked != nullptr);
    // 初始只有占位标签一个页面
    QCOMPARE(stacked->count(), 1);
}

/**
 * @brief 验证控件创建后包含占位标签
 */
void TestDANodeParamSettingPanelWidget::test_createWidget_hasPlaceholder()
{
    auto* placeholder = mWidget->findChild<QLabel*>(QStringLiteral("da_node_placeholder_label"));
    QVERIFY(placeholder != nullptr);
    QCOMPARE(placeholder->text(), QStringLiteral("未选中节点"));

    // 占位标签应为当前显示页面
    auto* stacked = mWidget->findChild<QStackedWidget*>();
    QVERIFY(stacked != nullptr);
    QCOMPARE(stacked->currentWidget(), placeholder);
}

/**
 * @brief 验证 null 代理时显示占位标签
 */
void TestDANodeParamSettingPanelWidget::test_nullProxy_showsPlaceholder()
{
    mWidget->setNodeProxy(nullptr);

    auto* placeholder = mWidget->findChild<QLabel*>(QStringLiteral("da_node_placeholder_label"));
    auto* stacked     = mWidget->findChild<QStackedWidget*>();

    QCOMPARE(stacked->currentWidget(), placeholder);
    QCOMPARE(mWidget->currentPanel(), static_cast<DANodeParamSettingPanel*>(nullptr));
}

/**
 * @brief 验证设置代理后创建面板
 *
 * 使用 testSetNodeProxyWithDescriptor 绕过 DAPyNodeProxy，
 * 通过模拟描述符测试面板创建逻辑。
 */
void TestDANodeParamSettingPanelWidget::test_setNodeProxy_createsPanel()
{
    // 清除之前测试的缓存
    mWidget->clearCache();

    // 创建模拟描述符
    QJsonObject descriptor;
    descriptor["qualified_name"] = "test::createPanel_node";
    descriptor["parameters"]     = QJsonArray();

    // 使用测试辅助方法
    mWidget->testSetNodeProxyWithDescriptor(descriptor);

    // 验证面板已创建
    QVERIFY(mWidget->currentPanel() != nullptr);

    // 验证面板为 QStackedWidget 当前页面
    auto* stacked = mWidget->findChild<QStackedWidget*>();
    QCOMPARE(stacked->currentWidget(), mWidget->currentPanel());
}

/**
 * @brief 验证相同 qualifiedName 时复用缓存面板
 *
 * 两次设置相同 qualifiedName 的描述符，第二次应复用已缓存的面板。
 */
void TestDANodeParamSettingPanelWidget::test_cacheReuse_sameQualifiedName()
{
    // 清除之前测试的缓存
    mWidget->clearCache();

    // 第一次设置
    QJsonObject descriptor1;
    descriptor1["qualified_name"] = "test::cacheReuse_node";
    descriptor1["parameters"]     = QJsonArray();

    mWidget->testSetNodeProxyWithDescriptor(descriptor1);

    DANodeParamSettingPanel* firstPanel = mWidget->currentPanel();
    QVERIFY(firstPanel != nullptr);

    auto* stacked = mWidget->findChild<QStackedWidget*>();
    int pageCountAfterFirst = stacked->count();

    // 第二次设置相同 qualifiedName
    QJsonObject descriptor2;
    descriptor2["qualified_name"] = "test::cacheReuse_node";
    descriptor2["parameters"]     = QJsonArray();

    mWidget->testSetNodeProxyWithDescriptor(descriptor2);

    DANodeParamSettingPanel* secondPanel = mWidget->currentPanel();
    QVERIFY(secondPanel != nullptr);

    // 应复用同一个面板实例
    QCOMPARE(firstPanel, secondPanel);

    // QStackedWidget 页面数不变（没有新增）
    QCOMPARE(stacked->count(), pageCountAfterFirst);
}

/**
 * @brief 验证 clearCache 清除所有缓存面板
 */
void TestDANodeParamSettingPanelWidget::test_clearCache_removesAllPanels()
{
    // 先创建面板
    QJsonObject descriptor;
    descriptor["qualified_name"] = "test::clearCache_node";
    descriptor["parameters"]     = QJsonArray();
    mWidget->testSetNodeProxyWithDescriptor(descriptor);

    // 验证面板存在
    QVERIFY(mWidget->currentPanel() != nullptr);

    // 清除缓存
    mWidget->clearCache();

    // 验证当前面板为 nullptr
    QCOMPARE(mWidget->currentPanel(), static_cast<DANodeParamSettingPanel*>(nullptr));

    // 验证切换回占位标签
    auto* placeholder = mWidget->findChild<QLabel*>(QStringLiteral("da_node_placeholder_label"));
    auto* stacked     = mWidget->findChild<QStackedWidget*>();
    QCOMPARE(stacked->currentWidget(), placeholder);
}

/**
 * @brief 验证 currentPanel() 返回当前显示的面板
 */
void TestDANodeParamSettingPanelWidget::test_currentPanel_returnsCurrent()
{
    mWidget->clearCache();

    QJsonObject descriptor;
    descriptor["qualified_name"] = "test::currentPanel_node";
    descriptor["parameters"]     = QJsonArray();

    mWidget->testSetNodeProxyWithDescriptor(descriptor);

    DANodeParamSettingPanel* panel = mWidget->currentPanel();
    QVERIFY(panel != nullptr);

    // currentPanel 应与 stacked widget 当前页面一致
    auto* stacked = mWidget->findChild<QStackedWidget*>();
    QCOMPARE(stacked->currentWidget(), panel);
}

/**
 * @brief 验证无代理时 currentPanel() 返回 nullptr
 */
void TestDANodeParamSettingPanelWidget::test_currentPanel_nullWhenNoProxy()
{
    mWidget->setNodeProxy(nullptr);
    QCOMPARE(mWidget->currentPanel(), static_cast<DANodeParamSettingPanel*>(nullptr));
}

}  // namespace DA