#ifndef DAABSTRACTARCHIVETASK_H
#define DAABSTRACTARCHIVETASK_H
#include "DAGuiAPI.h"
#include <QObject>
namespace DA
{
class DAAbstractArchive;
/**
 * @brief 针对@sa DAZipArchiveThreadWrapper 的任务
 *
 * @sa DAZipArchiveThreadWrapper 内部有个任务队列，在保存和加载需要推入任务，具体执行时会逐个任务进行执行，
 * 有特殊的读取和写入需求时，只要派生一个特殊的任务即可
 *
 * @note 注意任务的执行是在别的线程中，不要在任务中操作ui
 */
class DAGUI_API DAAbstractArchiveTask
{
public:
    /**
     * @brief 模式用来区分读写
     */
    enum Mode
    {
        ReadMode,
        WriteMode
    };
    DAAbstractArchiveTask();
    virtual ~DAAbstractArchiveTask();

    /**
     * @brief 执行任务
     * @param archive 传入的档案基类
     * @param mode 写或读模式
     * @return 任务执行成功失败的返回
     * @note 注意任务的执行是在别的线程中，不要在任务中操作ui
     *
     * 示例
     * @code
     *bool DAZipArchiveTask_ByteArray::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
     *{
     *    if (!archive) {
     *        return false;
     *    }
     *    DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
     *    if (mode == DAAbstractArchiveTask::WriteMode) {
     *        // 写模式
     *        if (!zip->isOpened()) {
     *            if (!zip->create()) {
     *                qDebug() << QString("create archive error:%1").arg(zip->getBaseFilePath());
     *                return false;
     *            }
     *        }
     *        if (!zip->write(mPath, mData)) {
     *            qDebug() << QString("write data to \"%1\" error").arg(mPath);
     *            return false;
     *        }
     *    } else {
     *        // 读取数据模式
     *        if (!zip->isOpened()) {
     *            if (!zip->open()) {
     *                qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
     *                return false;
     *            }
     *        }
     *        mData = zip->read(mPath);
     *        if (mData.isEmpty()) {
     *            qDebug() << QString("can not read %1 from %2").arg(mPath, zip->getBaseFilePath());
     *            return false;
     *        }
     *    }
     *    return true;
     *}
     * @endcode
     */
    virtual bool exec(DAAbstractArchive* archive, Mode mode) = 0;

    /**
     * @brief 获取流水号
     *
     * 流水号是为了方便知道这个任务的内容，用户可以设置任意数字，只要自己程序知道具体数字含义是什么即可
     * @return
     */
    int getCode() const;

    /**
     * @brief 设置流水号
     *
     * 流水号是为了方便知道这个任务的内容，用户可以设置任意数字，只要自己程序知道具体数字含义是什么即可
     * @param code
     */
    void setCode(int code);

    // 任务名称
    QString getName() const;
    void setName(const QString& name);

    // 任务描述
    QString getDescribe() const;
    void setDescribe(const QString& describe);

private:
    int mCode { 0 };
    QString mName;
    QString mDescribe;
};
}  // end DA
Q_DECLARE_METATYPE(DA::DAAbstractArchiveTask::Mode)
Q_DECLARE_METATYPE(std::shared_ptr< DA::DAAbstractArchiveTask >)
#endif  // DAABSTRACTARCHIVETASK_H
