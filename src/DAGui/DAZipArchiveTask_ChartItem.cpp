#include "DAZipArchiveTask_ChartItem.h"
#include "DAZipArchive.h"
#include <QDebug>
#include <QDomDocument>
#include "DAGuiEnumStringUtils.h"
#include "qwt_plot_item.h"
#include "DAChartSerialize.h"
#include "qwt_plot_curve.h"
namespace DA
{
DAZipArchiveTask_ChartItem::DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath, const DAChartItemsManager& items)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath), mItems(items)
{
}

DAZipArchiveTask_ChartItem::DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath)
{
}

DAZipArchiveTask_ChartItem::~DAZipArchiveTask_ChartItem()
{
}

const DAChartItemsManager& DAZipArchiveTask_ChartItem::getItems() const
{
	return mItems;
}

bool DAZipArchiveTask_ChartItem::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
{
	if (!archive) {
		return false;
	}
	DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
	if (mode == DAAbstractArchiveTask::WriteMode) {
		// 写模式
		return writeChartItems(zip, &mItems);
	} else {
		// 读取数据模式
		return readChartItems(zip, &mItems);
	}
	return true;
}

/**
 * @brief 序列化
 * @param item
 * @return
 */
QByteArray DAZipArchiveTask_ChartItem::qwtitemSerialization(const QwtPlotItem* item)
{
	QByteArray data;
	QDataStream st(&data, QIODevice::ReadWrite);
	st << item;
	switch (item->rtti()) {
	case QwtPlotItem::Rtti_PlotItem: {
		const QwtPlotCurve* c = static_cast< const QwtPlotCurve* >(item);
		st << c;
	} break;
	case QwtPlotItem::Rtti_PlotGrid:
		break;
	case QwtPlotItem::Rtti_PlotScale:
		break;
	case QwtPlotItem::Rtti_PlotLegend:
		break;
	case QwtPlotItem::Rtti_PlotMarker:
		break;
	case QwtPlotItem::Rtti_PlotCurve:
		break;
	case QwtPlotItem::Rtti_PlotSpectroCurve:
		break;
	case QwtPlotItem::Rtti_PlotIntervalCurve:
		break;
	case QwtPlotItem::Rtti_PlotHistogram:
		break;
	case QwtPlotItem::Rtti_PlotSpectrogram:
		break;
	case QwtPlotItem::Rtti_PlotGraphic:
		break;
	case QwtPlotItem::Rtti_PlotTradingCurve:
		break;
	case QwtPlotItem::Rtti_PlotBarChart:
		break;
	case QwtPlotItem::Rtti_PlotMultiBarChart:
		break;
	case QwtPlotItem::Rtti_PlotShape:
		break;
	case QwtPlotItem::Rtti_PlotTextLabel:
		break;
	case QwtPlotItem::Rtti_PlotZone:
		break;
	case QwtPlotItem::Rtti_PlotVectorField:
		break;
	case QwtPlotItem::Rtti_PlotUserItem:
		break;
	}
	return data;
}

/**
 * @brief 把基本信息转换为xml
 * @return
 */
QByteArray DAZipArchiveTask_ChartItem::toXml() const
{
	QDomDocument doc("chart-items");
	QDomElement root = doc.createElement("items");
	doc.appendChild(root);
	const QList< QwtPlotItem* > items = mItems.items();
	for (QwtPlotItem* item : items) {
		QString key = mItems.itemToKey(item);
		if (key.isEmpty()) {
			qWarning() << "plot item cannot find id";
			continue;
		}
		QDomElement itemEle = doc.createElement(QStringLiteral("item"));
		itemEle.setAttribute(QStringLiteral("key"), key);
		itemEle.setAttribute(QStringLiteral("rtti"), item->rtti());
		root.appendChild(itemEle);
	}
	return doc.toByteArray();
}

bool DAZipArchiveTask_ChartItem::writeChartItems(DAZipArchive* zip, const DAChartItemsManager* itemsData)
{
	//! 1.先把key和type信息保存到xml
	QByteArray itemsInfoXmlData = toXml();
	if (itemsInfoXmlData.isEmpty()) {
		qDebug() << QString("chart items manager get null xml info");
		return false;
	}
	if (!zip->write(QString("%1/%2").arg(mZipRelateFolderPath, chartItemsMgrXmlFileName()), itemsInfoXmlData)) {
		qWarning() << QString("write %1/%2 error").arg(mZipRelateFolderPath, chartItemsMgrXmlFileName());
		return false;
	}
	//! 2.写绘图的数据到文件中
	const QList< QwtPlotItem* > items = itemsData->items();
	for (QwtPlotItem* item : items) {
		QString key = itemsData->itemToKey(item);
		if (key.isEmpty()) {
			qWarning() << "plot item cannot find id";
			continue;
		}
		QByteArray byteData = qwtitemSerialization(item);
		// 写文件
		QString writeBytePath = QString("%1/%2").arg(mZipRelateFolderPath, key);
		if (!zip->write(writeBytePath, byteData)) {
			qWarning() << QString("write %1 error").arg(writeBytePath);
			continue;
		}
	}
	return true;
}

bool DAZipArchiveTask_ChartItem::readChartItems(DAZipArchive* zip, DAChartItemsManager* itemsData)
{
	return false;
}

QString DAZipArchiveTask_ChartItem::chartItemsMgrXmlFileName()
{
	const static QString s_chartItemsMgrXmlFileName = QStringLiteral("chart-items.xml");
	return s_chartItemsMgrXmlFileName;
}
}  // end namespace DA
