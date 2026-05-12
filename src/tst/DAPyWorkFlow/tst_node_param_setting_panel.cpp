#include "tst_node_param_setting_panel.h"
#include "DANodeParamSettingPanel.h"
#include "DAParamTypeRegistry.h"
#include "DAPyWorkFlow/DAParameterDescriptor.h"
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

}  // namespace DA
