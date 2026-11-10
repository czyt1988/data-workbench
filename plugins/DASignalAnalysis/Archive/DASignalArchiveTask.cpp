#include "DASignalArchiveTask.h"
#include <QDebug>
#include "Archive/DAAbstractArchive.h"

namespace DA
{
DASignalArchiveTask::DASignalArchiveTask(const QByteArray& saveData) : mData(saveData)
{
    setName(QStringLiteral("DASignalAnalysis"));
    setDescribe(QStringLiteral("signal analysis project data"));
}

DASignalArchiveTask::~DASignalArchiveTask()
{
}

/**
 * @brief 存档内的相对路径
 */
QString DASignalArchiveTask::archivePath()
{
    return QStringLiteral("signal-analysis/signals.json");
}

/**
 * @brief 执行读写任务（后台线程，不操作界面）
 */
bool DASignalArchiveTask::exec(DAAbstractArchive* archive, Mode mode)
{
    if (nullptr == archive) {
        qWarning() << "DASignalArchiveTask::exec: archive is null";
        return false;
    }
    if (mode == WriteMode) {
        return archive->write(archivePath(), mData);
    }
    mData = archive->read(archivePath());
    // 条目缺失时 read() 只返回空字节而不报错：记录「条目是否存在」，
    // 界面层据此区分「本工程没有本插件数据」（要清掉上一工程残留）与正常读到内容（评审 N-U2）
    mEntryFound = !mData.isEmpty();
    return true;
}

QByteArray DASignalArchiveTask::loadedData() const
{
    return mData;
}

bool DASignalArchiveTask::archiveEntryFound() const
{
    return mEntryFound;
}
}  // namespace DA
