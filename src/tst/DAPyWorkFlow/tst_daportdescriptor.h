#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DAPortDescriptor JSON序列化/验证测试
 *
 * 验证端口描述符结构体的默认构造、字段访问、
 * 有效性验证及JSON往返序列化。
 */
class TestDAPortDescriptor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 默认构造函数验证所有字段为预期默认值
    void testDefaultConstruction();

    // 字段访问：设置值后验证各字段正确返回
    void testFieldAccess();

    // 有效性验证：仅当name和dataType非空时返回true
    void testIsValid();

    // JSON序列化往返：创建→toJson→fromJson→验证值一致
    void testJsonRoundTrip();
};

}  // namespace DA
