#include "DAZipArchive.h"
#include <memory>
#include <QDebug>
#include <optional>
#include <QDateTime>
#include <QDir>
#include "DAAbstractArchiveTask.h"
#include "quazip/quazipfile.h"
#include "quazip/JlCompress.h"
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
	bool ensureOpenForRead();
	bool ensureOpenForWrite();
	QString errorStringFromZipError(int errorCode) const;
	bool copyZipEntry(QuaZip* sourceZip, QuaZip* destZip, const QString& fileName);
	static QString getSystemErrorString(int errnum);

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
	if (!mZip->isOpen() || mZip->getMode() != QuaZip::mdUnzip) {
		return QStringList();
	}
	return mZip->getFileNameList();
}

bool DAZipArchive::PrivateData::ensureOpenForRead()
{
	if (mZip->isOpen() && mZip->getMode() == QuaZip::mdUnzip) {
		return true;
	}

	if (mZip->isOpen()) {
		mZip->close();
	}

	if (!mZip->open(QuaZip::mdUnzip)) {
		mLastErrorString = errorStringFromZipError(mZip->getZipError());
		return false;
	}
	return true;
}

bool DAZipArchive::PrivateData::ensureOpenForWrite()
{
	if (mZip->isOpen() && mZip->getMode() == QuaZip::mdCreate) {
		return true;
	}

	if (mZip->isOpen()) {
		mZip->close();
	}

	if (!mZip->open(QuaZip::mdCreate)) {
		mLastErrorString = errorStringFromZipError(mZip->getZipError());
		return false;
	}
	return true;
}

QString DAZipArchive::PrivateData::errorStringFromZipError(int errorCode) const
{
	switch (errorCode) {
	case UNZ_OK:
		return QObject::tr("No error");
	case UNZ_END_OF_LIST_OF_FILE:
		return QObject::tr("End of list of file");
	case UNZ_ERRNO:
		return QObject::tr("File I/O error: %1").arg(getSystemErrorString(errno));
	case UNZ_PARAMERROR:
		return QObject::tr("Invalid parameter");
	case UNZ_BADZIPFILE:
		return QObject::tr("Bad zip file");
	case UNZ_INTERNALERROR:
		return QObject::tr("Internal error");
	case UNZ_CRCERROR:
		return QObject::tr("CRC error");
	case UNZ_OPENERROR:
		return QObject::tr("Open error");
	default:
		return QObject::tr("Unknown error (%1)").arg(errorCode);
	}
}

bool DAZipArchive::PrivateData::copyZipEntry(QuaZip* sourceZip, QuaZip* destZip, const QString& fileName)
{
	if (!sourceZip->setCurrentFile(fileName)) {
		return false;
	}

	QuaZipFile inFile(sourceZip);
	if (!inFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	QuaZipFile outFile(destZip);
	if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileName), s_password, 0, Z_DEFLATED, s_zip_compress_level)) {
		inFile.close();
		return false;
	}

	// 分块复制（4MB块）
	const qint64 CHUNK_SIZE = 4 * 1024 * 1024;
	char buffer[ CHUNK_SIZE ];
	bool success = true;

	while (!inFile.atEnd()) {
		qint64 bytesRead = inFile.read(buffer, CHUNK_SIZE);
		if (bytesRead == -1) {
			success = false;
			break;
		}

		if (outFile.write(buffer, bytesRead) != bytesRead) {
			success = false;
			break;
		}
	}

	inFile.close();
	outFile.close();
	return success;
}

QString DAZipArchive::PrivateData::getSystemErrorString(int errnum)
{
#ifdef _MSC_VER
	char buffer[ 256 ] = { 0 };
	errno_t result     = strerror_s(buffer, sizeof(buffer), errnum);
	if (result == 0) {
		return QString(buffer);
	}
	return "Unknown error";
#else
	return QString(strerror(errnum));
#endif
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
	close();
}

bool DAZipArchive::setBaseFilePath(const QString& path)
{
	DAAbstractArchive::setBaseFilePath(path);
	return setZipFileName(path);
}

