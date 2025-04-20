#include "DAZipArchive.h"
#include <memory>
#include <QDebug>
#include <optional>
#include <QDateTime>
#include <QDir>
#include "DAAbstractArchiveTask.h"
#include "quazip/quazipfile.h"
namespace DA
{
//===============================================================
// DAZipArchive::PrivateData
//===============================================================
class DAZipArchive::PrivateData
{
	DA_DECLARE_PUBLIC(DAZipArchive)
public:
	PrivateData(DAZipArchive* p);
	void makeSureZipPtr();
	// 是否打开
	bool isOpened() const;
	// 获取所有文件
	QStringList getAllFiles() const;

public:
	// 打开
	static const char* password();
	static int compressLevel();

public:
	std::unique_ptr< QuaZip > mZip;
	QString mLastErrorString;
	static const char* s_password;
	static int s_zip_compress_level;
};
const char* DAZipArchive::PrivateData::s_password   = nullptr;
int DAZipArchive::PrivateData::s_zip_compress_level = Z_NO_COMPRESSION;

DAZipArchive::PrivateData::PrivateData(DAZipArchive* p) : q_ptr(p)
{
	makeSureZipPtr();
}

void DAZipArchive::PrivateData::makeSureZipPtr()
{
	if (!mZip) {
		mZip = std::make_unique< QuaZip >();
	}
}

bool DAZipArchive::PrivateData::isOpened() const
{
	if (mZip && mZip->isOpen()) {
		return true;
	}
	return false;
}

const char* DAZipArchive::PrivateData::password()
{
	return s_password;
}

int DAZipArchive::PrivateData::compressLevel()
{
	return s_zip_compress_level;
}

QStringList DAZipArchive::PrivateData::getAllFiles() const
{
	return mZip->getFileNameList();
}

//===============================================================
// DAZipArchive
//===============================================================
DAZipArchive::DAZipArchive(QObject* par) : DAAbstractArchive(par), DA_PIMPL_CONSTRUCT
{
}

DAZipArchive::DAZipArchive(const QString& zipPath, QObject* par) : DAAbstractArchive(par), DA_PIMPL_CONSTRUCT
{
	setZipFileName(zipPath);
}

DAZipArchive::~DAZipArchive()
{
}

bool DAZipArchive::setBaseFilePath(const QString& path)
{
	DAAbstractArchive::setBaseFilePath(path);
	// 线关闭原来的
	close();
	return setZipFileName(path);
}

bool DAZipArchive::setZipFileName(const QString& fileName)
{
	DA_D(d);
	if (isOpened()) {
		qDebug() << tr("%1 is already opened").arg(fileName);  // cn:当前文件%1已经打开;
		return true;
	}
	d->makeSureZipPtr();
	d->mZip->setZipName(fileName);
	return true;
}

/**
 * @brief 打开一个压缩包，主要为了读取
 * @return
 */
bool DAZipArchive::open()
{
	DA_D(d);
	QString filePath = getBaseFilePath();
	if (isOpened()) {
		if (d->mZip->getMode() != QuaZip::mdUnzip) {
			close();
		} else {
			qDebug() << tr("%1 is already opened").arg(filePath);  // cn:当前文件%1已经打开;
			return true;
		}
	}
	if (!d->mZip->open(QuaZip::mdUnzip)) {
		qDebug() << tr("failed to open archive file %1").arg(filePath);
		d->mLastErrorString = d->mZip->getIoDevice()->errorString();
		d->mZip.reset();  // 重置 QuaZip 指针
		return false;
	}
	return true;
}

/**
 * @brief 创建一个压缩包
 * @return
 */
bool DAZipArchive::create()
{
	DA_D(d);
	QString filePath = getBaseFilePath();
	if (isOpened()) {
		if (d->mZip->getMode() != QuaZip::mdCreate) {
			close();
		} else {
			qDebug() << tr("%1 is already opened").arg(filePath);  // cn:当前文件%1已经打开;
			return true;
		}
	}
	if (!d->mZip->open(QuaZip::mdCreate)) {
		qDebug() << tr("failed to open archive file %1").arg(filePath);
		d->mLastErrorString = d->mZip->getIoDevice()->errorString();
		d->mZip.reset();  // 重置 QuaZip 指针
		return false;
	}
	return true;
}

/**
 * @brief 判断是否打开文件
 * @return
 */
bool DAZipArchive::isOpened() const
{
    return d_ptr->isOpened();
}

bool DAZipArchive::close()
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open,can not close");  // cn:文件没有打开，无法关闭
		return false;
	}

	// 关闭 ZIP 文件
	d->mZip->close();

	return true;
}

