#ifndef DACHART3DITEMARCHIVE_H
#define DACHART3DITEMARCHIVE_H
#include "DAGuiAPI.h"
#include <QByteArray>
#include <QList>
#include <QString>
namespace DA
{
class DAAbstractArchive;
class DAChart3DItemsManager;
class DAChart3DWidget;

/**
 * @brief 预序列化的三维图元快照
 *
 * GUI 线程生成，后台存档任务只写字节，不触碰 Qwt3DPlotItem*：
 * 保存期间 GUI 线程（嵌套事件循环继续派发定时器）可能删除/重建图元，
 * 后台线程若仍对 Qwt3DPlotItem* 逐个 serializeOut 会产生 use-after-free（与二维 chart-data 的
 * BLK-01 同一类问题）。
 */
struct DAChart3DItemSnapshot
{
    QString key;       ///< 图元 key（与 DAChart3DItemsManager 登记一致）
    int rtti { 0 };    ///< Qwt3DPlotItem::rtti()
    QByteArray data;   ///< DAChart3DItemSerialize::serializeOut 的字节
};

/**
 * @brief chart3d-data（三维图元数据）归档助手
 *
 * 与二维的 @ref DAChartItemArchive 对称：把「chart3d-items.xml 索引 + 每个 3D 图元一个二进制文件」
 * 的读写抽出来，供工程存档（以及需要 3D 图元的其它宿主）复用：
 *
 * @code
 * // 保存（通常在存档任务的 exec 内调用，不能在 GUI 线程直接写 zip）
 * DAChart3DItemArchive::save(archive, &items3dMgr, QStringLiteral("chart3d-data"));
 * // 加载（必须先于 charts.xml 的 XML 加载，xml 里只记 key）
 * DAChart3DItemArchive::load(archive, &items3dMgr, QStringLiteral("chart3d-data"));
 * @endcode
 *
 * 跨线程保存用快照两段式：GUI 线程 makeSnapshots() 先把图元序列化成字节，
 * 后台任务只调 saveSnapshots() 写字节，不再解引用任何 Qwt3DPlotItem*。
 *
 * @note 内部只走 @ref DAChart3DItemSerialize，不复用 Qwt3DPlot 的 plot 级 operator<</>>（那条路径
 *       有既知的顺序/缓存问题，且会把整个子图当成一个块）。
 */
class DAGUI_API DAChart3DItemArchive
{
public:
    // 写 chart3d-items.xml 索引 + 每图元二进制（单图元失败记录并继续、整体 true；索引写失败 false）
    static bool save(DAAbstractArchive* archive,
                     DAChart3DItemsManager* itemsMgr,
                     const QString& prefix,
                     QString* errorMessage = nullptr);
    // 读索引与图元（索引缺失 true；索引存在但为空/损坏 false；单图元失败跳过）
    static bool load(DAAbstractArchive* archive,
                     DAChart3DItemsManager* itemsMgr,
                     const QString& prefix,
                     QString* errorMessage = nullptr);
    // 在 GUI 线程调用：把管理者里全部图元序列化为快照（单图元失败跳过并 qWarning）
    static QList< DAChart3DItemSnapshot > makeSnapshots(const DAChart3DItemsManager* itemsMgr);
    // 在 GUI 线程调用：只序列化该 3D 子图里要落盘的图元（跳过 transient），key 取自子图的图元管理者
    static QList< DAChart3DItemSnapshot > makeChart3DSnapshots(const DAChart3DWidget* chart3d);
    // 后台线程调用：写 chart3d-items.xml 索引 + 每快照一个 <prefix>/<key> 文件；输出与 save() 字节等价
    static bool saveSnapshots(DAAbstractArchive* archive,
                              const QList< DAChart3DItemSnapshot >& snapshots,
                              const QString& prefix,
                              QString* errorMessage = nullptr);
    // 索引文件名
    static QString itemsIndexFileName();
    // 加载收尾：删除没有被任何 3D 子图挂回去的图元（坏索引/多余条目兜底，避免泄漏），返回删除数量
    static int deleteUnattachedItems(DAChart3DItemsManager* itemsMgr);
};
}  // namespace DA

#endif  // DACHART3DITEMARCHIVE_H