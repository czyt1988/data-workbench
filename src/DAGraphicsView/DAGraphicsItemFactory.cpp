#include "DAGraphicsItemFactory.h"
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include "DAGraphicsItem.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsRectItem.h"
#include "DAGraphicsTextItem.h"
namespace DA
{
/**
 * @brief 存放已经注册的item
 */
QHash< QString, DAGraphicsItemFactory::FpCreate >* g_pRegistedItems = nullptr;

DAGraphicsItemFactory::DAGraphicsItemFactory()
{
}

DAGraphicsItemFactory::~DAGraphicsItemFactory()
{
}

void DAGraphicsItemFactory::initialization()
{
    registItem(DAGraphicsPixmapItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsPixmapItem(); });
    registItem(DAGraphicsRectItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsRectItem(); });
    registItem(DAGraphicsTextItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsTextItem(); });
}

/**
 * @brief 向工厂注册
 * @param className
 * @param fp
 */
void DAGraphicsItemFactory::registItem(const QString& className, DAGraphicsItemFactory::FpCreate fp)
{
    if (g_pRegistedItems == nullptr) {
        g_pRegistedItems = new QHash< QString, DAGraphicsItemFactory::FpCreate >();
    }
    g_pRegistedItems->insert(className, fp);
}

DAGraphicsItem* DAGraphicsItemFactory::createItem(const QString& className)
{
    if (g_pRegistedItems == nullptr) {
        return nullptr;
    }
    FpCreate fp = g_pRegistedItems->value(className, nullptr);
    if (nullptr == fp) {
        qWarning() << QObject::tr("Class name %1 not registered to item factory").arg(className);
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
