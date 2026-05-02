#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DAPyNodeGraphicsItem::generateLinkPoints() 连接点生成测试
 *
 * 验证描述符路径和代理回退路径的连接点生成逻辑，
 * 重点暴露 Bug 3：薄描述符（非空但无 inputs/outputs）导致
 * generateLinkPoints() 进入描述符路径返回空，不回退到代理路径。
 */
class TestLinkPoints : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 薄描述符（无I/O数组）→ 连接点为空（描述符路径正确返回空）
    void testThinDescriptorNoIO();

    // 完整描述符（有I/O数组）→ 连接点非空
    void testFullDescriptorWithIO();

    // 薄描述符 + 有keys的代理 → 应回退到代理返回非空连接点
    // Bug 3: 当前返回空（描述符路径返回空后不回退到代理）
    void testThinDescriptorWithProxyFallback();

    // 集成测试：DAPyNodeProxy → DAPyNodeGraphicsItem → 连接点
    // 验证薄描述符+代理回退的完整管线（Bug 1数据丢失消除、Bug 2 setDescriptor覆写移除、Bug 3代理回退生效）
    void testIntegrationProxyToLinkPoints();
};

}  // namespace DA