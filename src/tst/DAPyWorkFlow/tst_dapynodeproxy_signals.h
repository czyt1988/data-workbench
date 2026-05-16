#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DAPyNodeProxy 信号的单元测试
 *
 * 验证 nodeNameChanged、nodeStateChanged、parameterValueChanged
 * 三个信号在对应 setter 调用时正确触发并传递参数。
 * 不依赖 Python 运行时。
 */
class TestDAPyNodeProxySignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test_nodeNameChanged();
    void test_nodeStateChanged();
    void test_parameterValueChanged();
    void test_setConfigEmitsMultipleSignals();
    void test_setConfigNoPyRefSafe();
};

}  // namespace DA