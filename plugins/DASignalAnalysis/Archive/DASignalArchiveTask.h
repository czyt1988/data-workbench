#ifndef DASIGNALARCHIVETASK_H
#define DASIGNALARCHIVETASK_H
#include <QByteArray>
#include <QList>
#include "Archive/DAAbstractArchiveTask.h"
#include "Archive/DAChartItemArchive.h"
#include "Chart/DAChartItemsManager.h"

namespace DA
{
/**
 * @brief 信号分析工程的存档任务（方案 §10.1 / T9）
 *
 * 一个任务里读写三部分：
 * - signal-analysis/signals.json：信号来源引用、分析参数与视图状态（原有条目）；
 * - signal-analysis/figures.xml：figure XML（网格 + 子图 + 轴/样式 + 非 transient 图元 key）；
 * - signal-analysis/chart-data：非 transient 图元的二进制数据（DAChartItemArchive）。
 *
 * 任务在后台线程执行，只做文件读写，不触碰界面；界面重建在主线程的 loadedCallBack 中完成。
 * figure XML 与 chart-data 只在「有 figure 内容」时写入，保证旧工程（无 figures.xml）可正常加载。
 *
 * BLK-01：写模式携带的是 GUI 线程预序列化的图元**字节快照**（DAChartItemSnapshot），
 * 不是 DAChartItemsManager——保存期间 GUI 线程（嵌套事件循环继续派发定时器）可能
 * 删除/重建峰值标记等图元，后台线程若仍对 QwtPlotItem* 逐个 serializeOut 会产生
 * use-after-free。快照在 createArchiveTask(true) 的 GUI 线程侧生成，后台只写字节。
 * 读模式仍读出到 DAChartItemsManager（恢复端按 key 取回图元，主线程使用）。
 */
class DASignalArchiveTask : public DAAbstractArchiveTask
{
public:
    // 保存模式：signals.json 字节 + figures.xml 字节 + 图元快照（GUI 线程预序列化字节，值语义）
    explicit DASignalArchiveTask(const QByteArray& signalsJson = QByteArray(),
                                 const QByteArray& figuresXml = QByteArray(),
                                 const QList< DAChartItemSnapshot >& itemSnapshots = QList< DAChartItemSnapshot >());
    ~DASignalArchiveTask() override;

    // 执行读写
    bool exec(DAAbstractArchive* archive, Mode mode) override;

    // 读取到的 signals.json 字节（ReadMode 执行后有效）
    QByteArray loadedData() const;

    // 工程包里是否真的存在本插件的 signals.json 条目（ReadMode 执行后有效）
    // 说明：read() 对「条目缺失」返回空 QByteArray 且不报错，单看 exec() 的返回值
    // 无法区分「没有本插件数据」和「无需处理」，界面层据此清理上一工程残留（评审 N-U2）
    bool archiveEntryFound() const;

    // 读取到的 figures.xml 字节（ReadMode 执行后有效）
    QByteArray loadedFiguresXml() const;

    // 工程包里是否存在 figures.xml 条目（ReadMode 执行后有效；旧工程为 false）
    bool figuresEntryFound() const;

    // 图元管理者（读模式执行后为读出的非 transient 图元；写模式不再使用——见类注释 BLK-01）
    const DAChartItemsManager& chartItemsManager() const;
    DAChartItemsManager& chartItemsManager();

    // 存档内的相对路径
    static QString archivePath();
    // figures.xml 的相对路径
    static QString figuresArchivePath();
    // chart-data 目录的存档内前缀
    static QString chartDataPrefix();

private:
    QByteArray mData;
    bool mEntryFound { false };
    QByteArray mFiguresXml;
    bool mFiguresEntryFound { false };
    // 写模式输入：GUI 线程预序列化的图元字节快照（后台线程只写字节，不触碰 QwtPlotItem*，BLK-01）
    QList< DAChartItemSnapshot > mItemSnapshots;
    // 读模式输出：从 chart-data 读回的图元管理者（主线程恢复用）
    DAChartItemsManager mItemsManager;
};
}  // namespace DA

#endif  // DASIGNALARCHIVETASK_H
