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
    return true;
}

QByteArray DASignalArchiveTask::loadedData() const
{
    return mData;
}
}  // namespace DA
