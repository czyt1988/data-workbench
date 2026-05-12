#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DANodeDescriptor 结构体测试
 *
 * 验证节点描述符的默认构造、字段访问、有效性判断、
 * toMetaData 转换、JSON 往返序列化及 JSON 键名与旧格式匹配。
 */
class TestDANodeDescriptor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 默认构造函数验证所有字段为预期默认值
    void testDefaultConstruction();

    // 字段访问：设置值后验证各字段正确返回
    void testFieldAccess();

    // 有效性验证：仅当 qualifiedName 非空时返回 true
    void testIsValid();

    // toMetaData 转换：字段映射到 DAPyNodeMetaData 对应字段
    void testToMetaData();
};

}  // namespace DA
