#ifndef DAZIPARCHIVETASK_CHARTITEM_H
#define DAZIPARCHIVETASK_CHARTITEM_H
#include "DAGuiAPI.h"
#include "DAAbstractArchiveTask.h"
#include "DAChartItemsManager.h"
namespace DA
{
class DAZipArchive;

/**
 * @brief 把QwtPlotItem写zip命令
 */
class DAGUI_API DAZipArchiveTask_ChartItem : public DAAbstractArchiveTask
{
public:
    // 保存构造
    DAZipArchiveTask_ChartItem(const DAChartItemsManager& items);
    // 加载构造
    DAZipArchiveTask_ChartItem();
    ~DAZipArchiveTask_ChartItem();
    // 获取item
    const DAChartItemsManager& getItems() const;
    //
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;
    // 序列化
    static QByteArray qwtitemSerialization(const QwtPlotItem* item);

private:
    QByteArray toXml() const;
    bool writeChartItems(DAZipArchive* zip, const DAChartItemsManager* itemsData);
    bool readChartItems(DAZipArchive* zip, DAChartItemsManager* itemsData);

private:
    DAChartItemsManager mItems;
};
}
#endif  // DAZIPARCHIVETASK_CHARTITEM_H
