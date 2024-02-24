#ifndef DAGRAPHICSITEMFACTORY_H
#define DAGRAPHICSITEMFACTORY_H
#include <QHash>
#include "DAGraphicsViewGlobal.h"
#include <functional>
#include "DAGraphicsItem.h"
namespace DA
{
/**
 * @brief DAGraphicsItem的工厂类，工厂类负责DAGraphicsItem的反射工作
 *
 * 所有自定义的item需要向工厂注册才可以实现加载
 */
class DAGRAPHICSVIEW_API DAGraphicsItemFactory
{
public:
    using FpItemCreate = std::function< QGraphicsItem*() >;

public:
    DAGraphicsItemFactory();
    virtual ~DAGraphicsItemFactory();
    // 工厂初始化，此初始化会注册DAGraphicsView库里面的元件
    static void initialization();
    // 注册
    static void registItem(const QString& className, FpItemCreate fp);
    // 创建item
    static QGraphicsItem* createItem(const QString& className);
    static QGraphicsItem* createItem(int itemType);
    // 销毁item
    static void destoryItem(DAGraphicsItem* i);
    // 生成一个id,rand建议传入对应指针的截断值
    static uint64_t generateID(uint32_t rand);
};
}

#endif  // DAGRAPHICSITEMFACTORY_H