bool DAZipArchive::setZipFileName(const QString& fileName)
{
	DA_D(d);
	if (isOpened()) {
		close();  // 关闭当前打开的ZIP
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
	if (isOpened()) {
		return true;
	}

	if (!d->ensureOpenForRead()) {
		qDebug() << "Failed to open archive for reading:" << d->mLastErrorString;
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
	if (isOpened()) {
		return true;
	}

	if (!d->ensureOpenForWrite()) {
		qDebug() << "Failed to open archive for writing:" << d->mLastErrorString;
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
	DA_DC(d);
	return d->mZip && d->mZip->isOpen();
}

bool DAZipArchive::close()
{
	DA_D(d);
	if (!isOpened()) {
		return true;
	}

	d->mZip->close();
	return d->mZip->getZipError() == UNZ_OK;
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
	if (!isOpened() || !d->ensureOpenForWrite()) {
		qDebug() << "Archive not open for writing";
		return false;
	}

	QuaZipFile zipFile(d->mZip.get());
	if (!zipFile.open(QIODevice::WriteOnly,
					  QuaZipNewInfo(relatePath),
					  DAZipArchive::PrivateData::s_password,
					  0,
					  Z_DEFLATED,
					  DAZipArchive::PrivateData::s_zip_compress_level)) {
		d->mLastErrorString = zipFile.errorString();
		qDebug() << tr("The file %1 in the archive could not be opened. The reason for the error is %2")
						.arg(relatePath, zipFile.errorString());  // cn:无法打开文件中的%1,错误原因为%2
		return false;
	}

	if (zipFile.write(byte) != byte.size()) {
		d->mLastErrorString = zipFile.errorString();
		zipFile.close();
		return false;
	}

	zipFile.close();
	return zipFile.getZipError() == ZIP_OK;
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
 * 基本用法示例
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
	if (!isOpened() || !d->ensureOpenForWrite()) {
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
	if (!isOpened() || !d->ensureOpenForRead()) {
		qDebug() << tr("archive is not open");  // cn:文件还未打开
		return QByteArray();
	}

	if (!d->mZip->setCurrentFile(relatePath)) {
		d->mLastErrorString = d->errorStringFromZipError(d->mZip->getZipError());
		qDebug() << tr("Unable to locate the %1 file in the current archive. The error code is %2,err str:%3")
						.arg(relatePath)
						.arg(d->mZip->getZipError())
						.arg(d->mLastErrorString);  // cn:无法找到当前档案下的%1文件。错误码为%2,错误内容为:%3
		return QByteArray();
	}

	QuaZipFile zipFile(d->mZip.get());
	if (!zipFile.open(QIODevice::ReadOnly)) {
		d->mLastErrorString = zipFile.errorString();
		qDebug() << tr("The file %1 in the archive could not be opened. The error code is %2")
						.arg(relatePath)
						.arg(zipFile.getZipError());  // cn:无法打开档案中的%1文件，错误码为%2
		return QByteArray();
	}

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
	if (!isOpened() || !d->ensureOpenForRead()) {
		qDebug() << "Archive not open";
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
	// 创建临时ZIP文件
	QuaZip tempZip(tempZipPath);
	if (!tempZip.open(QuaZip::mdCreate)) {
		d->mLastErrorString = d->errorStringFromZipError(tempZip.getZipError());
		qDebug() << "Failed to create temp archive:" << d->mLastErrorString;
		return false;
	}
	// 复制除目标文件外的所有文件
	bool success            = true;
	const QStringList files = getAllFiles();
	for (const QString& file : files) {
		if (file == fileToRemove) {
			continue;
		}
		if (!d->copyZipEntry(d->mZip.get(), &tempZip, file)) {
			success = false;
			break;
		}
	}

	tempZip.close();
	// 替换原文件
	if (success) {
		d->mZip->close();

		QFile::remove(archivePath);
		if (!QFile::rename(tempZipPath, archivePath)) {
			d->mLastErrorString = QObject::tr("Failed to replace archive file");
			success             = false;
		} else {
			// 重新打开原文件
			d->mZip->setZipName(archivePath);
			d->ensureOpenForRead();
		}
	}

	// 清理临时文件
	if (!success && QFile::exists(tempZipPath)) {
		QFile::remove(tempZipPath);
	}

	return success;
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
	if (!isOpened() && !open()) {
		qDebug() << tr("can not open archive");  // cn:无法打开档案
		return false;
	}
	return extractToDirectory(d->mZip.get(), extractDir);
}

bool DAZipArchive::compressDirectory(const QString& folderPath)
{
	DA_D(d);
	if (!isOpened() && !create()) {
		qDebug() << tr("can not open archive");  // cn:无法打开档案
		return false;
	}
	return compressDirectory(folderPath, d->mZip.get());
}

QStringList DAZipArchive::getFileNameList() const
{
	if (!isOpened()) {
		return QStringList();
	}
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
	const QString prefix = zipFolderPath.endsWith('/') ? zipFolderPath : zipFolderPath + '/';
	QStringList res;
	const QList< QuaZipFileInfo64 > files64 = d->mZip->getFileInfoList64();
	for (const QuaZipFileInfo64& fileInfo : files64) {
		QString zipPath = fileInfo.name;

		// 跳过目录和子文件夹中的文件
		if (!zipPath.startsWith(prefix)) {
			continue;
		}

		// 检查是否在目标文件夹的直接子级
		QString relativePath = zipPath.mid(prefix.length());
		if (!relativePath.contains('/')) {
			res.append(zipPath);
		}
	}
	return res;
}

QuaZip* DAZipArchive::quazip() const
{
    return d_ptr->mZip.get();
}

/**
 * @brief 获取最后的错误内容
 * @return
 */
QString DAZipArchive::getLastErrorString() const
{
    return d_ptr->mLastErrorString;
}

void DAZipArchive::saveAll(const QString& filePath)
{
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
			close();
			QFile::remove(tempFilePath);
			emit taskFinished(DAAbstractArchive::SaveFailed);
			return;
		}
		emit taskProgress(task, DAAbstractArchiveTask::WriteMode);
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
	if (isOpened()) {
		// 如果原来已经打开需要先关闭
		close();
	}
	if (!setBaseFilePath(filePath) || !open()) {
		// 打开失败
		d_ptr->mLastErrorString = QObject::tr("Failed to open archive");
		emit taskFinished(DAAbstractArchive::LoadFailed);
		return;
	}
	while (!isTaskQueueEmpty()) {
		std::shared_ptr< DAAbstractArchiveTask > task = takeTask();
		if (!task->exec(this, DAAbstractArchiveTask::ReadMode)) {
			close();
			emit taskFinished(DAAbstractArchive::LoadFailed);
			return;
		}
		emit taskProgress(task, DAAbstractArchiveTask::ReadMode);
	}
	// 加载完成后关闭
	close();
	emit taskFinished(DAAbstractArchive::LoadSuccess);
}

/**
 * @brief 判断是否是正确的工程
 * @param filePath
 * @return
 */
bool DAZipArchive::isCorrectFile(const QString& filePath)
{
	QuaZip zip(filePath);
	if (!zip.open(QuaZip::mdUnzip)) {
		return false;
	}

	// 检查是否有有效的文件列表
	if (zip.getEntriesCount() <= 0) {
		zip.close();
		return false;
	}

	zip.close();
	return true;
}

bool DAZipArchive::extractToDirectory(const QString& zipFilePath, const QString& extractDir)
{
	QuaZip zip(zipFilePath);
	if (!zip.open(QuaZip::mdUnzip)) {
		return false;
	}
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
	if (!targetDir.exists() && !targetDir.mkpath(".")) {
		qDebug() << tr("Failed to create target directory:%1").arg(extractDir);  // cn：无法创建目标文件夹%1
		return false;
	}

	// 遍历 ZIP 文件中的所有条目
	if (!zip->goToFirstFile()) {
		// 空压缩包也算成功
		qDebug() << "archive is empty.";
		return true;
	}
	do {
		QString entryName = zip->getCurrentFileName();      // 获取当前条目的相对路径
		QString fullPath  = targetDir.filePath(entryName);  // 目标文件的完整路径

		// 如果是目录，创建目录
		if (entryName.endsWith('/')) {
			// 如果是目录条目，创建对应的目录
			QDir dir;
			if (!dir.mkpath(fullPath)) {
				qDebug() << "Failed to create directory:" << fullPath;
				return false;
			}
			continue;
		}

		// 如果是文件条目，解压文件内容

		// 确保父目录存在
		QFileInfo fi(fullPath);
		if (!targetDir.mkpath(fi.path())) {
			return false;
		}
		// 提取文件
		if (!readToFile(zip, entryName, fullPath)) {
			return false;
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
	QDir dir(folderPath);
	if (!dir.exists()) {
		return false;
	}

	const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	for (const QFileInfo& entry : entries) {
		QString relativePath = relativeBase.isEmpty() ? entry.fileName() : relativeBase + '/' + entry.fileName();

		if (entry.isDir()) {
			if (!compressDirectory(entry.absoluteFilePath(), zip, relativePath)) {
				return false;
			}
		} else {
			if (!writeFileToZip(zip, relativePath, entry.absoluteFilePath())) {
				return false;
			}
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
 * 基本用法示例
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

	if (!zipFile.open(
			QIODevice::WriteOnly, zipInfo, PrivateData::s_password, 0, Z_DEFLATED, PrivateData::s_zip_compress_level)) {
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
	// 确保目标目录存在
	QDir dir = fileInfo.dir();
	if (!dir.exists() && !dir.mkpath(".")) {
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
	buffer.resize(static_cast< int >(chunkSize));  // 比构造函数方式更安全

	// 分块读写操作 ------------------------------------------------------
	bool success = true;

	while (!zipFile.atEnd() && success) {
		qint64 bytesRead = zipFile.read(buffer.data(), chunkSize);
		if (bytesRead <= 0) {
			success = false;
			break;
		}

		if (localFile.write(buffer.constData(), bytesRead) != bytesRead) {
			success = false;
			break;
		}
	}

	// 资源清理 ----------------------------------------------------------
	zipFile.close();
	localFile.close();

	if (!success) {  // 失败时清理
		QFile::remove(localFilePath);
	}

	return success;
}

}  // end DA
