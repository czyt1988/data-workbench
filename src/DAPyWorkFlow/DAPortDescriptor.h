#ifndef DAPORTDESCRIPTOR_H
#define DAPORTDESCRIPTOR_H

#include <QString>
#include <QJsonObject>

namespace DA
{

/**
 * @brief 端口描述符结构体
 *
 * 用于描述工作流节点输入/输出端口的元数据，包括端口名称、数据类型、
 * 是否必需以及描述信息。支持 JSON 序列化与反序列化。
 *
 * 使用示例：
 * @code
 * DAPortDescriptor desc("input_data", "DataFrame", true, "输入数据");
 * QJsonObject json = desc.toJson();
 * DAPortDescriptor restored = DAPortDescriptor::fromJson(json);
 * @endcode
 */
struct DAPortDescriptor
{
    QString name;         ///< 端口名称（唯一标识）
    QString dataType;     ///< 数据类型（如 "DataFrame"、"Series"、"int" 等）
    bool required;        ///< 是否为必需端口（默认 true）
    QString description;  ///< 端口描述信息（可选）

    /**
     * @brief 默认构造函数
     *
     * 初始化为默认值：
     * - name: 空字符串
     * - dataType: 空字符串
     * - required: true
     * - description: 空字符串
     */
    DAPortDescriptor() : required(true)
    {
    }

    /**
     * @brief 带参构造函数
     * @param n 端口名称
     * @param dt 数据类型
     * @param req 是否必需（默认 true）
     * @param desc 描述信息（默认空字符串）
     */
    DAPortDescriptor(const QString& n, const QString& dt, bool req = true, const QString& desc = QString())
        : name(n), dataType(dt), required(req), description(desc)
    {
    }

    /**
     * @brief 判断端口描述符是否有效
     * @return true 表示 name 和 dataType 均非空，false 表示无效
     */
    bool isValid() const
    {
        return !name.isEmpty() && !dataType.isEmpty();
    }

    /**
     * @brief 序列化为 QJsonObject
     * @return JSON 对象
     *
     * JSON 键名使用 snake_case 以匹配 Python 侧约定。
     * required 和 description 仅在非默认值时写入，实现稀疏序列化。
     */
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj[QStringLiteral("name")]     = name;
        obj[QStringLiteral("data_type")] = dataType;
        if (!required)
            obj[QStringLiteral("required")] = required;
        if (!description.isEmpty())
            obj[QStringLiteral("description")] = description;
        return obj;
    }

    /**
     * @brief 从 QJsonObject 反序列化
     * @param obj JSON 对象
     * @return DAPortDescriptor 实例
     *
     * 对于可选字段 required，仅在 JSON 中包含该键时才覆盖默认值。
     */
    static DAPortDescriptor fromJson(const QJsonObject& obj)
    {
        DAPortDescriptor desc;
        desc.name     = obj.value(QStringLiteral("name")).toString();
        desc.dataType = obj.value(QStringLiteral("data_type")).toString();
        if (obj.contains(QStringLiteral("required")))
            desc.required = obj.value(QStringLiteral("required")).toBool();
        desc.description = obj.value(QStringLiteral("description")).toString();
        return desc;
    }
};

}  // namespace DA

#endif  // DAPORTDESCRIPTOR_H
