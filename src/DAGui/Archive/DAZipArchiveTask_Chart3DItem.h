#ifndef DAZIPARCHIVETASK_CHART3DITEM_H
#define DAZIPARCHIVETASK_CHART3DITEM_H
#include "DAGuiAPI.h"
#include <QList>
#include <QString>
#include "Archive/DAAbstractArchiveTask.h"
#include "Archive/DAChart3DItemArchive.h"
#include "DAChart3DItemsManager.h"

namespace DA
{
class DAZipArchive;

/**
 * @brief 把三维图元（Qwt3DPlotItem）写/读 zip 的任务
 *
 * 读写逻辑已抽到 @ref DAChart3DItemArchive，本任务只负责在存档任务队列/线程中调用它。
 * 保存侧收的是 GUI 线程预生成的**快照**（不是图元管理者）：后台线程只写字节，
 * 不触碰 Qwt3DPlotItem*（与二维 chart-data 的 BLK-01 同一约束）。
 */
class DAGUI_API DAZipArchiveTask_Chart3DItem : public DAAbstractArchiveTask
{
public:
    // 保存构造：snapshots 必须由 GUI 线程用 DAChart3DItemArchive::makeSnapshots 预先生成
    DAZipArchiveTask_Chart3DItem(const QString& zipRelateFolderPath,
                                 const QList< DAChart3DItemSnapshot >& snapshots);
    // 加载构造
    DAZipArchiveTask_Chart3DItem(const QString& zipRelateFolderPath);
    ~DAZipArchiveTask_Chart3DItem();
    // 加载读回的 3D 图元池（key → 图元），供 charts.xml 按 key 挂回 3D 子图
    const DAChart3DItemsManager& getChart3DItemsManager() const;
    //
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;
    QString getLastError() const;
    void setLastError(const QString& lastError);

private:
    QString mZipRelateFolderPath;
    QList< DAChart3DItemSnapshot > mSnapshots;
    DAChart3DItemsManager mItemsManager;
    QString mLastError;
};
}  // namespace DA
#endif  // DAZIPARCHIVETASK_CHART3DITEM_H