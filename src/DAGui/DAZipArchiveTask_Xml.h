#ifndef DAZIPARCHIVETASK_XML_H
#define DAZIPARCHIVETASK_XML_H
#include "DAGuiAPI.h"
#include <QDomDocument>
#include <QObject>
#include <QString>
#include "DAAbstractArchiveTask.h"
namespace DA
{
/**
 * @brief 保存/加载xml文件的任务
 */
class DAGUI_API DAZipArchiveTask_Xml : public DAAbstractArchiveTask
{
public:
    DAZipArchiveTask_Xml();
    // 加载构造
    DAZipArchiveTask_Xml(const QString& path);
    // 保存构造
    DAZipArchiveTask_Xml(const QString& path, const QDomDocument& doc);
    ~DAZipArchiveTask_Xml();
    // xml文档
    QDomDocument getDomDocument() const;
    void setDomDocument(const QDomDocument& domDocument);
    // 路径
    QString getPath() const;
    void setPath(const QString& path);
    // 错误信息
    QString getLastErrorString() const;
    void setLastErrorString(const QString& lastErrorString);
    //
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;

private:
    QString mPath;
    QDomDocument mDomDocument;
    QString mLastErrorString;
};
}

#endif  // DAZIPARCHIVETASK_XML_H
