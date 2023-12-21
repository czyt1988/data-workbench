#include "DAGraphicsItemFactory.h"
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include "DAGraphicsItem.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsRectItem.h"
#include "DAGraphicsTextItem.h"
#include "DAGraphicsStandardTextItem.h"
namespace DA
{
/**
 * @brief 存放已经注册的item
 */
QHash< QString, DAGraphicsItemFactory::FpItemCreate >* g_pRegistedItems = nullptr;
QHash< int, DAGraphicsItemFactory::FpItemCreate >* g_pRegistedItems2    = nullptr;

DAGraphicsItemFactory::DAGraphicsItemFactory()
{
}

DAGraphicsItemFactory::~DAGraphicsItemFactory()
{
}

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
}

/**
 * @brief 向工厂注册
 * @param className
 * @param fp
 */
void DAGraphicsItemFactory::registItem(const QString& className, DAGraphicsItemFactory::FpItemCreate fp)
{
    if (g_pRegistedItems == nullptr) {
        g_pRegistedItems = new QHash< QString, DAGraphicsItemFactory::FpItemCreate >();
    }
    if (g_pRegistedItems2 == nullptr) {
        g_pRegistedItems2 = new QHash< int, DAGraphicsItemFactory::FpItemCreate >();
    }
    std::unique_ptr< QGraphicsItem > it(fp());
    g_pRegistedItems->operator[](className)   = fp;
    g_pRegistedItems2->operator[](it->type()) = fp;
}

QGraphicsItem* DAGraphicsItemFactory::createItem(const QString& className)
{
    if (g_pRegistedItems == nullptr) {
        return nullptr;
    }
    FpItemCreate fp = g_pRegistedItems->value(className, nullptr);
    if (nullptr == fp) {
        qWarning() << QObject::tr("Class name %1 not registered to item factory").arg(className);
        return nullptr;
    }
    return fp();
}

QGraphicsItem* DAGraphicsItemFactory::createItem(int itemType)
{
    if (g_pRegistedItems2 == nullptr) {
        return nullptr;
    }
    FpItemCreate fp = g_pRegistedItems2->value(itemType, nullptr);
    if (nullptr == fp) {
        qWarning() << QObject::tr("type %1 not registered to item factory").arg(itemType);
        return nullptr;
    }
    return fp();
}

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
