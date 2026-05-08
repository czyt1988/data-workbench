#include "tst_node_param_setting_panel.h"
#include "DANodeParamSettingPanel.h"
#include "DAParamTypeRegistry.h"
#include "DAPyWorkFlow/ParameterDescriptor.h"
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
    QLabel* placeholder = qobject_cast<QLabel*>(item->editorWidget());
    QVERIFY2(placeholder != nullptr, "占位属性项的编辑器应为 QLabel");
    QCOMPARE(placeholder->text(), QStringLiteral("无可配置参数"));
}

/**
 * @brief 构建 int 类型参数属性面板
 *
 * 使用包含一个 int 类型参数的描述符 JSON，
 * 验证面板创建后 propertyCount 为 1，且包含正确的整数编辑器。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithIntParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]        = "threshold";
    p1["type"]        = "int";
    p1["description"] = "阈值";
    p1["default"]     = 10;
    p1["min"]         = 0;
    p1["max"]         = 100;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);

    // 验证编辑器类型为 QSpinBox
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(item->editorWidget());
    QVERIFY2(spinBox != nullptr, "int 参数编辑器应为 QSpinBox");
    QCOMPARE(spinBox->value(), 10);
    QCOMPARE(spinBox->minimum(), 0);
    QCOMPARE(spinBox->maximum(), 100);
}

/**
 * @brief 构建 float 类型参数属性面板
 *
 * 验证 QDoubleSpinBox 编辑器创建和默认值设置。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithFloatParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]    = "ratio";
    p1["type"]    = "float";
    p1["default"] = 0.5;
    p1["min"]     = 0.0;
    p1["max"]     = 1.0;
    p1["decimals"] = 3;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QDoubleSpinBox* dSpinBox = qobject_cast<QDoubleSpinBox*>(item->editorWidget());
    QVERIFY2(dSpinBox != nullptr, "float 参数编辑器应为 QDoubleSpinBox");
    QCOMPARE(dSpinBox->value(), 0.5);
}

/**
 * @brief 构建 bool 类型参数属性面板
 *
 * 验证 QCheckBox 编辑器创建和默认选中状态。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithBoolParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]    = "enabled";
    p1["type"]    = "bool";
    p1["default"] = true;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(item->editorWidget());
    QVERIFY2(checkBox != nullptr, "bool 参数编辑器应为 QCheckBox");
    QCOMPARE(checkBox->isChecked(), true);
}

/**
 * @brief 构建 str 类型参数属性面板
 *
 * 验证 QLineEdit 编辑器创建和默认文本设置。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithStrParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]        = "filename";
    p1["type"]        = "str";
    p1["description"] = "文件名";
    p1["default"]     = "test.txt";
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(item->editorWidget());
    QVERIFY2(lineEdit != nullptr, "str 参数编辑器应为 QLineEdit");
    QCOMPARE(lineEdit->text(), QStringLiteral("test.txt"));
}

/**
 * @brief 构建 enum 类型参数属性面板
 *
 * 验证 QComboBox 编辑器创建和 enum_values 填充。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithEnumParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]        = "mode";
    p1["type"]        = "enum";
    p1["description"] = "运行模式";
    QJsonArray enumVals;
    enumVals.append("fast");
    enumVals.append("normal");
    enumVals.append("slow");
    p1["enum_values"] = enumVals;
    p1["default"]     = "normal";
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QComboBox* combo = qobject_cast<QComboBox*>(item->editorWidget());
    QVERIFY2(combo != nullptr, "enum 参数编辑器应为 QComboBox");
    QCOMPARE(combo->count(), 3);
    QCOMPARE(combo->currentText(), QStringLiteral("normal"));
}

/**
 * @brief 构建 file 类型参数属性面板
 *
 * 验证 DAFilePathEditWidget 编辑器创建。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithFileParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"]    = "filepath";
    p1["type"]    = "file";
    p1["default"] = "/tmp/data.csv";
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    // file 类型编辑器由 DAParamTypeRegistry 创建，为 DAFilePathEditWidget
    QVERIFY(item->editorWidget() != nullptr);
}

/**
 * @brief 构建多参数属性面板
 *
 * 验证多个参数同时创建时 propertyCount 和 propertyId 递增。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithMultipleParams()
{
    QJsonArray params;
    QJsonObject p1, p2, p3;
    p1["name"] = "count"; p1["type"] = "int"; p1["default"] = 5;
    p2["name"] = "label"; p2["type"] = "str"; p2["default"] = "hello";
    p3["name"] = "flag";  p3["type"] = "bool"; p3["default"] = false;
    params.append(p1);
    params.append(p2);
    params.append(p3);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QCOMPARE(panel.propertyPanel()->propertyCount(), 3);
    QVERIFY(panel.propertyPanel()->getPropertyItem(1) != nullptr);
    QVERIFY(panel.propertyPanel()->getPropertyItem(2) != nullptr);
    QVERIFY(panel.propertyPanel()->getPropertyItem(3) != nullptr);
}

/**
 * @brief 无效参数描述符被跳过
 *
 * 缺少 name 或 type 字段的描述符应被跳过，不影响正常参数的创建。
 */
