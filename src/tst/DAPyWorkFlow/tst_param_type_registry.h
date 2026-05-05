#pragma once

#include <QObject>
#include <QVariant>
#include <QJsonObject>

namespace DA
{

/**
 * @brief DAParamTypeRegistry 参数类型注册系统的单元测试
 *
 * 验证 11 种内置类型的编辑器创建、未知类型处理、
 * supportedTypes 列表、默认值设置、枚举值填充以及重新注册覆盖。
 */
class TestDAParamTypeRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 初始化和清理
    void initTestCase();

    // 11 种内置类型的编辑器创建与类型验证
    void test_registerAndCreate_all11Types();

    // 未知类型返回 nullptr
    void test_unknownType_returnsNull();

    // supportedTypes() 返回完整列表
    void test_supportedTypes();

    // 各类型的默认值设置
    void test_defaultValues();

    // enum 类型从 descriptor 读取 enum_values 和 default
    void test_enumValues();

    // 重新注册同一类型会覆盖旧的 creator
    void test_reRegistration_overwrites();

private:
    // 辅助方法：创建参数描述符 JSON 对象
    QJsonObject makeDescriptor(const QString& type,
                               const QString& description = QString(),
                               const QVariant& defaultValue = QVariant(),
                               const QJsonObject& extensions = QJsonObject()) const;
};

}  // namespace DA