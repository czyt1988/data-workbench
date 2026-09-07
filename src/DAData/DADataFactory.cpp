#include "DADataFactory.h"
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
namespace DA
{

namespace
{
// 创建函数注册表（函数级静态，避免全局构造顺序问题）
QHash< QString, DADataFactory::Creator >& s_creatorMap()
{
    static QHash< QString, DADataFactory::Creator > s_map;
    return s_map;
}

QMutex& s_factoryMutex()
{
    static QMutex s_mutex;
    return s_mutex;
}
}  // namespace

//===================================================
// DADataFactory
//===================================================

/**
 * @brief 注册创建函数
 * @param typeIdentifier 类型标识，需与数据类的typeIdentifier()返回值一致
 * @param creator 创建函数
 * @return typeIdentifier为空或creator为空返回false，重复注册覆盖旧实现并返回true
 */
bool DADataFactory::registerCreator(const QString& typeIdentifier, Creator creator)
{
    if (typeIdentifier.isEmpty() || !creator) {
        return false;
    }
    QMutexLocker locker(&s_factoryMutex());
    s_creatorMap()[ typeIdentifier ] = creator;
    return true;
}

/**
 * @brief 注销创建函数
 * @param typeIdentifier 类型标识
 */
void DADataFactory::unregisterCreator(const QString& typeIdentifier)
{
    QMutexLocker locker(&s_factoryMutex());
    s_creatorMap().remove(typeIdentifier);
}

/**
 * @brief 是否已注册
 * @param typeIdentifier 类型标识
 * @return 已注册返回true
 */
bool DADataFactory::contains(const QString& typeIdentifier)
{
    QMutexLocker locker(&s_factoryMutex());
    return s_creatorMap().contains(typeIdentifier);
}

/**
 * @brief 创建实例
 * @param typeIdentifier 类型标识
 * @return 未注册或创建失败返回空指针
 */
DAAbstractData::Pointer DADataFactory::create(const QString& typeIdentifier)
{
    Creator creator;
    {
        QMutexLocker locker(&s_factoryMutex());
        creator = s_creatorMap().value(typeIdentifier);
    }
    if (!creator) {
        return DAAbstractData::Pointer();
    }
    return creator();
}

/**
 * @brief 所有已注册的类型标识
 * @return 类型标识列表
 */
QStringList DADataFactory::registeredTypeIdentifiers()
{
    QMutexLocker locker(&s_factoryMutex());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return QStringList(s_creatorMap().keys());
#else
    return s_creatorMap().keys();
#endif
}

}  // namespace DA
