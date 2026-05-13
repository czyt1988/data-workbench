#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DAPortDescriptor 字段读写与 pybind11 构造测试
 *
 * 验证端口描述符结构体的默认构造、字段访问、
 * 有效性验证、相等性比较及 pybind11 构造。
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

    // 相等性比较：两个相同字段值的描述符逐字段相等
    void testEqualityComparison();

    // pybind11 构造：通过 Python 创建 DAPortDescriptor 并验证字段
    void testPybind11Construction();
};

}  // namespace DA