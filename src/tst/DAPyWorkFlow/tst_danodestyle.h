#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DANodeStyle 及 DAPyLinkPointStyle 字段读写与默认值测试
 *
 * 验证节点样式结构体的默认构造、字段赋值读写、颜色属性往返、
 * 端口样式赋值、setDefaults重置、XML持久化、向后兼容、连接点样式默认值。
 */
class TestDANodeStyle : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 默认构造函数验证所有字段为预期默认值
    void testDefaultConstruction();

    // 字段赋值读写：逐字段修改并读回验证
    void testFieldAssignment();

    // 颜色属性往返：backgroundColor/borderColor赋值与读回
    void testColorPropertyRoundTrip();

    // 端口样式字段赋值：inputPortStyle/outputPortStyle读写
    void testPortStyleAssignment();

    // setDefaults重置：已修改字段调用setDefaults后恢复默认
    void testSetDefaultsReset();

    // XML持久化往返：通过DAPyNodeGraphicsItem save/load
    void testXmlRoundTrip();

    // XML向后兼容：旧式XML（无style元素，renderTemplate="rect"）
    void testXmlBackwardCompat();

    // 连接点样式默认值验证
    void testLinkPointStyleDefaults();
};

}  // namespace DA