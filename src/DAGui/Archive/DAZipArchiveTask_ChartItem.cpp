#include "Archive/DAZipArchiveTask_ChartItem.h"
#include <QDebug>
#include "Archive/DAChartItemArchive.h"
#include "DAChartSerialize.h"

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

/**
 * @brief 执行任务
 *
 * 读写逻辑统一走 @ref DAChartItemArchive（与工程存档/插件存档共用一份实现）：
 * 写模式把 mItemsManager 里的图元写成 chart-items.xml + 每图元一个二进制文件；
 * 读模式把图元读回 mItemsManager，供后续 charts.xml 按 key 挂载。
 * @param archive 存档对象
 * @param mode 写模式或读模式
 * @return 成功返回true；失败时 lastError 记录原因
 */
bool DAZipArchiveTask_ChartItem::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
{
    if (!archive) {
        return false;
    }
    QString errorMessage;
    bool isok = false;
    if (mode == DAAbstractArchiveTask::WriteMode) {
        isok = DAChartItemArchive::save(archive, &mItemsManager, mZipRelateFolderPath, &errorMessage);
    } else {
        isok = DAChartItemArchive::load(archive, &mItemsManager, mZipRelateFolderPath, &errorMessage);
    }
    if (!isok) {
        setLastError(errorMessage);
    } else if (!errorMessage.isEmpty()) {
        // 部分图元写入/读取失败不中断整体流程（与既有行为一致），但错误信息仍要透出
        setLastError(errorMessage);
    }
    return isok;
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