/**
 * @brief 写数据
 * @param relatePath
 * @param byte
 * @return
 */
bool DAZipArchive::write(const QString& relatePath, const QByteArray& byte)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return false;
	}

	// 打开一个新的 QuaZipFile 以存储文件
	QuaZipFile zipFile(d->mZip.get());

	if (!zipFile.open(QIODevice::WriteOnly,
					  QuaZipNewInfo(relatePath),
					  DAZipArchive::PrivateData::password(),
					  0,
					  Z_DEFLATED,
					  DAZipArchive::PrivateData::compressLevel())) {  // 设置压缩级别为 0
		d->mLastErrorString = zipFile.errorString();
		qDebug() << tr("The file %1 in the archive could not be opened. The reason for the error is %2")
						.arg(relatePath, zipFile.errorString());  // cn:无法打开文件中的%1,错误原因为%2
		return false;
	}

	// 写入数据
	zipFile.write(byte);
	zipFile.close();
	return true;
}

/**
 * @brief 将本地文件写入ZIP压缩包中的指定路径
 *
 * 本函数将本地文件（localFilePath）的内容写入到已打开的ZIP对象（zip）中，
 * 文件在ZIP内的路径由relatePath指定。函数会根据文件大小自动选择一次性写入或分块写入策略，
 * 以避免内存占用过高，并确保大文件处理的稳定性。
 *
 * @param[in] relatePath 文件在ZIP内的相对路径（如："doc/report.txt"），
 *                       路径格式不应以斜杠开头，目录层级使用正斜杠"/"分隔。
 * @param[in] localFilePath 待写入的本地文件路径，必须为有效可读的常规文件，
 *                          符号链接或其他特殊文件将被拒绝。
 * @param[in] chunk_mb 分块大小，默认为4mb
 *
 * @return bool
 *         - true  : 文件成功写入ZIP
 *         - false : 失败（可能原因：文件不存在/不可读、ZIP写入错误、内存不足等）
 * @note
 * - **分块策略**: 文件大小超过chunk_mb MB时启用分块写入，每次读取chunk_mb MB数据以减少内存峰值
 * - **元数据保留**: 使用原文件的修改时间和权限信息（通过QuaZipNewInfo实现）
 *
 * @example 基本用法示例
 * @code
 * QuaZip zip("archive.zip");
 * zip.open(QuaZip::mdCreate); // 必须确保zip已打开
 *
 * bool ok = DAZipArchive::writeFileToZip(&zip,"data/sample.txt","/home/user/sample.txt");
 *
 * if (ok) {
 *     qDebug() << "写入成功";
 * } else {
 *     qDebug() << "写入失败";
 * }
 * @endcode
 *
 * @warning 需确保:
 * - ZIP对象已通过open()方法打开且未关闭
 * - 写入过程中本地文件内容不被修改
 * - 多线程场景需自行处理同步
 *
 */
bool DAZipArchive::writeFileToZip(const QString& relatePath, const QString& localFilePath, std::size_t chunk_mb)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return false;
	}
	return writeFileToZip(d->mZip.get(), relatePath, localFilePath, chunk_mb);
}

/**
 * @brief 读数据
 * @param relatePath
 * @return
 */
QByteArray DAZipArchive::read(const QString& relatePath)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return QByteArray();
	}
	// 打开 QuaZipFile 以读取文件
	if (!d->mZip->setCurrentFile(relatePath)) {
		qDebug() << tr("Unable to locate the %1 file in the current archive. The error code is %2")
						.arg(relatePath)
						.arg(d->mZip->getZipError());  // cn:无法找到当前档案下的%1文件。错误码为%2
		return QByteArray();
	}
	QuaZipFile zipFile(d->mZip.get());
	if (!zipFile.open(QIODevice::ReadOnly)) {  // 设置压缩级别为 0
		d->mLastErrorString = zipFile.errorString();

		qDebug() << tr("The file %1 in the archive could not be opened. The error code is %2")
						.arg(relatePath)
						.arg(zipFile.getZipError());  // cn:无法打开档案中的%1文件，错误码为%2
		return QByteArray();
	}

	// 读取数据
	QByteArray data = zipFile.readAll();
	zipFile.close();

	return data;
}

