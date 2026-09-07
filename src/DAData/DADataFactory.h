#ifndef DADATAFACTORY_H
#define DADATAFACTORY_H
#include <QString>
#include <QStringList>
#include <functional>
#include "DADataAPI.h"
#include "DAAbstractData.h"
namespace DA
{
/**
 * @brief DAAbstractData的工厂注册表
 *
 * 以类型标识字符串（DAAbstractData::typeIdentifier）为键登记创建函数，
 * 工程加载时据此从类型标识重建数据对象：工厂创建实例后经
 * DAAbstractData::read(QDataStream) 恢复引用内容（数据库惰性表等引用式数据），
 * 数据本体不进入工程文件。
 *
 * 插件（如数据库管理插件）应在初始化阶段调用 @ref registerCreator 登记自有类型，
 * 卸载阶段调用 @ref unregisterCreator。注册表内部互斥保护，可在任意线程注册。
 */
class DADATA_API DADataFactory
{
public:
    using Creator = std::function< DAAbstractData::Pointer() >;

public:
    // 注册创建函数，同一typeIdentifier重复注册会覆盖旧实现
    static bool registerCreator(const QString& typeIdentifier, Creator creator);
    // 注销创建函数
    static void unregisterCreator(const QString& typeIdentifier);
    // 是否已注册
    static bool contains(const QString& typeIdentifier);
    // 创建实例，未注册或创建失败返回空指针
    static DAAbstractData::Pointer create(const QString& typeIdentifier);
    // 所有已注册的类型标识
    static QStringList registeredTypeIdentifiers();
};
}  // namespace DA
#endif  // DADATAFACTORY_H
