#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief DAPyNodeMetaData 元数据结构的冒烟测试
 *
 * 验证基础构造、字段访问、有效性判断和比较运算符。
 */
class TestDAPyNodeMetaData : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 默认构造测试
    void testDefaultConstruction();

    // 字段赋值与访问测试
    void testFieldAssignment();

    // isValid() 有效性判断测试
    void testIsValid();

    // bool 转换运算符测试
    void testBoolConversion();

    // 比较运算符测试
    void testComparisonOperators();

    // 兼容 getter 方法测试
    void testGetterMethods();

    // getIcon() 空路径测试
    void testGetIconEmptyPath();

    // XML 序列化向后兼容测试 - 新格式 (qualifiedName)
    void testXmlNewFormatQualifiedname();

    // XML 序列化向后兼容测试 - 旧格式 (prototype)
    void testXmlOldPrototypeBackwardCompat();
};

}  // namespace DA
