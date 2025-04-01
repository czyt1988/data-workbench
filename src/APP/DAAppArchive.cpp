#include "DAAppArchive.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDebug>
namespace DA
{
//----------------------------------------------------
// DAAppArchive::Tas
//----------------------------------------------------

DAAppArchive::Task::Task()
{
}

DAAppArchive::Task::Task(const QString& path, const QByteArray& d, const QString& des)
    : relatePath(path), data(d), describe(des)
{
}

DAAppArchive::Task::Task(const QString& path, const DAAppArchive::Task::FunPtr& fp, const QString& des)
    : relatePath(path), function(fp), describe(des)
{
}

DAAppArchive::Task::~Task()
{
}
//----------------------------------------------------
// DAAppArchive
//----------------------------------------------------
DAAppArchive::DAAppArchive(QObject* par) : DAZipArchive(par)
{
}

DAAppArchive::DAAppArchive(const QString& zipPath, QObject* par) : DAZipArchive(zipPath, par)
{
}

DAAppArchive::~DAAppArchive()
{
}

void DAAppArchive::appendTask(const DAAppArchive::Task& t)
{
    mTaskQueue.push_back(t);
}

DAAppArchive::Task DAAppArchive::takeTask()
{
    DAAppArchive::Task t = mTaskQueue.front();
    mTaskQueue.pop_front();
    return t;
}

bool DAAppArchive::isTaskEmpty() const
{
    return mTaskQueue.empty();
}

int DAAppArchive::getTaskCount() const
{
    return static_cast< int >(mTaskQueue.size());
}

/**
 * @brief 转换为临时路径
 * @return
 */
QString DAAppArchive::toTemporaryPath(const QString& path)
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
bool DAAppArchive::replaceFile(const QString& file, const QString& beReplaceFile)
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

void DAAppArchive::saveAll(const QString& filePath)
{
    const int cnt = getTaskCount();
    int index     = 0;
    if (!isOpened()) {
        // 如果原来已经打开需要先关闭
        close();
    }
    // toTemporaryPath获取临时文件路径
    QString tempFilePath = toTemporaryPath(filePath);
    setBaseFilePath(tempFilePath);
    if (!create()) {
        // 创建失败，设置回来
        setBaseFilePath(filePath);
        emit taskFinished(DAAppArchive::SaveFailed);
        return;
    }
    while (!isTaskEmpty()) {
        DAAppArchive::Task t = takeTask();
        if (!execTask(t)) {
            emit taskFinished(DAAppArchive::SaveFailed);
            return;
        }
        ++index;
        emit taskProgress(cnt, index, t.describe);
    }
    // 创建完成关闭文件
    close();
    // 把文件替换为正式文件
    if (!replaceFile(tempFilePath, filePath)) {
        // 删除临时文件
        if (QFile::exists(tempFilePath)) {
            if (!QFile::remove(tempFilePath)) {
                qDebug() << "Failed to remove the temp file:" << tempFilePath;
                // 虽然删除临时文件失败，但也返回true
            }
        }
        emit taskFinished(DAAppArchive::SaveFailed);
        return;
    }
    // 替换完成后，重新以读取方式打开
    setBaseFilePath(filePath);
    if (!isOpened()) {
        open();
    }
    emit taskFinished(DAAppArchive::SaveSuccess);
}

/**
 * @brief 执行任务
 * @param t
 */
bool DAAppArchive::execTask(const DAAppArchive::Task& t)
{

    if (!t.data.isEmpty()) {
        // 如果data有内容，就直接写入的zip文件中
        return write(t.relatePath, t.data);
    } else {
        //
        if (t.function) {
            t.function(this);
        }
    }
    return true;
}

}
