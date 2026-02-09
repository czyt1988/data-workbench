#include "DAChartItemsManager.h"
#include "DAGuiEnumStringUtils.h"

namespace DA
{
DAChartItemsManager::DAChartItemsManager()
{
}

DAChartItemsManager::~DAChartItemsManager()
{
}

QString DAChartItemsManager::recordItem(QwtPlotItem* item)
{
    QString key = makeItemKey(item);
    recordItem(item, key);
    return key;
}

void DAChartItemsManager::recordItem(QwtPlotItem* item, const QString& key)
{
    mItemToKey[ item ] = key;
    mKeyToItem[ key ]  = item;
}

QString DAChartItemsManager::itemToKey(QwtPlotItem* item) const
{
    return mItemToKey.value(item, QString());
}

QwtPlotItem* DAChartItemsManager::keyToItem(const QString& key) const
{
    return mKeyToItem.value(key, nullptr);
}

QString DAChartItemsManager::makeItemKey(QwtPlotItem* item)
{
    ++mKeyID;
    QString key = enumToString(static_cast< QwtPlotItem::RttiValues >(item->rtti()));
    return QString("%1-%2").arg(key).arg(mKeyID);
}

bool DAChartItemsManager::isEmpty() const
{
    return mItemToKey.isEmpty();
}

QList< QString > DAChartItemsManager::keys() const
{
    return mKeyToItem.keys();
}

QList< QwtPlotItem* > DAChartItemsManager::items() const
{
    return mItemToKey.keys();
}

}