/**
 * @brief 从 ZIP 归档中提取指定文件到本地路径
 *
 * @param[in] zipRelatePath ZIP 归档内的相对文件路径（例如："documents/report.pdf"）
 * @param[in] localFilePath 要写入的本地文件绝对路径（例如："/home/user/report.pdf"）
 * @param[in] chunk_mb 读写缓冲区大小（单位：MB），建议值 1-64，默认 4MB
 * @return bool
 * @retval true 文件提取成功
 * @retval false 提取失败，可能原因包括：
 *               - 参数无效
 *               - ZIP 内找不到指定文件
 *               - 磁盘空间不足
 *               - 文件读写错误
 *
 * @note 函数特性：
 * - 支持大文件处理（恒定内存占用）
 * - 自动创建目标目录结构
 * - 失败时自动清理不完整文件
 * - 线程安全性：非线程安全，需外部同步
 *
 * @warning
 * - 需确保传入的 QuaZip 实例已处于打开状态（QuaZip::mdUnzip 模式）
 * - chunk_mb 值过大会增加内存占用，过小会影响IO效率
 */
bool DAZipArchive::readToFile(const QString& zipRelatePath, const QString& localFilePath, size_t chunk_mb)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return false;
	}
	return readToFile(d->mZip.get(), zipRelatePath, localFilePath, chunk_mb);
}

/**
 * @brief 获取所有文件
 * @return
 */
QStringList DAZipArchive::getAllFiles() const
{
    return d_ptr->getAllFiles();
}

/**
 * @brief 检查文件是否存在
 * @param relatePath
 * @return
 */
bool DAZipArchive::contains(const QString& relatePath) const
{
	QStringList allFiles = getAllFiles();
	return allFiles.contains(relatePath);
}

/**
 * @brief 删除文件
 * @param relatePath
 * @return
 */
bool DAZipArchive::remove(const QString& fileToRemove)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return false;
	}
	if (!contains(fileToRemove)) {
		qDebug() << QString("remove %1,but archive not contain this file").arg(fileToRemove);
		return false;
	}
	QuaZip* zip = d->mZip.get();
	// 先保留之前的currentfile
	QString oldCurrentFile = zip->getCurrentFileName();
	if (oldCurrentFile == fileToRemove) {
		oldCurrentFile = QString();
	}
	// QuaZip 不支持直接删除文件，因此需要重新创建一个新的 ZIP 文件
	QString archivePath = getBaseFilePath();
	QFileInfo fi(archivePath);
	QString tempZipPath = QString("%1/.tmp-%2").arg(fi.absolutePath()).arg(reinterpret_cast< quintptr >(this));  // 创建临时文件路径
	std::unique_ptr< QuaZip > newZip = std::make_unique< QuaZip >(tempZipPath);
	if (!newZip->open(QuaZip::mdCreate)) {
		d->mLastErrorString = tr("Failed to create temporary file:%1").arg(tempZipPath);
		return false;
	}

	// 遍历原压缩包中的文件
	zip->goToFirstFile();
	do {
		QString currentFileName = zip->getCurrentFileName();
		if (currentFileName != fileToRemove) {
			QuaZipFile inFile(zip);
			// 将非目标文件添加到新压缩包
			QuaZipFile outFile(newZip.get());
			if (!outFile.open(QIODevice::WriteOnly,
							  QuaZipNewInfo(currentFileName),
							  DAZipArchive::PrivateData::password(),
							  0,
							  Z_DEFLATED,
							  DAZipArchive::PrivateData::compressLevel())) {
				qDebug() << tr("open %1 to temp file error").arg(currentFileName);  // cn:打开临时文件%1失败
				d->mLastErrorString = tr("open %1 to temp file error").arg(currentFileName);  // cn:打开临时文件%1失败
				newZip->close();
				zip->setCurrentFile(oldCurrentFile);
				return false;
			}
			// 把具体内容读取
			if (!inFile.open(QIODevice::ReadOnly)) {
				outFile.close();
				newZip->close();
				zip->setCurrentFile(oldCurrentFile);
				return false;
			}
			outFile.write(inFile.readAll());
			inFile.close();
			outFile.close();
		}
	} while (zip->goToNextFile());
	// 先关闭原来压缩包
	zip->close();
	// 替换原压缩包
	if (!QFile::remove(archivePath) || !QFile::rename(tempZipPath, archivePath)) {
		qDebug() << tr("when rename temp %1 to archive %2 occure error").arg(tempZipPath, archivePath);  // cn:当把临时文件改名为存档文件时发生异常
		return false;
	}
	// 把内存还原
	d->mZip = std::move(newZip);
	if (!oldCurrentFile.isEmpty()) {
		d->mZip->setCurrentFile(oldCurrentFile);
	}
	return true;
}

