#ifndef DACHARTITEMARCHIVE_H
#define DACHARTITEMARCHIVE_H
#include "DAGuiAPI.h"
#include <QByteArray>
#include <QList>
#include <QString>
namespace DA
{
class DAAbstractArchive;
class DAChartItemsManager;

/**
 * @brief 预序列化图元快照
 *
 * GUI 线程生成，后台存档任务只写字节，不触碰 QwtPlotItem*：
 * 保存期间 GUI 线程（嵌套事件循环继续派发定时器）可能删除/重建图元，
 * 后台线程若仍对 QwtPlotItem* 逐个 serializeOut 会产生 use-after-free（BLK-01）。
 */
struct DAChartItemSnapshot
{
    QString key;       ///< 图元 key（与 DAChartItemsManager 登记一致）
    int rtti { 0 };    ///< QwtPlotItem::rtti()
    QByteArray data;   ///< DAChartItemSerialize::serializeOut 的字节
};

/**
 * @brief chart-data（图元数据）归档助手
 *
 * 把「chart-items.xml 索引 + 每个图元一个二进制文件」的读写从
 * @ref DAZipArchiveTask_ChartItem 抽出，供工程存档与插件存档（信号分析等）复用：
 *
 * @code
 * // 保存（通常在存档任务的 exec 内调用，不能在GUI线程直接写zip）
 * DAChartItemArchive::save(archive, &itemsMgr, QStringLiteral("signal-analysis/chart-data"));
 * // 加载（必须先于 figure/charts 的 XML 加载，xml 里只记 key）
 * DAChartItemArchive::load(archive, &itemsMgr, QStringLiteral("signal-analysis/chart-data"));
 * @endcode
 *
 * prefix 是相对存档根的目录，例如工程存档的 @c chart-data。
 *
 * 跨线程保存用快照两段式：GUI 线程 makeSnapshots() 先把图元序列化成字节，
 * 后台任务只调 saveSnapshots() 写字节，不再解引用任何 QwtPlotItem*。
 */
class DAGUI_API DAChartItemArchive
{
public:
    // 写 chart-items.xml 索引 + 每图元二进制（单图元失败记录并继续、整体 true；索引写失败 false）
    static bool save(DAAbstractArchive* archive,
                     DAChartItemsManager* itemsMgr,
                     const QString& prefix,
                     QString* errorMessage = nullptr);
    // 读索引与图元（索引缺失 true；索引存在但为空/损坏 false；单图元失败跳过）
    static bool load(DAAbstractArchive* archive,
                     DAChartItemsManager* itemsMgr,
                     const QString& prefix,
                     QString* errorMessage = nullptr);
    // 在 GUI 线程调用：把管理者里全部图元序列化为快照（单图元失败跳过并 qWarning，与 save 的容错口径一致）
    static QList< DAChartItemSnapshot > makeSnapshots(const DAChartItemsManager* itemsMgr);
    // 后台线程调用：写 chart-items.xml 索引 + 每快照一个 <prefix>/<key> 文件；输出与 save() 字节等价
    static bool saveSnapshots(DAAbstractArchive* archive,
                              const QList< DAChartItemSnapshot >& snapshots,
                              const QString& prefix,
                              QString* errorMessage = nullptr);
    // 索引文件名
    static QString itemsIndexFileName();
};
}  // namespace DA

#endif  // DACHARTITEMARCHIVE_H
