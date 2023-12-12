#include "DAGraphicsItemFactory.h"
#include <QDebug>
#include <QObject>
#include "DAGraphicsItem.h"
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAGraphicsResizeableRectItem.h"
#include "DAGraphicsResizeableTextItem.h"
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
    registItem(DAGraphicsResizeablePixmapItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsResizeablePixmapItem(); });
    registItem(DAGraphicsResizeableRectItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsResizeableRectItem(); });
    registItem(DAGraphicsResizeableTextItem::staticMetaObject.className(),
               []() -> DAGraphicsItem* { return new DAGraphicsResizeableTextItem(); });
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

}  // end DA
