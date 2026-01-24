#include "DAZipArchiveTask_ChartItem.h"
#include "DAZipArchive.h"
#include <QDebug>
#include <QDomDocument>
#include "DAGuiEnumStringUtils.h"
#include "qwt_plot_item.h"
#include "DAChartSerialize.h"
#include <QCoreApplication>

#ifndef TR_
#define TR_(TEXT) QCoreApplication::translate("DAZipArchiveTask_ChartItem", TEXT)
#endif
namespace DA
{
DAZipArchiveTask_ChartItem::DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath, const DAChartItemsManager& itemsMgr)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath), mItemsManager(itemsMgr)
{
}

DAZipArchiveTask_ChartItem::DAZipArchiveTask_ChartItem(const QString& zipRelateFolderPath)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath)
{
}

DAZipArchiveTask_ChartItem::~DAZipArchiveTask_ChartItem()
{
}

const DAChartItemsManager& DAZipArchiveTask_ChartItem::getChartItemsManager() const
{
    return mItemsManager;
}

bool DAZipArchiveTask_ChartItem::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
{
	if (!archive) {
		return false;
	}
	DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
	if (mode == DAAbstractArchiveTask::WriteMode) {
		// 写模式
        return writeChartItems(zip, &mItemsManager);
	} else {
		// 读取数据模式
        return readChartItems(zip, &mItemsManager);
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
    DAChartItemSerialize st;
    return st.serializeOut(item);
}

QwtPlotItem* DAZipArchiveTask_ChartItem::qwtitemSerialization(const QByteArray& byte)
{
    DAChartItemSerialize st;
    return st.serializeIn(byte);
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
    const QList< QwtPlotItem* > items = mItemsManager.items();
	for (QwtPlotItem* item : items) {
        QString key = mItemsManager.itemToKey(item);
		if (key.isEmpty()) {
            qDebug() << "plot item cannot find id";  // cn:plotitem 无法找到id
			continue;
		}
		QDomElement itemEle = doc.createElement(QStringLiteral("item"));
		itemEle.setAttribute(QStringLiteral("key"), key);
		itemEle.setAttribute(QStringLiteral("rtti"), item->rtti());
		root.appendChild(itemEle);
	}
    return doc.toByteArray();
}

bool DAZipArchiveTask_ChartItem::fromXml(DAZipArchive* zip, QList< QPair< QString, int > >& res)
{
    const QString chartItemXmlPath = mZipRelateFolderPath + "/" + chartItemsMgrXmlFileName();
    if (!zip->isOpened()) {
        if (!zip->open()) {
            setLastError(TR_("open archive error:%1(task code:%2)").arg(zip->getBaseFilePath()).arg(getCode()));  // cn:打开存档%1发生异常(任务id%2)
            return false;
        }
    }
    if (!zip->contains(chartItemXmlPath)) {
        // 没有这个文件也返回true,说明是空的绘图或者旧版本
        setLastError(TR_("The archive is missing %1.(task code:%2)").arg(chartItemXmlPath).arg(getCode()));  // cn:存档文件中缺失%1。(任务id%2)
        return true;
    }
    QByteArray data = zip->read(chartItemXmlPath);
    if (data.isEmpty()) {
        setLastError(
            TR_("can not read %1 from %2(task code:%3)").arg(chartItemXmlPath, zip->getBaseFilePath()).arg(getCode()));  // cn:无法在%2中提取%1(任务id%3)
        return false;
    }
    QDomDocument chartItemDoc;
    if (!chartItemDoc.setContent(data, &mLastError)) {
        qDebug() << QString("can not read %1 from %2,last error string is %3(task code:%4)")
                        .arg(chartItemXmlPath, zip->getBaseFilePath(), mLastError)
                        .arg(getCode());
        return false;
    }
    auto root       = chartItemDoc.documentElement();
    auto childNodes = root.childNodes();
    for (int i = 0; i < childNodes.size(); ++i) {
        QDomElement itemEle = childNodes.at(i).toElement();
        if (itemEle.isNull()) {
            continue;
        }
        bool isok   = false;
        QString key = itemEle.attribute(QStringLiteral("key"));
        int rtti    = itemEle.attribute(QStringLiteral("rtti")).toInt(&isok);
        if (isok) {
            res.append(qMakePair(key, rtti));
        }
    }
    return true;
}

bool DAZipArchiveTask_ChartItem::writeChartItems(DAZipArchive* zip, const DAChartItemsManager* itemsData)
{
	//! 1.先把key和type信息保存到xml
	QByteArray itemsInfoXmlData = toXml();
	if (itemsInfoXmlData.isEmpty()) {
        setLastError(TR_("Failed to generate description information(task code %1)").arg(getCode()));  // cn:生成描述信息异常
		return false;
	}
	if (!zip->write(QString("%1/%2").arg(mZipRelateFolderPath, chartItemsMgrXmlFileName()), itemsInfoXmlData)) {
        setLastError(TR_("An error occurred while writing %1/%2").arg(mZipRelateFolderPath, chartItemsMgrXmlFileName()));  // cn:写%1/%2异常
		return false;
	}
	//! 2.写绘图的数据到文件中
	const QList< QwtPlotItem* > items = itemsData->items();
	for (QwtPlotItem* item : items) {
		QString key = itemsData->itemToKey(item);
		if (key.isEmpty()) {
            setLastError(TR_("plot item cannot find id"));
			continue;
		}
		QByteArray byteData = qwtitemSerialization(item);
		// 写文件
		QString writeBytePath = QString("%1/%2").arg(mZipRelateFolderPath, key);
		if (!zip->write(writeBytePath, byteData)) {
            setLastError(TR_("An error occurred while writing %1").arg(writeBytePath));  // cn:写%1发生错误
			continue;
		}
	}
	return true;
}

bool DAZipArchiveTask_ChartItem::readChartItems(DAZipArchive* zip, DAChartItemsManager* itemsData)
{
    //! 读取mZipRelateFolderPath下面的内容
    // 首先读取xml文件 -----------------------------------------------
    QList< QPair< QString, int > > keyToRTTI;
    if (!fromXml(zip, keyToRTTI)) {
        return false;
    }
    // 如果没有，就完成
    if (keyToRTTI.empty()) {
        // 空的推出完成
        return true;
    }
    // 不空，加载item
    for (const QPair< QString, int >& item : std::as_const(keyToRTTI)) {
        const QString itemPath = QString("%1/%2").arg(mZipRelateFolderPath, item.first);
        QByteArray byte        = zip->read(itemPath);
        if (byte.isEmpty()) {
            setLastError(TR_("Failed to read %1 from the archive. Reason: %2")
                             .arg(zip->getLastErrorString())
                             .arg(zip->getLastErrorString()));  // cn:无法从存档中读取%1，原因是%2
            continue;
        }
        // 读取完成后生成item
        QwtPlotItem* plotitem = qwtitemSerialization(byte);
        if (!plotitem) {
            // 说明序列化异常
            setLastError(TR_("Failed to deserialize from %1 to a drawing object").arg(itemPath));  // cn:无法从%1序列化到绘图对象
            continue;
        }
        // 序列化完成，存入DAChartItemsManager
        itemsData->recordItem(plotitem, item.first);
    }
    return true;
}

QString DAZipArchiveTask_ChartItem::chartItemsMgrXmlFileName()
{
	const static QString s_chartItemsMgrXmlFileName = QStringLiteral("chart-items.xml");
	return s_chartItemsMgrXmlFileName;
}

QString DAZipArchiveTask_ChartItem::getLastError() const
{
    return mLastError;
}

void DAZipArchiveTask_ChartItem::setLastError(const QString& lastError)
{
    mLastError = lastError;
    qDebug() << lastError;
}
}  // end namespace DA
