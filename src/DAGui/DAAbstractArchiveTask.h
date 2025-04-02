#ifndef DAABSTRACTARCHIVETASK_H
#define DAABSTRACTARCHIVETASK_H
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
class DAAbstractArchiveTask
{
public:
    DAAbstractArchiveTask();
    virtual ~DAAbstractArchiveTask();

    /**
     * @brief 执行任务
     * @param archive 传入的档案基类
     * @return 任务执行成功失败的返回
     * @note 注意任务的执行是在别的线程中，不要在任务中操作ui
     */
    virtual bool exec(DAAbstractArchive* archive) = 0;
};
}  // end DA
#endif  // DAABSTRACTARCHIVETASK_H
