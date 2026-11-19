#ifndef DASIGNALFIGUREARCHIVE_H
#define DASIGNALFIGUREARCHIVE_H
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QString>

class QDomDocument;
class QDomElement;

namespace DA
{
class DASignalAnalysisView;
class DAChartItemsManager;

/**
 * @brief 信号分析 figure XML 的保存/恢复助手（T9）
 *
 * 把「信号视图的 figure 布局 + 子图样式 + 非 transient 图元」写成
 * @c signal-figures/figures.xml，加载时按「角色」把子图 XML 应用到已存在的视图子图上。
 *
 * 存档只记录非 transient 图元（用户添加的图元、峰值标记）；包络/频谱曲线/热图等
 * 分析结果图元是 transient，不写入 XML，加载后由分析重算重建。
 *
 * figure 元素同时写 @c signal-name（可读）与 @c signal-id（会话内唯一 id）属性；
 * 匹配先按 @c signal-id 精确命中，旧存档（无 signal-id）回退按 @c signal-name。
 * 恢复端保证：一个 figure 元素只被一个视图消费、一个 chart-data 图元 key 只 attach
 * 到一个子图（重复命中跳过并 qWarning），同名信号不再串档（BLK-02）。
 *
 * @code
 * // 保存（在存档任务里，主线程收集好后交给后台任务）
 * QByteArray xml = DASignalFigureArchive::saveFigures(views, &itemsMgr);
 * // 恢复（主线程，视图已创建、chart-data 已读回；候选元素逐个消费防争用）
 * QList<QDomElement> candidates = DASignalFigureArchive::figureElements(doc);
 * QDomElement figureEle = DASignalFigureArchive::takeFigureElement(candidates, sig->signalId(), sig->name());
 * QSet<QString> consumedKeys;
 * DASignalFigureArchive::restoreFigure(view, figureEle, &task->chartItemsManager(), &consumedKeys);
 * @endcode
 */
class DASignalFigureArchive
{
public:
    // 生成 <signal-figures version="1"> XML 字节（每个 view 一个 <figure signal-name="..." signal-id="...">）；
    // 没有可写视图（空工程/全部视图无效）时返回空字节，调用方据此跳过 figures.xml 与 chart-data
    static QByteArray saveFigures(const QList< DASignalAnalysisView* >& views, DAChartItemsManager* itemsMgr);
    // 收集文档里全部 <figure> 元素（恢复端逐个消费的候选集合）
    static QList< QDomElement > figureElements(const QDomDocument& doc);
    // 在 XML 文档里按 signal-id（回退 signal-name，兼容旧存档）找 figure；非消费式查找
    static QDomElement findFigureElement(const QDomDocument& doc, qint64 signalId, const QString& signalName);
    // 从候选集合里取走匹配的 figure 元素（同一元素只能被消费一次；找不到返回空元素）
    static QDomElement takeFigureElement(QList< QDomElement >& candidates, qint64 signalId, const QString& signalName);
    // 把 figure XML 应用到已存在的视图子图上；XML 缺失/损坏返回 false（视图保持默认布局）。
    // consumedItemKeys 跨视图共享：记录已 attach 的 chart-data 图元 key，重复命中跳过（防争用）
    static bool restoreFigure(DASignalAnalysisView* view,
                              const QDomElement& figureEle,
                              const DAChartItemsManager* itemsMgr,
                              QSet< QString >* consumedItemKeys = nullptr);
    // 释放图元管理者里「没有挂到任何子图」的图元（未被任何 figure 消费的残留），返回释放个数；
    // 已 attach 的图元归子图所有，不动。manager 不持有所有权，这是加载残档后唯一的回收点（F-10）
    static int releaseUnattachedItems(const DAChartItemsManager* itemsMgr);
};
}  // namespace DA

#endif  // DASIGNALFIGUREARCHIVE_H
