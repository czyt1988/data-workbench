#include "DAGraphicsItemFactory.h"
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include "DAGraphicsItem.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsRectItem.h"
#include "DAGraphicsTextItem.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAGraphicsLabelItem.h"
#include "DALogCategory.h"
namespace DA
{
/**
 * @brief 存放已经注册的item（按类名索引）
 * @return
 */
static QHash< QString, DAGraphicsItemFactory::FpItemCreate >& registedItemsByName()
{
    static QHash< QString, DAGraphicsItemFactory::FpItemCreate > s;
    return s;
}

/**
 * @brief 存放已经注册的item（按type索引）
 * @return
 */
static QHash< int, DAGraphicsItemFactory::FpItemCreate >& registedItemsByType()
{
    static QHash< int, DAGraphicsItemFactory::FpItemCreate > s;
    return s;
}

/**
 * @brief 构造函数
 */
DAGraphicsItemFactory::DAGraphicsItemFactory()
{
}

/**
 * @brief 析构函数
 */
DAGraphicsItemFactory::~DAGraphicsItemFactory()
{
}

/**
 * @brief 初始化工厂，注册内置的图形项
 */
void DAGraphicsItemFactory::initialization()
{
    // Qt

    // DA
    registItem(DAGraphicsPixmapItem::staticMetaObject.className(),
               []() -> QGraphicsItem* { return new DAGraphicsPixmapItem(); });
    registItem(DAGraphicsRectItem::staticMetaObject.className(),
               []() -> QGraphicsItem* { return new DAGraphicsRectItem(); });
    registItem(DAGraphicsTextItem::staticMetaObject.className(),
               []() -> QGraphicsItem* { return new DAGraphicsTextItem(); });
    // Other
    registItem("DA::DAGraphicsStandardTextItem", []() -> QGraphicsItem* { return new DAGraphicsStandardTextItem(); });
    registItem("DA::DAGraphicsLabelItem", []() -> QGraphicsItem* { return new DAGraphicsLabelItem(); });
}

/**
 * @brief 向工厂注册
 * @param className
 * @param fp
 */
void DAGraphicsItemFactory::registItem(const QString& className, DAGraphicsItemFactory::FpItemCreate fp)
{
    std::unique_ptr< QGraphicsItem > it(fp());
    registedItemsByName()[ className ] = fp;
    registedItemsByType()[ it->type() ] = fp;
}

/**
 * @brief 根据类名创建图形项
 * @param className 类名
 * @return 创建的图形项指针，若类名未注册则返回nullptr
 */
QGraphicsItem* DAGraphicsItemFactory::createItem(const QString& className)
{
    FpItemCreate fp = registedItemsByName().value(className, nullptr);
    if (nullptr == fp) {
        daWarning << QObject::tr("Class name %1 not registered to item factory").arg(className);  // cn:类名 %1 未注册到 item 工厂
        return nullptr;
    }
    return fp();
}

/**
 * @brief 根据类型创建图形项
 * @param itemType 类型枚举值
 * @return 创建的图形项指针，若类型未注册则返回nullptr
 */
QGraphicsItem* DAGraphicsItemFactory::createItem(int itemType)
{
    FpItemCreate fp = registedItemsByType().value(itemType, nullptr);
    if (nullptr == fp) {
        daWarning << QObject::tr("Type %1 not registered to item factory").arg(itemType);  // cn:类型 %1 未注册到 item 工厂
        return nullptr;
    }
    return fp();
}

/**
 * @brief 销毁图形项
 * @param i 要销毁的图形项指针
 */
void DAGraphicsItemFactory::destoryItem(DAGraphicsItem* i)
{
    delete i;
}

/**
   @brief 生成一个id
   @return
 */
uint64_t DAGraphicsItemFactory::generateID(uint32_t rand)
{
    union {
        uint64_t id;
        uint32_t raw[ 2 ];
    } mem;
    QDateTime dt = QDateTime::currentDateTime();

    mem.id       = uint64_t(dt.toMSecsSinceEpoch());
    mem.raw[ 1 ] = rand;
    return mem.id;
}

}  // end DA