void TestNodeParamSettingPanel::testBuildPropertyPanelWithInvalidParamSkipped()
{
    QJsonArray params;
    // 有效参数
    QJsonObject p1;
    p1["name"] = "valid_param"; p1["type"] = "int"; p1["default"] = 1;
    params.append(p1);
    // 无效参数：缺少 name
    QJsonObject p2;
    p2["type"] = "str";
    params.append(p2);
    // 无效参数：缺少 type
    QJsonObject p3;
    p3["name"] = "no_type";
    params.append(p3);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    // 只有有效参数被创建
    QCOMPARE(panel.propertyPanel()->propertyCount(), 1);
}

/**
 * @brief 3-hop 信号链验证
 *
 * 信号链: mPanel→onPanelPropertyValueChanged→emit propertyValueChanged→外部接收
 * 改变 int 编辑器值 → 验证 propertyValueChanged 信号被发射。
 */
void TestNodeParamSettingPanel::testSignalChainPropertyValueChanged()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"] = "threshold"; p1["type"] = "int"; p1["default"] = 10;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QSignalSpy spy(&panel, &DANodeParamSettingPanel::propertyValueChanged);

    // 模拟属性面板值变化
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QVERIFY(item != nullptr);
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(item->editorWidget());
    QVERIFY(spinBox != nullptr);

    // 改变值触发信号链
    spinBox->setValue(20);

    // 3-hop: mPanel propertyValueChanged → onPanelPropertyValueChanged → emit propertyValueChanged
    QVERIFY(spy.count() >= 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 1);  // propertyId = 1
}

/**
 * @brief updateUI 使用 QSignalBlocker
 *
 * 调用 updateUI 时不应触发 propertyValueChanged 信号。
 * 测试方法：设置 int 编辑器值，调用 updateUI，验证信号未被发射。
 */
void TestNodeParamSettingPanel::testUpdateUIBlocksSignals()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"] = "count"; p1["type"] = "int"; p1["default"] = 5;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QSignalSpy spy(&panel, &DANodeParamSettingPanel::propertyValueChanged);

    // 直接调用 updateUI（无代理时应安全处理）
    panel.updateUI();

    // updateUI 应使用 QSignalBlocker，不应触发信号
    QCOMPARE(spy.count(), 0);
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
    QVERIFY(qobject_cast<DAPropertyPanelContainerWidget*>(pPanel) != nullptr);
}

/**
 * @brief collectConfig 收集 int 参数配置
 *
 * 验证从 int 编辑器收集值生成正确的 QJsonObject。
 */
void TestNodeParamSettingPanel::testCollectConfigFromIntParam()
{
    QJsonArray params;
    QJsonObject p1;
    p1["name"] = "threshold"; p1["type"] = "int"; p1["default"] = 10;
    params.append(p1);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    // 改变编辑器值
    DAPropertyItemWidget* item = panel.propertyPanel()->getPropertyItem(1);
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(item->editorWidget());
    spinBox->setValue(20);

    // 收集配置
    QJsonObject config = panel.testCollectConfig();
    QVERIFY(config.contains("threshold"));
    QCOMPARE(config["threshold"].toInt(), 20);
}

/**
 * @brief collectConfig 收集多参数配置
 *
 * 验证从多个编辑器收集值生成完整的 QJsonObject。
 */
void TestNodeParamSettingPanel::testCollectConfigFromMultipleParams()
{
    QJsonArray params;
    QJsonObject p1, p2;
    p1["name"] = "count"; p1["type"] = "int"; p1["default"] = 5;
    p2["name"] = "label"; p2["type"] = "str"; p2["default"] = "hello";
    params.append(p1);
    params.append(p2);

    DANodeParamSettingPanel panel;
    panel.testBuildPropertyPanelFromJson(ParameterDescriptor::fromJsonArray(params));

    QJsonObject config = panel.testCollectConfig();
    QVERIFY(config.contains("count"));
    QVERIFY(config.contains("label"));
    QCOMPARE(config["count"].toInt(), 5);
    QCOMPARE(config["label"].toString(), QStringLiteral("hello"));
}

}  // namespace DA