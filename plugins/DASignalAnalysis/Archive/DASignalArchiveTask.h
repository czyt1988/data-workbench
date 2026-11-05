#ifndef DASIGNALARCHIVETASK_H
#define DASIGNALARCHIVETASK_H
#include <QByteArray>
#include "Archive/DAAbstractArchiveTask.h"

namespace DA
{
/**
 * @brief 信号分析工程的存档任务（方案 §10.1）
 *
 * 只存信号来源引用、分析参数与视图状态（JSON），不存分析结果、不存 STFT 矩阵。
 * 任务在后台线程执行，只做文件读写，不触碰界面；界面重建在主线程的 loadedCallBack 中完成。
 */
class DASignalArchiveTask : public DAAbstractArchiveTask
{
public:
    // 保存模式：传入待写入的 JSON 字节
    explicit DASignalArchiveTask(const QByteArray& saveData = QByteArray());
    ~DASignalArchiveTask() override;

    // 执行读写
    bool exec(DAAbstractArchive* archive, Mode mode) override;

    // 读取到的 JSON 字节（ReadMode 执行后有效）
    QByteArray loadedData() const;

    // 存档内的相对路径
    static QString archivePath();

private:
    QByteArray mData;
};
}  // namespace DA

#endif  // DASIGNALARCHIVETASK_H
