#ifndef DAPORTDESCRIPTOR_H
#define DAPORTDESCRIPTOR_H

#include <QString>

namespace DA
{

/**
 * @brief 端口描述符结构体
 *
 * 用于描述工作流节点输入/输出端口的元数据，包括端口名称、数据类型、
 * 是否必需以及描述信息。
 *
 * 序列化由上层调用方直接操作字段完成（见 DANodeDescriptor::toJson/fromJson）。
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

};

}  // namespace DA

#endif  // DAPORTDESCRIPTOR_H