/**
 * @brief 获取文件大小
 *
 * @note 如果没有返回-1
 * @return
 */
qint64 DAZipArchive::getFileSize() const
{
	QFileInfo fileInfo(getBaseFilePath());
	return fileInfo.exists() ? fileInfo.size() : -1;  // 如果文件不存在，返回 -1
}

/**
 * @brief 获取文件数量，此函数会遍历zip的所有文件
 * @return
 */
int DAZipArchive::getFileCount() const
{
	QStringList filelist = d_ptr->getAllFiles();
	return filelist.size();
}

/**
 * @brief 获取注释内容
 * @return
 */
QString DAZipArchive::getComment() const
{
	DA_DC(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return QString();
	}

	return d->mZip->getComment();
}

void DAZipArchive::setComment(const QString& comment)
{
	DA_D(d);
	if (!isOpened()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return;
	}

	d->mZip->setComment(comment);
}

/**
 * @brief 压缩包整体解压到目录下
 * @param extractDir
 * @return
 */
bool DAZipArchive::extractToDirectory(const QString& extractDir)
{
	DA_D(d);
	if (!isOpened()) {
		if (!open()) {
			qDebug() << tr("can not open archive");  // cn:无法打开档案
			return false;
		}
	}
	return extractToDirectory(d->mZip.get(), extractDir);
}

bool DAZipArchive::compressDirectory(const QString& folderPath)
{
	DA_D(d);
	if (!isOpened()) {
		if (!create()) {
			qDebug() << tr("can not open archive");  // cn:无法打开档案
			return false;
		}
	}
	return compressDirectory(folderPath, d->mZip.get());
}

QStringList DAZipArchive::getFileNameList() const
{
    return d_ptr->mZip->getFileNameList();
}

/**
 * @brief 获取zip中一个文件夹下的所有文件列表
 *
 * @note 注意不包括这个文件下的子目录及子目录下的文件
 * @param zipFolderPath
 * @return
 */
QStringList DAZipArchive::getFolderFileNameList(const QString& zipFolderPath) const
{
	DA_DC(d);
	if (!isOpened()) {
		return QStringList();
	}
	QStringList res;
	const QList< QuaZipFileInfo64 > files64 = d->mZip->getFileInfoList64();
	for (const QuaZipFileInfo64& fileInfo : files64) {
		QString zipPath = fileInfo.name;

		// 跳过目录和子文件夹中的文件
		if (!zipPath.startsWith(zipFolderPath + "/")) {
			continue;
		}

		// 检查是否在目标文件夹的直接子级
		QString relativePath = zipPath.mid(zipFolderPath.length() + 1);
		if (relativePath.contains('/')) {
			continue;  // 排除子目录中的文件
		}
		res.push_back(zipPath);
	}
	return res;
}

QuaZip* DAZipArchive::quazip() const
{
	return d_ptr->mZip.get();
}

void DAZipArchive::saveAll(const QString& filePath)
{
	const int cnt = getTaskCount();
	int index     = 0;
	if (isOpened()) {
		// 如果原来已经打开需要先关闭
		close();
	}
	// toTemporaryPath获取临时文件路径,这里临时路径是xxx文件名下面建立一个.~xxx文件
	QString tempFilePath = toTemporaryPath(filePath);
	if (!setBaseFilePath(tempFilePath)) {
		qDebug() << "can not open " << tempFilePath;
		emit taskFinished(DAAbstractArchive::SaveFailed);
		return;
	}
	if (!create()) {
		// 创建失败，设置回来
		setBaseFilePath(filePath);
		emit taskFinished(DAAbstractArchive::SaveFailed);
		return;
	}
	while (!isTaskQueueEmpty()) {
		std::shared_ptr< DAAbstractArchiveTask > task = takeTask();
		if (!task->exec(this, DAAbstractArchiveTask::WriteMode)) {
			emit taskFinished(DAAbstractArchive::SaveFailed);
			return;
		}
		++index;
		emit taskProgress(cnt, index, task);
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
		emit taskFinished(DAAbstractArchive::SaveFailed);
		return;
	}
	// 替换完成后，重新以读取方式打开
	setBaseFilePath(filePath);

	emit taskFinished(DAAbstractArchive::SaveSuccess);
}

