#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DANodeParamSettingPanelFactory 工厂类的测试
 *
 * 验证工厂注册、创建面板、单例等基本功能。
 */
class TestNodeParamSettingFactory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();   // 每个测试前清理
    void cleanup(); // 每个测试后清理

    void testSingletonInstance();
    void testRegisterAndHasPanel();
    void testCreatePanelCallsCreator();
    void testCreatePanelReturnsNullForUnregistered();
    void testRegisterOverwritesExisting();
    void testRegisteredNames();
    void testRegisterWithNullCreatorDoesNothing();
    void testRegisterDefaultPanels();
};

}  // namespace DA
