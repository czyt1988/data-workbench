#include "tst_node_param_setting_panel.h"
#include "DANodeParamSettingPanel.h"
#include "DAParamTypeRegistry.h"
#include "DAPyWorkFlow/DAParameterDescriptor.h"
#include "DAPyWorkFlow/DAPyNodeProxy.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QtTest/QtTest>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>

namespace DA
{

/**
 * @brief 测试初始化
 */
void TestNodeParamSettingPanel::initTestCase()
{
}

/**
 * @brief 创建空面板（无参数描述符）
 *
 * 验证面板在无描述符时能正常创建，且内部 mPanel 已初始化。
 * 空面板默认包含一个占位属性（显示 "无可配置参数"），因此 propertyCount 为 1。
 */
void TestNodeParamSettingPanel::testCreateEmptyPanel()
{
    DANodeParamSettingPanel panel;
    QVERIFY(panel.propertyPanel() != nullptr);
    // buildPropertyPanel() 为空参数创建占位 QLabel 作为属性项，propertyCount = 1
    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
}

/**
 * @brief 空参数描述符时显示占位文本
 *
 * 当 getParameters() 返回空数组时，面板应显示 "无可配置参数" 占位标签。
 */
void TestNodeParamSettingPanel::testEmptyParametersShowsPlaceholder()
{
    DANodeParamSettingPanel panel;
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY2(item != nullptr, "空参数时应创建占位属性项");
    QLabel* placeholder = qobject_cast< QLabel* >(item->editorWidget());
    QVERIFY2(placeholder != nullptr, "占位属性项的编辑器应为 QLabel");
    QCOMPARE(placeholder->text(), QStringLiteral("无可配置参数"));
}

/**
 * @brief propertyPanel 访问器
 *
 * 验证 propertyPanel() 返回有效的 DAPropertyPanelContainerWidget 指针。
 */
void TestNodeParamSettingPanel::testPropertyPanelAccessor()
{
    DANodeParamSettingPanel panel;
    DAPropertyPanelContainerWidget* pPanel = panel.propertyPanel();
    QVERIFY(pPanel != nullptr);
    QVERIFY(qobject_cast< DAPropertyPanelContainerWidget* >(pPanel) != nullptr);
}

/**
 * @brief buildPropertyPanel 创建各类型编辑器
 *
 * TODO: 需要重构测试以适应纯代理模式的 DAPyNodeProxy（不再有 getDescriptorStruct）
 * 需要 Python 运行时环境来创建真正的 proxy 对象
 */
void TestNodeParamSettingPanel::test_buildPropertyPanelCreatesEditors()
{
    // TODO: 重构为使用 Python 运行时创建真实 proxy
}

/**
 * @brief collectConfig 收集所有编辑器当前值
 *
 * TODO: 需要重构测试以适应纯代理模式
 */
void TestNodeParamSettingPanel::test_collectConfigGathersAllValues()
{
    // TODO: 重构为使用 Python 运行时创建真实 proxy
}

/**
 * @brief updateUI 从缓存回写编辑器值
 *
 * TODO: 需要重构测试以适应纯代理模式
 */
void TestNodeParamSettingPanel::test_updateUISetsWidgetValues()
{
    // TODO: 重构为使用 Python 运行时创建真实 proxy
}

}  // namespace DA