void DAZipArchive::loadAll(const QString& filePath)
{
	const int cnt = getTaskCount();
	int index     = 0;
	if (isOpened()) {
		// 如果原来已经打开需要先关闭
		close();
	}
	setBaseFilePath(filePath);
	if (!open()) {
		// 打开失败
		emit taskFinished(DAAbstractArchive::LoadFailed);
		return;
	}
	while (!isTaskQueueEmpty()) {
		std::shared_ptr< DAAbstractArchiveTask > task = takeTask();
		if (!task->exec(this, DAAbstractArchiveTask::ReadMode)) {
			emit taskFinished(DAAbstractArchive::LoadFailed);
			return;
		}
		++index;
		emit taskProgress(cnt, index, task);
	}
	// 加载完成后关闭
	close();
	emit taskFinished(DAAbstractArchive::LoadSuccess);
}

bool DAZipArchive::extractToDirectory(const QString& zipFilePath, const QString& extractDir)
{
	QuaZip zip(zipFilePath);
	zip.setAutoClose(true);
	return extractToDirectory(&zip, extractDir);
}

bool DAZipArchive::extractToDirectory(QuaZip* zip, const QString& extractDir)
{
	if (!zip->isOpen()) {
		if (!zip->open(QuaZip::mdUnzip)) {
			qDebug() << "can not open archive:" << zip->getZipName();  // cn:无法打开档案
			return false;
		}
	}

	// 确保目标目录存在
	QDir targetDir(extractDir);
	if (!targetDir.exists()) {
		if (!targetDir.mkpath(".")) {
			qDebug() << tr("Failed to create target directory:%1").arg(extractDir);  // cn：无法创建目标文件夹%1
			return false;
		}
	}

	// 遍历 ZIP 文件中的所有条目
	if (!zip->goToFirstFile()) {
		qDebug() << "archive is empty or failed to read entries.";
		return false;
	}
	do {
		QString entryName = zip->getCurrentFileName();      // 获取当前条目的相对路径
		QString fullPath  = targetDir.filePath(entryName);  // 目标文件的完整路径

		if (entryName.endsWith('/')) {
			// 如果是目录条目，创建对应的目录
			QDir dir;
			if (!dir.mkpath(fullPath)) {
				qDebug() << "Failed to create directory:" << fullPath;
				return false;
			}
		} else {
			// 如果是文件条目，解压文件内容
			QuaZipFile zipFile(zip);
			if (!zipFile.open(QIODevice::ReadOnly)) {
				qDebug() << "Failed to open file in zip archive:" << entryName;
				return false;
			}

			// 创建目标文件并写入内容
			QFile outFile(fullPath);
			if (!outFile.open(QIODevice::WriteOnly)) {
				qDebug() << "Failed to create output file:" << fullPath;
				zipFile.close();
				return false;
			}

			outFile.write(zipFile.readAll());
			outFile.close();
			zipFile.close();
		}
	} while (zip->goToNextFile());
	return true;
}

bool DAZipArchive::compressDirectory(const QString& folderPath, const QString& zipFilePath)
{
	// 打开 ZIP 文件
	QuaZip zip(zipFilePath);
	if (!zip.open(QuaZip::mdCreate)) {
		qDebug() << "Failed to create ZIP file:" << zipFilePath;
		return false;
	}
	return compressDirectory(folderPath, &zip);
}

