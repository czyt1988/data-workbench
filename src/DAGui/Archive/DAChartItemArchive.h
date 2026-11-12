#ifndef DACHARTITEMARCHIVE_H
#define DACHARTITEMARCHIVE_H
#include "DAGuiAPI.h"
#include <QString>
namespace DA
{
class DAAbstractArchive;
class DAChartItemsManager;

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
    // 索引文件名
    static QString itemsIndexFileName();
};
}  // namespace DA

#endif  // DACHARTITEMARCHIVE_H
