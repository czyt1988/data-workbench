#include "DAAbstractArchive.h"
#include "DAAbstractArchiveTask.h"
#include <deque>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDebug>
namespace DA
{

//===============================================================
// DAAbstractArchive::PrivateData
//===============================================================
class DAAbstractArchive::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractArchive)
public:
	PrivateData(DAAbstractArchive* p);
    std::shared_ptr< DAAbstractArchiveTask > takeTask();

public:
	QString mBaseFilePath;
    std::deque< std::shared_ptr< DAAbstractArchiveTask > > mTaskQueue;
};

DAAbstractArchive::PrivateData::PrivateData(DAAbstractArchive* p) : q_ptr(p)
{
}

std::shared_ptr< DAAbstractArchiveTask > DAAbstractArchive::PrivateData::takeTask()
{
    std::shared_ptr< DAAbstractArchiveTask > t = mTaskQueue.front();
    mTaskQueue.pop_front();
    return t;
}

//===============================================================
// DAAbstractArchive
//===============================================================
DAAbstractArchive::DAAbstractArchive(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DAAbstractArchive::~DAAbstractArchive()
{
}

void DAAbstractArchive::setBaseFilePath(const QString& path)
{
	d_ptr->mBaseFilePath = path;
}

QString DAAbstractArchive::getBaseFilePath() const
{
    return d_ptr->mBaseFilePath;
}

void DAAbstractArchive::appendTask(const std::shared_ptr< DAAbstractArchiveTask >& task)
{
    d_ptr->mTaskQueue.emplace_back(task);
}

int DAAbstractArchive::getTaskCount() const
{
    return static_cast< int >(d_ptr->mTaskQueue.size());
}

bool DAAbstractArchive::isTaskQueueEmpty() const
{
    return d_ptr->mTaskQueue.empty();
}

/**
 * @brief 从顶部提取一个任务
 * @return
 */
std::shared_ptr< DAAbstractArchiveTask > DAAbstractArchive::takeTask()
{
    return d_ptr->takeTask();
}

/**
 * @brief 转换为临时路径
 * @return
 */
QString DAAbstractArchive::toTemporaryPath(const QString& path)
{
    QFileInfo fileInfo(path);

    // 获取原始文件的目录和文件名
    QDir dir = fileInfo.absoluteDir();

    // 构造临时文件名：.~原文件名
    QString tempFileName = QString(".~%1").arg(fileInfo.fileName());

    // 拼接完整临时路径
    QString tempPath = dir.filePath(tempFileName);

    return tempPath;
}

/**
 * @brief 替换文件
 * @param file 要被替换的文件路径,此函数执行完成后，这个文件将消失
 * @param beReplaceFile 用于替换的文件路径,这个路径文件可以不存在
 * @return 成功返回 true，失败返回 false
 */
bool DAAbstractArchive::replaceFile(const QString& file, const QString& beReplaceFile)
{
    // 检查文件是否存在
    if (!QFile::exists(file)) {
        qDebug() << "file does not exist:" << file;
        return false;
    }

    // 确保两个文件不是同一个文件
    if (QFileInfo(file).canonicalFilePath() == QFileInfo(beReplaceFile).canonicalFilePath()) {
        qDebug() << "The two files are the same. No replacement needed.";
        return true;  // 如果是同一个文件，直接返回成功
    }

    // 将 beReplaceFile 重命名为 file
    if (!QFile::copy(file, beReplaceFile)) {
        qDebug() << "Failed to copy replacement file to target location:" << file << "->" << beReplaceFile;
        return false;
    }
    // 删除目标文件（file）
    if (!QFile::remove(file)) {
        qDebug() << "Failed to remove the original file:" << file;
        // 虽然删除临时文件失败，但也返回true
        return true;
    }
    return true;
}

}