bool DAZipArchive::compressDirectory(const QString& folderPath, QuaZip* zip, const QString& relativeBase)
{
	QDir folderDir(folderPath);
	if (!folderDir.exists()) {
		qDebug() << "Source folder does not exist:" << folderPath;
		return false;
	}
	// 遍历文件夹中的所有文件和子目录
	const QStringList allFiles = folderDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
	for (const QString& entry : allFiles) {
		QString fullPath     = folderDir.filePath(entry);           // 当前条目的完整路径
		QString relativePath = QDir(relativeBase).filePath(entry);  // 相对于顶层文件夹的路径

		QFileInfo fileInfo(fullPath);
		if (fileInfo.isDir()) {
			// 如果是目录，递归处理子目录,同时传入relativePath，能组出完整路径
			if (!compressDirectory(fullPath, zip, relativePath)) {
				qDebug() << "Failed to compress subdirectory:" << fullPath;
				return false;
			}
		} else {
			// 如果是文件，将其写入 ZIP 文件
			QFile inFile(fullPath);
			if (!inFile.open(QIODevice::ReadOnly)) {
				qDebug() << "Failed to open file for reading:" << fullPath;
				return false;
			}

			QByteArray fileData = inFile.readAll();

			QuaZipFile zipFile(zip);
			if (!zipFile.open(QIODevice::WriteOnly,
							  QuaZipNewInfo(relativePath),
							  DAZipArchive::PrivateData::password(),
							  0,
							  Z_DEFLATED,
							  DAZipArchive::PrivateData::compressLevel())) {
				qWarning() << "Failed to open file in ZIP archive:" << relativePath;
				return false;
			}

			zipFile.write(fileData);
			inFile.close();
			zipFile.close();
		}
	}
	return true;
}

/**
 * @brief 将本地文件写入ZIP压缩包中的指定路径
 *
 * 本函数将本地文件（localFilePath）的内容写入到已打开的ZIP对象（zip）中，
 * 文件在ZIP内的路径由relatePath指定。函数会根据文件大小自动选择一次性写入或分块写入策略，
 * 以避免内存占用过高，并确保大文件处理的稳定性。
 *
 * @param[in] zip 指向已打开的QuaZip对象的指针，函数不会接管该对象的所有权，
 *                 调用者需确保zip对象在函数调用期间有效且处于打开状态。
 * @param[in] relatePath 文件在ZIP内的相对路径（如："doc/report.txt"），
 *                       路径格式不应以斜杠开头，目录层级使用正斜杠"/"分隔。
 * @param[in] localFilePath 待写入的本地文件路径，必须为有效可读的常规文件，
 *                          符号链接或其他特殊文件将被拒绝。
 * @param[in] chunk_mb 分块大小，默认为4mb
 *
 * @return bool
 *         - true  : 文件成功写入ZIP
 *         - false : 失败（可能原因：文件不存在/不可读、ZIP写入错误、内存不足等）
 * @note
 * - **分块策略**: 文件大小超过chunk_mb MB时启用分块写入，每次读取chunk_mb MB数据以减少内存峰值
 * - **元数据保留**: 使用原文件的修改时间和权限信息（通过QuaZipNewInfo实现）
 *
 * @example 基本用法示例
 * @code
 * QuaZip zip("archive.zip");
 * zip.open(QuaZip::mdCreate); // 必须确保zip已打开
 *
 * bool ok = DAZipArchive::writeFileToZip(&zip,"data/sample.txt","/home/user/sample.txt");
 *
 * if (ok) {
 *     qDebug() << "写入成功";
 * } else {
 *     qDebug() << "写入失败";
 * }
 * @endcode
 *
 * @warning 需确保:
 * - ZIP对象已通过open()方法打开且未关闭
 * - 写入过程中本地文件内容不被修改
 * - 多线程场景需自行处理同步
 *
 */
