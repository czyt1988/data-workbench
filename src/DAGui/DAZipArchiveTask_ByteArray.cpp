#include "DAZipArchiveTask_ByteArray.h"
#include "DAZipArchive.h"
#include <QDebug>
namespace DA
{
//----------------------------------------------------
// DAZipArchiveSaveByteArrayTask
//----------------------------------------------------

DAZipArchiveTask_ByteArray::DAZipArchiveTask_ByteArray() : DAAbstractArchiveTask()
{
}

DAZipArchiveTask_ByteArray::DAZipArchiveTask_ByteArray(const QString& path) : DAAbstractArchiveTask(), mPath(path)
{
}

DAZipArchiveTask_ByteArray::DAZipArchiveTask_ByteArray(const QString& path, const QByteArray& byte)
    : DAAbstractArchiveTask(), mPath(path), mData(byte)
{
}

DAZipArchiveTask_ByteArray::~DAZipArchiveTask_ByteArray()
{
}

void DAZipArchiveTask_ByteArray::setPath(const QString& path)
{
    mPath = path;
}

QString DAZipArchiveTask_ByteArray::getPath() const
{
    return mPath;
}

QByteArray DAZipArchiveTask_ByteArray::getData() const
{
    return mData;
}

void DAZipArchiveTask_ByteArray::setData(const QByteArray& data)
{
    mData = data;
}

bool DAZipArchiveTask_ByteArray::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
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
        if (!zip->write(mPath, mData)) {
            qDebug() << QString("write data to \"%1\" error").arg(mPath);
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
        mData = zip->read(mPath);
        if (mData.isEmpty()) {
            qDebug() << QString("can not read %1 from %2").arg(mPath, zip->getBaseFilePath());
            return false;
        }
    }
    return true;
}

}  // end DA
