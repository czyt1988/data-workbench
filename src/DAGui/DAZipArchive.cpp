#include "DAZipArchive.h"
#include <memory>
#include <QDebug>
#include <optional>
#include <QDateTime>
#include <QDir>
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
    static bool open(QuaZipFile& zipFile, QIODevice::OpenMode mode, const QString relatePath);
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

bool DAZipArchive::PrivateData::open(QuaZipFile& zipFile, QIODevice::OpenMode mode, const QString relatePath)
{
    return zipFile.open(mode, QuaZipNewInfo(relatePath), password(), 0, Z_DEFLATED, compressLevel());
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

void DAZipArchive::setBaseFilePath(const QString& path)
{
	DAAbstractArchive::setBaseFilePath(path);
	setZipFileName(path);
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
        qDebug() << tr("%1 is already opened").arg(filePath);  // cn:当前文件%1已经打开;
        return true;
    }
    d->makeSureZipPtr();
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
        qDebug() << tr("%1 is already opened").arg(filePath);  // cn:当前文件%1已经打开;
        return true;
    }
    d->makeSureZipPtr();
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
    if (!DAZipArchive::PrivateData::open(zipFile, QIODevice::WriteOnly,
                                         relatePath)) {  // 设置压缩级别为 0
        d->mLastErrorString = zipFile.errorString();
        qDebug() << tr("Failed to open file in archive:").arg(relatePath);  // cn:无法打开文件中的%1
        return false;
    }

    // 写入数据
    zipFile.write(byte);
    zipFile.close();
    return true;
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
    QuaZipFile zipFile(d->mZip.get());

    if (!DAZipArchive::PrivateData::open(zipFile, QIODevice::WriteOnly,
                                         relatePath)) {  // 设置压缩级别为 0
        d->mLastErrorString = zipFile.errorString();
        qDebug() << tr("Failed to open file in archive:").arg(relatePath);  // cn:无法打开文件中的%1
        return QByteArray();
    }

    // 读取数据
    QByteArray data = zipFile.readAll();
    zipFile.close();

    return data;
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
            if (!DAZipArchive::PrivateData::open(outFile, QIODevice::WriteOnly, currentFileName)) {
                qDebug() << tr("open %1 to temp file error").arg(currentFileName);  // cn:打开临时文件%1失败
                d->mLastErrorString = tr("open %1 to temp file error").arg(currentFileName);  // cn:打开临时文件%1失败
                newZip->close();
                zip->setCurrentFile(oldCurrentFile);
                return false;
            }
            // 把具体内容读取
            if (!DAZipArchive::PrivateData::open(inFile, QIODevice::ReadOnly, currentFileName)) {
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
            if (DAZipArchive::PrivateData::open(zipFile, QIODevice::ReadOnly, entryName)) {
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
            if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(relativePath))) {
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

}  // end DA
