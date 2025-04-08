#include "DAZipArchiveTask_Xml.h"
#include "DAZipArchive.h"
#include <QDebug>
namespace DA
{

DAZipArchiveTask_Xml::DAZipArchiveTask_Xml() : DAAbstractArchiveTask()
{
}

DAZipArchiveTask_Xml::DAZipArchiveTask_Xml(const QString& path)
{
    setPath(path);
}

DAZipArchiveTask_Xml::DAZipArchiveTask_Xml(const QString& path, const QDomDocument& doc) : DAAbstractArchiveTask()
{
    setPath(path);
    setDomDocument(doc);
}

DAZipArchiveTask_Xml::~DAZipArchiveTask_Xml()
{
}

QDomDocument DAZipArchiveTask_Xml::getDomDocument() const
{
    return mDomDocument;
}

void DAZipArchiveTask_Xml::setDomDocument(const QDomDocument& domDocument)
{
    mDomDocument = domDocument;
}

QString DAZipArchiveTask_Xml::getPath() const
{
    return mPath;
}

void DAZipArchiveTask_Xml::setPath(const QString& path)
{
    mPath = path;
}

bool DAZipArchiveTask_Xml::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
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
        QByteArray data = mDomDocument.toByteArray();
        if (data.isEmpty()) {
            qDebug() << QString("create archive error:%1,get null xml data").arg(zip->getBaseFilePath());
            return false;
        }
        if (!zip->write(mPath, data)) {
            qDebug() << QString("write data to \"%1\" error").arg(mPath);
            return false;
        }
    } else {
        // 读取数据模式
        // 读取数据模式
        if (!zip->isOpened()) {
            if (!zip->open()) {
                qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
                return false;
            }
        }
        QByteArray data = zip->read(mPath);
        if (data.isEmpty()) {
            qDebug() << QString("can not read %1 from %2").arg(mPath, zip->getBaseFilePath());
            return false;
        }
        if (!mDomDocument.setContent(data, &mLastErrorString)) {
            qDebug() << QString("can not read %1 from %2,last error string is %3").arg(mPath, zip->getBaseFilePath(), mLastErrorString);
            return false;
        }
    }
    return true;
}

QString DAZipArchiveTask_Xml::getLastErrorString() const
{
    return mLastErrorString;
}

void DAZipArchiveTask_Xml::setLastErrorString(const QString& lastErrorString)
{
    mLastErrorString = lastErrorString;
}

}
