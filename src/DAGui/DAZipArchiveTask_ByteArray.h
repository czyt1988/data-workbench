#ifndef DAZIPARCHIVETASK_BYTEARRAY_H
#define DAZIPARCHIVETASK_BYTEARRAY_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QString>
#include "DAAbstractArchiveTask.h"
namespace DA
{
/**
 * @brief 保存/加载Byte任务
 */
class DAGUI_API DAZipArchiveTask_ByteArray : public DAAbstractArchiveTask
{
public:
    DAZipArchiveTask_ByteArray();
    // 读取任务
    DAZipArchiveTask_ByteArray(const QString& path);
    // 写任务
    DAZipArchiveTask_ByteArray(const QString& path, const QByteArray& byte);
    ~DAZipArchiveTask_ByteArray();
    // 设置保存到zip的相对路径
    void setPath(const QString& path);
    QString getPath() const;
    // 数据操作
    QByteArray getData() const;
    void setData(const QByteArray& data);
    //
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;

private:
    QString mPath;
    QByteArray mData;
};

}
#endif  // DAZIPARCHIVETASK_BYTEARRAY_H