bool DAZipArchive::writeFileToZip(QuaZip* zip, const QString& relatePath, const QString& localFilePath, std::size_t chunk_mb)
{
	// 检查本地文件是否存在
	QFileInfo fileInfo(localFilePath);
	if (!fileInfo.exists() || !fileInfo.isFile()) {
		return false;
	}

	// 打开本地文件
	QFile localFile(localFilePath);
	if (!localFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	// 创建并打开zip中的文件条目
	QuaZipFile zipFile(zip);
	QuaZipNewInfo zipInfo(relatePath, localFilePath);  // 使用本地文件信息设置zip条目属性

	if (!zipFile.open(QIODevice::WriteOnly, zipInfo)) {
		localFile.close();
		return false;
	}

	// 获取文件大小并确定分块阈值
	const qint64 fileSize  = fileInfo.size();
	const qint64 chunkSize = chunk_mb * 1024 * 1024;  // chunk_mb MB
	bool success           = true;

	if (fileSize <= chunkSize) {
		// 小文件一次性读取
		QByteArray data = localFile.readAll();
		if (data.size() != fileSize || zipFile.write(data) != data.size()) {
			success = false;
		}
	} else {
		// 大文件分块读取
		QByteArray buffer(chunkSize, Qt::Uninitialized);
		qint64 totalBytesWritten = 0;

		while (!localFile.atEnd()) {
			qint64 bytesRead = localFile.read(buffer.data(), chunkSize);
			if (bytesRead < 0) {
				success = false;
				break;
			}

			qint64 bytesWritten = zipFile.write(buffer.constData(), bytesRead);
			if (bytesWritten != bytesRead) {
				success = false;
				break;
			}
			totalBytesWritten += bytesWritten;
		}

		// 验证总写入字节数
		if (totalBytesWritten != fileSize) {
			success = false;
		}
	}

	// 确保文件关闭
	localFile.close();
	zipFile.close();

	return success;
}

/**
 * @brief 从 ZIP 归档中提取指定文件到本地路径
 *
 * @param[in] zip 已打开的 QuaZip 实例指针，需确保在函数调用期间保持打开状态
 * @param[in] zipRelatePath ZIP 归档内的相对文件路径（例如："documents/report.pdf"）
 * @param[in] localFilePath 要写入的本地文件绝对路径（例如："/home/user/report.pdf"）
 * @param[in] chunk_mb 读写缓冲区大小（单位：MB），建议值 1-64，默认 4MB
 * @return bool
 * @retval true 文件提取成功
 * @retval false 提取失败，可能原因包括：
 *               - 参数无效
 *               - ZIP 内找不到指定文件
 *               - 磁盘空间不足
 *               - 文件读写错误
 *
 * @note 函数特性：
 * - 支持大文件处理（恒定内存占用）
 * - 自动创建目标目录结构
 * - 失败时自动清理不完整文件
 * - 线程安全性：非线程安全，需外部同步
 *
 * @warning
 * - 需确保传入的 QuaZip 实例已处于打开状态（QuaZip::mdUnzip 模式）
 * - chunk_mb 值过大会增加内存占用，过小会影响IO效率
 */
bool DAZipArchive::readToFile(QuaZip* zip, const QString& zipRelatePath, const QString& localFilePath, std::size_t chunk_mb)
{
	// 参数有效性检查 ---------------------------------------------------
	if (!zip || zipRelatePath.isEmpty() || localFilePath.isEmpty() || chunk_mb == 0) {
		return false;
	}

	// 计算实际缓冲区大小（防止整数溢出）
	constexpr std::size_t MAX_CHUNK_MB = 1024;  // 安全上限 1GB
	const std::size_t chunkSize        = static_cast< std::size_t >(qMin(chunk_mb, MAX_CHUNK_MB)) * 1024 * 1024;

	// 设置当前 ZIP 文件 ------------------------------------------------
	if (!zip->setCurrentFile(zipRelatePath)) {
		return false;
	}

	// 打开 ZIP 文件 ----------------------------------------------------
	QuaZipFile zipFile(zip);
	if (!zipFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	// 准备本地文件 -----------------------------------------------------
	QFileInfo fileInfo(localFilePath);
	if (!QDir().mkpath(fileInfo.absolutePath())) {  // 显式检查目录创建结果
		zipFile.close();
		return false;
	}

	QFile localFile(localFilePath);
	if (!localFile.open(QIODevice::WriteOnly)) {
		zipFile.close();
		return false;
	}

	// 初始化缓冲区（添加分配失败检查）-------------------------------------
	QByteArray buffer;
	buffer.resize(chunkSize);  // 比构造函数方式更安全
	if (buffer.size() != chunkSize) {
		zipFile.close();
		localFile.close();
		QFile::remove(localFilePath);
		return false;
	}

	// 分块读写操作 ------------------------------------------------------
	bool success     = true;
	qint64 bytesRead = 0;

	do {
		bytesRead = zipFile.read(buffer.data(), chunkSize);
		if (bytesRead < 0) {  // 读取错误
			success = false;
			break;
		}

		if (bytesRead > 0) {
			const qint64 bytesWritten = localFile.write(buffer.constData(), bytesRead);
			if (bytesWritten != bytesRead) {  // 写入不完整或错误
				success = false;
				break;
			}
		}
	} while (bytesRead > 0);

	// 资源清理 ----------------------------------------------------------
	zipFile.close();
	localFile.close();

	if (!success) {  // 失败时清理
		QFile::remove(localFilePath);
	}

	return success;
}

}  // end DA
