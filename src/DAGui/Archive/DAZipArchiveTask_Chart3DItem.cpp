#include "Archive/DAZipArchiveTask_Chart3DItem.h"
#include <QDebug>

namespace DA
{
DAZipArchiveTask_Chart3DItem::DAZipArchiveTask_Chart3DItem(const QString& zipRelateFolderPath,
                                                           const QList< DAChart3DItemSnapshot >& snapshots)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath), mSnapshots(snapshots)
{
}

DAZipArchiveTask_Chart3DItem::DAZipArchiveTask_Chart3DItem(const QString& zipRelateFolderPath)
    : DAAbstractArchiveTask(), mZipRelateFolderPath(zipRelateFolderPath)
{
}

DAZipArchiveTask_Chart3DItem::~DAZipArchiveTask_Chart3DItem()
{
}

const DAChart3DItemsManager& DAZipArchiveTask_Chart3DItem::getChart3DItemsManager() const
{
    return mItemsManager;
}

/**
 * @brief 执行任务
 *
 * 读写逻辑统一走 @ref DAChart3DItemArchive（与工程存档共用一份实现）：
 * 写模式把 GUI 线程预序列化的快照写成 chart3d-items.xml + 每图元一个二进制文件；
 * 读模式把图元读回 mItemsManager，供后续 charts.xml 按 key 挂回 3D 子图。
 * @param archive 存档对象
 * @param mode 写模式或读模式
 * @return 成功返回 true；失败时 lastError 记录原因
 */
bool DAZipArchiveTask_Chart3DItem::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
{
    if (!archive) {
        return false;
    }
    QString errorMessage;
    bool isok = false;
    if (mode == DAAbstractArchiveTask::WriteMode) {
        isok = DAChart3DItemArchive::saveSnapshots(archive, mSnapshots, mZipRelateFolderPath, &errorMessage);
    } else {
        isok = DAChart3DItemArchive::load(archive, &mItemsManager, mZipRelateFolderPath, &errorMessage);
    }
    if (!isok) {
        setLastError(errorMessage);
    } else if (!errorMessage.isEmpty()) {
        // 部分图元写入/读取失败不中断整体流程（与二维 chart-data 一致），但错误信息仍要透出
        setLastError(errorMessage);
    }
    return isok;
}

QString DAZipArchiveTask_Chart3DItem::getLastError() const
{
    return mLastError;
}

void DAZipArchiveTask_Chart3DItem::setLastError(const QString& lastError)
{
    mLastError = lastError;
    qDebug() << lastError;
}
}  // end namespace DA