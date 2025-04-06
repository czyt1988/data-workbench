#ifndef DAZIPARCHIVETASK_ARCHIVEFILE_H
#define DAZIPARCHIVETASK_ARCHIVEFILE_H
#include "DAGuiAPI.h"
#include "DAAbstractArchiveTask.h"
#include <QTemporaryDir>
namespace DA
{
/**
 * @brief 把文件压缩到压缩包的任务
 *
 * 执行 DAZipArchiveTask_ArchiveFile(const QString& tempFilePath, const QString& archivePath); 构造函数的时候为保存，
 * 会把tempFilePath文件保存到zip的archivePath文件中
 *
 * 执行 DAZipArchiveTask_ArchiveFile(const QString& archivePath);
 * 进行读取操作，读取操作会把archivePath的内容，保存到一个临时路径中， 保存的临时路径通过@sa getTempFilePath 函数获取
 */
class DAGUI_API DAZipArchiveTask_ArchiveFile : public DAAbstractArchiveTask
{
public:
	DAZipArchiveTask_ArchiveFile();
	// 保存
	DAZipArchiveTask_ArchiveFile(const QString& archivePath, const QString& tempFilePath);
	// 读取
	DAZipArchiveTask_ArchiveFile(const QString& archivePath);
	virtual ~DAZipArchiveTask_ArchiveFile();
	// 临时文件路径
	QString getTempFilePath() const;
	void setTempFilePath(const QString& v);
	// zip档案文件路径
	QString getArchivePath() const;
	void setArchivePath(const QString& v);
	//
	virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;

private:
	QString mTempFilePath;
	QString archivePath;
	QTemporaryDir mTempDir;
};
}

#endif  // DAZIPARCHIVETASK_ARCHIVEFILE_H
