#include "Archive/DAAbstractArchive.h"
#include "Archive/DAAbstractArchiveTask.h"
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
	std::shared_ptr< DA::DAAbstractArchiveTask > takeTask();

public:
	QString mBaseFilePath;
	std::deque< std::shared_ptr< DA::DAAbstractArchiveTask > > mTaskQueue;
};

DAAbstractArchive::PrivateData::PrivateData(DAAbstractArchive* p) : q_ptr(p)
{
}

std::shared_ptr< DA::DAAbstractArchiveTask > DAAbstractArchive::PrivateData::takeTask()
{
	if (mTaskQueue.empty()) {
		return nullptr;
	}
	std::shared_ptr< DA::DAAbstractArchiveTask > t = mTaskQueue.front();
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

bool DAAbstractArchive::setBaseFilePath(const QString& path)
{
	d_ptr->mBaseFilePath = path;
	return true;
}

QString DAAbstractArchive::getBaseFilePath() const
{
	DA_DC(d);
	return d->mBaseFilePath;
}

/**
 * @brief 检查存档内是否存在指定路径
 *
 * 基类没有通用的存在性判断方式（read 为非常量纯虚函数），默认返回 false；
 * 需要区分「缺失」与「存在但为空」的存档（zip 等）应覆盖本函数。
 * @param relatePath 存档内相对路径
 * @return 存在返回true，默认返回false
 */
bool DAAbstractArchive::contains(const QString& relatePath) const
{
	Q_UNUSED(relatePath);
	return false;
}

/**
 * @brief 存档是否处于可用状态（已打开/已创建）
 *
 * 基类不知道子类的打开语义（有的存档对象本身就是目录/内存，无需打开），默认返回 true；
 * 需要区分「未打开」与「打开但没有该文件」的存档（zip 等）应覆盖本函数。
 * 读取方在调用 contains/read 前应先判断本函数，否则未打开的存档会被当成「旧工程/空存档」静默放过。
 * @return 可用返回true，默认返回true
 */
bool DAAbstractArchive::isOpen() const
{
	return true;
}

void DAAbstractArchive::appendTask(const std::shared_ptr< DA::DAAbstractArchiveTask >& task)
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
std::shared_ptr< DA::DAAbstractArchiveTask > DAAbstractArchive::takeTask()
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
		qWarning() << "file does not exist:" << file;
		return false;
	}

	// 确保两个文件不是同一个文件
	if (QFileInfo(file).canonicalFilePath() == QFileInfo(beReplaceFile).canonicalFilePath()) {
		qWarning() << "The two files are the same. No replacement needed.";
		return true;  // 如果是同一个文件，直接返回成功
	}

	//! 将 beReplaceFile 重命名为 file,如果beReplaceFile已经存在，QFile::copy(file, beReplaceFile)这个函数会返回false，
	//! 因此，beReplaceFile如果存在，先要删除beReplaceFile
	if (QFile::exists(beReplaceFile)) {
		if (!QFile::remove(beReplaceFile)) {
			qWarning() << QString("Failed to remove %1 file").arg(beReplaceFile);
			return false;
		}
	}
	if (!QFile::copy(file, beReplaceFile)) {
		qWarning() << "Failed to copy replacement file to target location:" << file << "->" << beReplaceFile;
		return false;
	}
	// 删除目标文件（file）
	if (!QFile::remove(file)) {
		qWarning() << "Failed to remove the original file:" << file;
		// 虽然删除临时文件失败，但也返回true
		return true;
	}
	return true;
}

}
