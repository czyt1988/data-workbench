#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DANodeParamSettingPanel 面板组件的单元测试
 *
 * 验证面板创建、属性展示、3-hop信号链和 updateUI 等核心功能。
 * 由于 DAPyNodeProxy 需要 Python 运行时，测试使用模拟描述符（QJsonObject）
 * 直接设置参数，不依赖真实代理。
 */
class TestNodeParamSettingPanel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    // 面板创建与空参数
    void testCreateEmptyPanel();
    void testEmptyParametersShowsPlaceholder();

    // buildPropertyPanel 功能
    void testBuildPropertyPanelWithIntParam();
    void testBuildPropertyPanelWithFloatParam();
    void testBuildPropertyPanelWithBoolParam();
    void testBuildPropertyPanelWithStrParam();
    void testBuildPropertyPanelWithEnumParam();
    void testBuildPropertyPanelWithFileParam();
    void testBuildPropertyPanelWithMultipleParams();
    void testBuildPropertyPanelWithInvalidParamSkipped();

    // 3-hop 信号链
    void testSignalChainPropertyValueChanged();

    // updateUI 与 QSignalBlocker
    void testUpdateUIBlocksSignals();

    // propertyPanel 访问
    void testPropertyPanelAccessor();

    // collectConfig 收集配置
    void testCollectConfigFromIntParam();
    void testCollectConfigFromMultipleParams();
};

}  // namespace DA