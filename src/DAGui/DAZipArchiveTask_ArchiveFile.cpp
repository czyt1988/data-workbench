#include "DAZipArchiveTask_ArchiveFile.h"
#include "DAZipArchive.h"
#include <QDebug>
#include <QFileInfo>
namespace DA
{
DAZipArchiveTask_ArchiveFile::DAZipArchiveTask_ArchiveFile() : DAAbstractArchiveTask()
{
}

DAZipArchiveTask_ArchiveFile::DAZipArchiveTask_ArchiveFile(const QString& archivePath, const QString& tempFilePath)
	: DAAbstractArchiveTask()
{
	setTempFilePath(tempFilePath);
	setArchivePath(archivePath);
}

DAZipArchiveTask_ArchiveFile::DAZipArchiveTask_ArchiveFile(const QString& archivePath) : DAAbstractArchiveTask()
{
}

DAZipArchiveTask_ArchiveFile::~DAZipArchiveTask_ArchiveFile()
{
}

QString DAZipArchiveTask_ArchiveFile::getTempFilePath() const
{
	return mTempFilePath;
}

void DAZipArchiveTask_ArchiveFile::setTempFilePath(const QString& v)
{
	mTempFilePath = v;
}

QString DAZipArchiveTask_ArchiveFile::getArchivePath() const
{
	return archivePath;
}

void DAZipArchiveTask_ArchiveFile::setArchivePath(const QString& v)
{
	archivePath = v;
}

bool DAZipArchiveTask_ArchiveFile::exec(DAAbstractArchive* archive, Mode mode)
{
	if (!archive) {
		return false;
	}
	DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
	if (mode == DAAbstractArchiveTask::WriteMode) {
		// 写模式
		if (!zip->isOpened()) {
			if (!zip->create()) {
				qDebug() << QString("create archive error:%1").arg(zip->getBaseFilePath());
				return false;
			}
		}
		if (!zip->writeFileToZip(getArchivePath(), getTempFilePath())) {
			qDebug() << QString("Unable to write the file from %1 to %2").arg(getTempFilePath(), getArchivePath());  // cn:无法把文件从%1写入到%2
			return false;
		}
	} else {
		// 读取数据模式
		if (!zip->isOpened()) {
			if (!zip->open()) {
				qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
				return false;
			}
		}
		// 先生成一个临时目录
		QFileInfo fi(getArchivePath());
		QString tempFilePath = mTempDir.filePath(fi.fileName());
		if (!zip->readToFile(getArchivePath(), tempFilePath)) {
			qDebug() << QString("Unable to write the file from %1 to %2.").arg(getArchivePath(), tempFilePath);
			return false;
		}
		setTempFilePath(tempFilePath);
	}
	return true;
}

}  // end DA
