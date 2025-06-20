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
    DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath, const DAChartItemsManager& itemsMgr);
	// 加载构造
	DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath);
	~DAZipArchiveTask_ChartItem();
	// 获取item
    const DAChartItemsManager& getChartItemsManager() const;
	//
	virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;
	// 序列化
	static QByteArray qwtitemSerialization(const QwtPlotItem* item);
    static QwtPlotItem* qwtitemSerialization(const QByteArray& byte);
    QString getLastError() const;
    void setLastError(const QString& lastError);

private:
    QByteArray toXml() const;
    bool fromXml(DAZipArchive* zip, QList< QPair< QString, int > >& res);
    bool writeChartItems(DAZipArchive* zip, const DAChartItemsManager* itemsData);
	bool readChartItems(DAZipArchive* zip, DAChartItemsManager* itemsData);
	static QString chartItemsMgrXmlFileName();

private:
	QString mZipRelateFolderPath;
    DAChartItemsManager mItemsManager;
    QString mLastError;
};
}
#endif  // DAZIPARCHIVETASK_CHARTITEM_H
