#include "tst_dapynodeproxy_signals.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodeState.h"
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <QJsonObject>

namespace DA
{

/**
 * @brief 验证 nodeNameChanged 信号
 *
 * 调用 setNodeName 后，nodeNameChanged 信号应触发一次，
 * 且携带的参数与传入值一致。
 */
void TestDAPyNodeProxySignals::test_nodeNameChanged()
{
    DAPyNodeProxy proxy;
    QSignalSpy spy(&proxy, &DAPyNodeProxy::nodeNameChanged);
    proxy.setNodeName("test_name");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("test_name"));
}

/**
 * @brief 验证 nodeStateChanged 信号
 *
 * 调用 setNodeState 后，nodeStateChanged 信号应触发一次，
 * 且携带的 DAPyNodeState 参数与传入值一致。
 */
void TestDAPyNodeProxySignals::test_nodeStateChanged()
{
    DAPyNodeProxy proxy;
    QSignalSpy spy(&proxy, &DAPyNodeProxy::nodeStateChanged);
    proxy.setNodeState(DAPyNodeState::Running);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value< DAPyNodeState >(), DAPyNodeState::Running);
}

/**
 * @brief 验证 parameterValueChanged 信号
 *
 * 调用 setConfig 后，parameterValueChanged 信号应至少触发一次，
 * 表示配置参数发生了变化。setConfig 将 QJsonObject 中的每个键值对
 * 映射为 (propertyId, QVariant) 信号参数。
 */
void TestDAPyNodeProxySignals::test_parameterValueChanged()
{
    DAPyNodeProxy proxy;
    QSignalSpy spy(&proxy, &DAPyNodeProxy::parameterValueChanged);
    QJsonObject config;
    config["param1"] = 42;
    config["param2"] = "hello";
    proxy.setConfig(config);
    QVERIFY(spy.count() >= 1);
}

/**
 * @brief 验证 setConfig 多参数配置时每个键触发一次信号
 *
 * setConfig 将 QJsonObject 中的每个键值对映射为 parameterValueChanged 信号，
 * propertyId 使用 qHash(key) 计算。3 个键应触发 3 次信号。
 */
void TestDAPyNodeProxySignals::test_setConfigEmitsMultipleSignals()
{
    DAPyNodeProxy proxy;
    QSignalSpy spy(&proxy, &DAPyNodeProxy::parameterValueChanged);
    QJsonObject config;
    config["a"] = 1;
    config["b"] = 2;
    config["c"] = 3;
    proxy.setConfig(config);
    QCOMPARE(spy.count(), 3);
}

/**
 * @brief 验证无 Python 节点引用时 setConfig 不崩溃
 *
 * 未设置 pyNodeRef 的 DAPyNodeProxy 调用 setConfig 不应崩溃，
 * 信号仍会发射（本地缓存已更新），但 Python 操作被跳过。
 */
void TestDAPyNodeProxySignals::test_setConfigNoPyRefSafe()
{
    DAPyNodeProxy proxy;  // no pyNodeRef set
    QJsonObject config;
    config["x"] = 99;
    // Should NOT crash even with no Python ref
    proxy.setConfig(config);
    QVERIFY(true);  // reached this line = no crash
}

}  // namespace DA