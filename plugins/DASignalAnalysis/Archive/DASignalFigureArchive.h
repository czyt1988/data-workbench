#ifndef DASIGNALFIGUREARCHIVE_H
#define DASIGNALFIGUREARCHIVE_H
#include <QByteArray>
#include <QList>
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
 * @code
 * // 保存（在存档任务里，主线程收集好后交给后台任务）
 * QByteArray xml = DASignalFigureArchive::saveFigures(views, &itemsMgr);
 * // 恢复（主线程，视图已创建、chart-data 已读回）
 * QDomElement figureEle = DASignalFigureArchive::findFigureElement(doc, signalName);
 * DASignalFigureArchive::restoreFigure(view, figureEle, &task->chartItemsManager());
 * @endcode
 */
class DASignalFigureArchive
{
public:
    // 生成 <signal-figures version="1"> XML 字节（每个 view 一个 <figure signal-name="...">）
    static QByteArray saveFigures(const QList< DASignalAnalysisView* >& views, DAChartItemsManager* itemsMgr);
    // 在 XML 文档里按 signal-name 找 figure；回退按 figure id=="sig-"+name
    static QDomElement findFigureElement(const QDomDocument& doc, const QString& signalName);
    // 把 figure XML 应用到已存在的视图子图上；XML 缺失/损坏返回 false（视图保持默认布局）
    static bool restoreFigure(DASignalAnalysisView* view,
                              const QDomElement& figureEle,
                              const DAChartItemsManager* itemsMgr);
};
}  // namespace DA

#endif  // DASIGNALFIGUREARCHIVE_H
