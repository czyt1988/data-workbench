#ifndef PARAMETERDESCRIPTOR_H
#define PARAMETERDESCRIPTOR_H

#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

namespace DA
{

/**
 * @brief Python 节点参数描述符
 *
 * 轻量级数据结构，用于解析并存储 Python 节点的参数描述信息（从 JSON 解析）。
 * 包含参数名称、类型、描述、默认值等基础字段，并保留原始 JSON 描述符供扩展访问。
 *
 * Descriptor JSON 格式示例:
 * @code
 * {
 *   "name": "threshold",
 *   "type": "float",
 *   "description": "筛选阈值",
 *   "default": 0.5
 * }
 * @endcode
 *
 * @note 此为纯数据头文件结构体，无需 .cpp 实现文件。
 * @see DAPyParamTypeHelper
 */
struct ParameterDescriptor
{
    QString name;           ///< 参数名称
    QString type;           ///< 参数类型 (str/int/float/bool/list/dict)
    QString description;    ///< 参数描述
    QVariant defaultValue;  ///< 默认值（可为无效 QVariant 表示无默认值）
    QJsonObject rawDescriptor;  ///< 原始 JSON 描述符（保留所有字段供扩展访问）
    int propertyId;         ///< 属性面板中的属性 ID（由面板构建器设置）

    /**
     * @brief 默认构造函数
     *
     * 初始化 propertyId 为 0，其余字段为空。
     */
    ParameterDescriptor()
        : propertyId(0)
    {
    }

    /**
     * @brief 从单个 JSON 对象构造参数描述符
     *
     * @param[in] obj 包含参数描述信息的 JSON 对象
     * @return 构造完成的 ParameterDescriptor
     */
    static ParameterDescriptor fromJson(const QJsonObject& obj)
    {
        ParameterDescriptor desc;
        desc.name = obj.value(QStringLiteral("name")).toString();
        desc.type = obj.value(QStringLiteral("type")).toString();
        desc.description = obj.value(QStringLiteral("description")).toString();

        QJsonValue defaultVal = obj.value(QStringLiteral("default"));
        if (!defaultVal.isNull()) {
            desc.defaultValue = defaultVal.toVariant();
        }

        desc.rawDescriptor = obj;
        desc.propertyId = 0;
        return desc;
    }

    /**
     * @brief 从 JSON 数组批量解析参数描述符
     *
     * 遍历数组中的每个 JSON 对象，若对象缺少 "name" 或 "type" 字段则跳过。
     *
     * @param[in] params JSON 数组，包含多个参数描述对象
     * @return 解析成功的 ParameterDescriptor 列表
     */
    static QVector<ParameterDescriptor> fromJsonArray(const QJsonArray& params)
    {
        QVector<ParameterDescriptor> result;
        for (const QJsonValue& val : params) {
            QJsonObject obj = val.toObject();
            // 跳过缺少 "name" 或 "type" 的无效条目
            if (!obj.contains(QStringLiteral("name")) || !obj.contains(QStringLiteral("type"))) {
                continue;
            }
            result.append(fromJson(obj));
        }
        return result;
    }

    /**
     * @brief 检查原始描述符中是否包含指定字段
     *
     * @param[in] fieldName 字段名称
     * @return 包含该字段返回 true
     */
    bool hasField(const QString& fieldName) const
    {
        return rawDescriptor.contains(fieldName);
    }

    /**
     * @brief 获取原始描述符中指定字段的值
     *
     * @param[in] fieldName 字段名称
     * @return 字段对应的 JSON 值，不存在返回无效 QJsonValue
     */
    QJsonValue getField(const QString& fieldName) const
    {
        return rawDescriptor.value(fieldName);
    }
};

}  // namespace DA

#endif  // PARAMETERDESCRIPTOR_H
