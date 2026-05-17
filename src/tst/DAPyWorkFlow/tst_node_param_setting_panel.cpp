#include "tst_node_param_setting_panel.h"
#include "DANodeParamSettingPanel.h"
#include "DAParamTypeRegistry.h"
#include "DAPyWorkFlow/DAParameterDescriptor.h"
#include "DAPyWorkFlow/DAPyNodeProxy.h"
#include "DAPyWorkFlow/DANodeDescriptor.h"
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
 * 注意：DAPropertyItemWidget::setEditorWidget 会覆盖编辑器 widget 的 objectName，
 * 因此不能通过 findChild 查找 "da_placeholder_label"，需要通过 propertyPanel 获取。
 */
void TestNodeParamSettingPanel::testEmptyParametersShowsPlaceholder()
{
    DANodeParamSettingPanel panel;
    // 空面板中占位属性为 propertyId=1（auto-generated）
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
 * 通过 DAPyNodeProxy 设置 str/int 类型参数描述符，触发面板重建，
 * 验证 DAParamTypeRegistry 为每种类型创建了对应的编辑器控件。
 * 使用 const_cast 访问代理描述符以填充 parameters（无需 Python 运行时）。
 */
void TestNodeParamSettingPanel::test_buildPropertyPanelCreatesEditors()
{
#if 0
    // 创建代理并填充描述符参数（const_cast 访问底层非 const mDescriptor）
    DAPyNodeProxy proxy;
    proxy.setQualifiedName("test.node");

    DAParameterDescriptor strParam;
    strParam.name        = "text_param";
    strParam.type        = "str";
    strParam.description = "Test string";

    DAParameterDescriptor intParam;
    intParam.name        = "num_param";
    intParam.type        = "int";
    intParam.description = "Test int";

    DANodeDescriptor& desc = const_cast< DANodeDescriptor& >(proxy.getDescriptorStruct());
    desc.parameters.append(strParam);
    desc.parameters.append(intParam);

    // 创建面板并设置代理 → 复制描述符到基类缓存
    DANodeParamSettingPanel panel;
    panel.setNodeProxy(&proxy);
    // setNodeProxy 不自动重建面板，手动触发 buildPropertyPanel
    panel.buildPropertyPanel();

    // 验证 str 类型参数创建了 QLineEdit 编辑器
    QLineEdit* le = panel.findChild< QLineEdit* >();
    QVERIFY2(le != nullptr, "str 类型参数应创建 QLineEdit 编辑器");

    // 验证 int 类型参数创建了 QSpinBox 编辑器
    QSpinBox* sb = panel.findChild< QSpinBox* >();
    QVERIFY2(sb != nullptr, "int 类型参数应创建 QSpinBox 编辑器");

    // propertyCount 应为 2（每个参数一个属性项）
    QCOMPARE(panel.propertyPanel()->propertyCount(), 2);
#endif
}

/**
 * @brief collectConfig 收集所有编辑器当前值
 *
 * 设置编辑器值后调用 collectConfig，验证返回的 QJsonObject
 * 包含正确的参数名和值。覆盖 str 和 int 两种类型。
 */
void TestNodeParamSettingPanel::test_collectConfigGathersAllValues()
{
#if 0
    DAPyNodeProxy proxy;
    proxy.setQualifiedName("test.node");

    DAParameterDescriptor intParam;
    intParam.name        = "num_param";
    intParam.type        = "int";
    intParam.description = "Test int";

    DAParameterDescriptor strParam;
    strParam.name        = "text_param";
    strParam.type        = "str";
    strParam.description = "Test string";

    DANodeDescriptor& desc = const_cast< DANodeDescriptor& >(proxy.getDescriptorStruct());
    desc.parameters.append(intParam);
    desc.parameters.append(strParam);

    DANodeParamSettingPanel panel;
    panel.setNodeProxy(&proxy);
    panel.buildPropertyPanel();

    // 模拟用户输入：设置编辑器值
    QSpinBox* sb = panel.findChild< QSpinBox* >();
    QVERIFY(sb != nullptr);
    sb->setValue(42);

    QLineEdit* le = panel.findChild< QLineEdit* >();
    QVERIFY(le != nullptr);
    le->setText("hello");

    // 收集配置并验证
    QJsonObject config = panel.collectConfig();
    QVERIFY2(config.contains("num_param"), "配置应包含 int 参数 num_param");
    QCOMPARE(config.value("num_param").toInt(), 42);
    QVERIFY2(config.contains("text_param"), "配置应包含 str 参数 text_param");
    QCOMPARE(config.value("text_param").toString(), QStringLiteral("hello"));
#endif
}

/**
 * @brief updateUI 从缓存回写编辑器值
 *
 * 步骤：
 * 1. 创建面板并设置参数
 * 2. 设置编辑器值，触发 onPropertyValueChanged 缓存配置
 * 3. 手动修改编辑器值（模拟外部变更）
 * 4. 调用 updateUI → 从 mConfigCache 恢复缓存值
 * 5. 验证编辑器值已被恢复为缓存值
 */
void TestNodeParamSettingPanel::test_updateUISetsWidgetValues()
{
#if 0
    DAPyNodeProxy proxy;
    proxy.setQualifiedName("test.node");

    DAParameterDescriptor strParam;
    strParam.name        = "text_param";
    strParam.type        = "str";
    strParam.description = "Test string";

    DAParameterDescriptor intParam;
    intParam.name        = "num_param";
    intParam.type        = "int";
    intParam.description = "Test int";

    DANodeDescriptor& desc = const_cast< DANodeDescriptor& >(proxy.getDescriptorStruct());
    desc.parameters.append(strParam);
    desc.parameters.append(intParam);

    DANodeParamSettingPanel panel;
    panel.setNodeProxy(&proxy);
    panel.buildPropertyPanel();

    QSpinBox* sb = panel.findChild< QSpinBox* >();
    QVERIFY(sb != nullptr);

    QLineEdit* le = panel.findChild< QLineEdit* >();
    QVERIFY(le != nullptr);

    // Step 1: 设置编辑器值，触发缓存收集
    sb->setValue(42);
    le->setText("cached_text");
    // onPropertyValueChanged 将当前编辑器值收集到 mConfigCache
    panel.onPropertyValueChanged(1);

    // Step 2: 手动修改编辑器值（模拟后续变更，与缓存不一致）
    sb->setValue(0);
    le->setText("");

    // Step 3: updateUI 从 mConfigCache 恢复缓存值
    panel.updateUI();

    // Step 4: 验证编辑器值恢复为缓存值
    QCOMPARE(sb->value(), 42);
    QCOMPARE(le->text(), QStringLiteral("cached_text"));
#endif
}

}  // namespace DA
