#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DANodeStyle 及 DAPyLinkPointStyle 序列化/默认值测试
 *
 * 验证节点样式结构体的默认构造、JSON往返、稀疏策略、
 * XML持久化、向后兼容、连接点样式默认值及颜色序列化。
 */
class TestDANodeStyle : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 默认构造函数验证所有字段为预期默认值
    void testDefaultConstruction();

    // JSON序列化往返：修改后toJson→fromJson→验证值一致
    void testJsonRoundTrip();

    // 稀疏策略：默认样式toJson应产生空/最小JSON
    void testJsonSparseStrategy();

    // 安全默认值：fromJson空对象应产生全默认样式
    void testJsonSafeDefaults();

    // XML持久化往返：通过DAPyNodeGraphicsItem save/load
    void testXmlRoundTrip();

    // XML向后兼容：旧式XML（无style元素，renderTemplate="rect"）
    void testXmlBackwardCompat();

    // 连接点样式默认值验证
    void testLinkPointStyleDefaults();

    // QColor hex往返序列化
    void testColorSerialization();
};

}  // namespace DA