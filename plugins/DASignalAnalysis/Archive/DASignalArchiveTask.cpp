#include "DASignalArchiveTask.h"
#include <QDebug>
#include "Archive/DAAbstractArchive.h"
#include "Archive/DAChartItemArchive.h"

namespace DA
{
DASignalArchiveTask::DASignalArchiveTask(const QByteArray& signalsJson,
                                         const QByteArray& figuresXml,
                                         const DAChartItemsManager& chartItems)
    : mData(signalsJson), mFiguresXml(figuresXml), mItemsManager(chartItems)
{
    setName(QStringLiteral("DASignalAnalysis"));
    setDescribe(QStringLiteral("signal analysis project data"));
}

DASignalArchiveTask::~DASignalArchiveTask()
{
}

/**
 * @brief 存档内 signals.json 的相对路径
 */
QString DASignalArchiveTask::archivePath()
{
    return QStringLiteral("signal-analysis/signals.json");
}

/**
 * @brief 存档内 figures.xml 的相对路径
 */
QString DASignalArchiveTask::figuresArchivePath()
{
    return QStringLiteral("signal-analysis/figures.xml");
}

/**
 * @brief 存档内 chart-data 目录前缀
 */
QString DASignalArchiveTask::chartDataPrefix()
{
    return QStringLiteral("signal-analysis/chart-data");
}

/**
 * @brief 执行读写任务（后台线程，不操作界面）
 *
 * 写：先写 signals.json；figures.xml 非空时再写 figures.xml 与 chart-data（图元二进制）。
 * 读：先读 signals.json（保留原有「条目是否存在」语义），再读 figures.xml，非空时
 * 读回 chart-data 填进 mItemsManager；旧工程没有 figures.xml/chart-data 时保持空并返回 true。
 * @return 写模式返回 signals.json 是否写成功（figures 写失败也返回 false 并记录）；
 *         读模式恒返回 true（条目缺失由 archiveEntryFound()/figuresEntryFound() 表达）
 */
bool DASignalArchiveTask::exec(DAAbstractArchive* archive, Mode mode)
{
    if (nullptr == archive) {
        qWarning() << "DASignalArchiveTask::exec: archive is null";
        return false;
    }
    if (mode == WriteMode) {
        const bool jsonWritten = archive->write(archivePath(), mData);
        if (!jsonWritten) {
            qWarning() << "DASignalArchiveTask::exec: failed to write" << archivePath();
        }
        if (!mFiguresXml.isEmpty()) {
            if (!archive->write(figuresArchivePath(), mFiguresXml)) {
                qWarning() << "DASignalArchiveTask::exec: failed to write" << figuresArchivePath();
                return false;
            }
            QString errorMessage;
            if (!DAChartItemArchive::save(archive, &mItemsManager, chartDataPrefix(), &errorMessage)) {
                qWarning() << "DASignalArchiveTask::exec: failed to save chart-data:" << errorMessage;
                return false;
            }
        }
        return jsonWritten;
    }
    // 读模式：每次读取都从干净状态开始（图元管理者为值语义，避免复用任务时残留上一工程的图元）
    mItemsManager = DAChartItemsManager();
    mData         = archive->read(archivePath());
    // 条目缺失时 read() 只返回空字节而不报错：记录「条目是否存在」，
    // 界面层据此区分「本工程没有本插件数据」（要清掉上一工程残留）与正常读到内容（评审 N-U2）
    mEntryFound = !mData.isEmpty();
    mFiguresXml = archive->read(figuresArchivePath());
    mFiguresEntryFound = !mFiguresXml.isEmpty();
    if (mFiguresEntryFound) {
        QString errorMessage;
        if (!DAChartItemArchive::load(archive, &mItemsManager, chartDataPrefix(), &errorMessage)) {
            // chart-data 损坏/缺失不算整个存档失败：界面层按「无用户图元」恢复，
            // 分析结果类 transient 图元本来就会重算重建
            qWarning() << "DASignalArchiveTask::exec: failed to load chart-data:" << errorMessage;
        }
    }
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

QByteArray DASignalArchiveTask::loadedFiguresXml() const
{
    return mFiguresXml;
}

bool DASignalArchiveTask::figuresEntryFound() const
{
    return mFiguresEntryFound;
}

const DAChartItemsManager& DASignalArchiveTask::chartItemsManager() const
{
    return mItemsManager;
}

DAChartItemsManager& DASignalArchiveTask::chartItemsManager()
{
    return mItemsManager;
}
}  // namespace DA
