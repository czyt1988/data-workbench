#ifndef DACHARTITEMSMANAGER_H
#define DACHARTITEMSMANAGER_H
#include <QHash>
#include "qwt_plot_item.h"
#include "DAGuiAPI.h"
namespace DA
{

/**
 * @brief 这个类用于管理和记录整个绘图的所有QwtPlotItem,主要用于持久化的时候，记录QwtPlotItem和key的对应关系
 *
 * QwtPlotItem内容不保存到xml中，因此，在生成绘图的xml里，仅仅保存了item对应的key，保存xml后，item对应的key都在此类里面，把此类给对应的写数据线程，进行QwtPlotItem的数据保存，这样可以使QwtPlotItem的保存在线程中进行
 *
 * 同理，读取的时候，先要读取PlotItemsDataInfo，里面把所有QwtPlotItem都先读取出来，再读取xml，这样xml获取QwtPlotItem直接通过key来查询PlotItemsDataInfo即可
 */
class DAGUI_API DAChartItemsManager
{
public:
    DAChartItemsManager();
    ~DAChartItemsManager();
    // 记录
    QString recordItem(QwtPlotItem* item);
    void recordItem(QwtPlotItem* item, const QString& key);
    QString itemToKey(QwtPlotItem* item) const;
    QwtPlotItem* keyToItem(const QString& key) const;
    // 生成一个唯一的key
    QString makeItemKey(QwtPlotItem* item);

private:
    QHash< QwtPlotItem*, QString > mItemToKey;
    QHash< QString, QwtPlotItem* > mKeyToItem;
    int mKeyID { 0 };
};
}

#endif  // DACHARTITEMSMANAGER_H
