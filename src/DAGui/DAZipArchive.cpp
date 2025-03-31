#include "DAZipArchive.h"
#include <memory>
#include <QDebug>
#include <optional>
#include <QDateTime>
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
    // 重置zip的列表文件
    void resetZipFileList();
    // 把zip文件列表内容设置为无效
    void invalidZipFileList();

public:
    // 打开
    static bool open(QuaZipFile& zipFile, QIODevice::OpenMode mode, const QString relatePath);
    static const char* password();
    static int compressLevel();

public:
    std::unique_ptr< QuaZip > mZip;
    QString mLastErrorString;
    std::optional< QStringList > mFileList;
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
    if (!mFileList) {
        DAZipArchive::PrivateData* that = const_cast< DAZipArchive::PrivateData* >(this);
        that->resetZipFileList();
    }
    return mFileList.value();
}

void DAZipArchive::PrivateData::resetZipFileList()
{
    mFileList   = QStringList();
    QuaZip* zip = const_cast< QuaZip* >(mZip.get());
    // 先获取之前的文件
    QString currentFile = zip->getCurrentFileName();
    if (!zip->goToFirstFile()) {
        return;  // 如果没有文件，返回空列表
    }

    do {
        mFileList->append(zip->getCurrentFileName());
    } while (zip->goToNextFile());
    // 设置回之前的文件
    zip->setCurrentFile(currentFile);
    return;
}

void DAZipArchive::PrivateData::invalidZipFileList()
{
    if (mFileList) {
        mFileList = std::nullopt;
    }
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

bool DAZipArchive::open()
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
    d->invalidZipFileList();
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
    d->mZip.reset();  // 释放 QuaZip 对象

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
    d->invalidZipFileList();
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
    d->invalidZipFileList();
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
    d->invalidZipFileList();
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
 * @brief 获取文件数量，此函数会遍历zip的所有文件并缓存，第一次调用性能较低，如果有缓存（上次刷新文件后没有进行写操作），则不需要遍历
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

}  // end DA
