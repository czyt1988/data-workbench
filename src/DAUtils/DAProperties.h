#ifndef DAPROPERTIES_H
#define DAPROPERTIES_H
#include <QHash>
#include <QVariant>
#include "DAUtilsAPI.h"

namespace DA
{

/**
 * @brief 参考Java Properties 类封装的属性类，负责SA的属性传递
 *
 *
 */
class DAUTILS_API DAProperties : public QVariantHash
{
public:
    DAProperties() = default;

    //获取属性
    QVariant getProperty(const QString& key) const;
    QVariant getProperty(const QString& key, const QVariant& defaultProperty) const;

    //设置属性
    void setProperty(const QString& key, const QVariant& value);
};

/**
 * @brief 属性组
 *
 */
class DAUTILS_API DAPropertiesGroup : public QHash< QString, DAProperties >
{
public:
    DAPropertiesGroup() = default;
    //获取属性
    QVariant getProperty(const QString& group, const QString& key) const;
    QVariant getProperty(const QString& group, const QString& key, const QVariant& defaultProperty) const;

    //获取一组属性，必须先确保有这个分组
    const DAProperties& constProperties(const QString& group) const;

    //获取一组属性的引用，如果没有，会插入一个默认属性,但对于常量操作不会插入，而是触发断言
    DAProperties& properties(const QString& group);
    const DAProperties& properties(const QString& group) const;

    //获取一组属性
    DAProperties getProperties(const QString& group);

    //设置一组属性
    void setProperties(const QString& group, const DAProperties& propertys);

    //设置属性
    void setProperty(const QString& group, const QString& key, const QVariant& value);

    //判断是否存在分组
    bool hasGroup(const QString& group);
};

}
Q_DECLARE_METATYPE(DA::DAProperties)
Q_DECLARE_METATYPE(DA::DAPropertiesGroup)
#endif  // SAPROPERTIES_H
