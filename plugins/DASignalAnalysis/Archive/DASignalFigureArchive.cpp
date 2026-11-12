#include "DASignalFigureArchive.h"
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QSet>
#include <QStringList>
#include <QWidget>
#include <utility>
// DA
#include "Archive/DAXmlHelper.h"
#include "Chart/DAChartItemsManager.h"
#include "DAChart3DWidget.h"
#include "DAChartWidget.h"
#include "DAColorTheme.h"
#include "DAFigureWidget.h"
#include "DAXMLFileInterface.h"
#include "Page/DASignalAnalysisView.h"
#include "Signal/DASignal.h"
// qwt
#include "qwt_plot_item.h"
#include "qwt_plot_marker.h"

namespace DA
{
namespace
{
/// 信号视图里子图的角色：存档按角色恢复，不依赖 XML 顺序
enum class FigureRole
{
    Unknown,
    Wave,
    Spectrum,
    Heatmap,
    Waterfall
};

QString figureRoleToString(FigureRole role)
{
    switch (role) {
    case FigureRole::Wave:
        return QStringLiteral("wave");
    case FigureRole::Spectrum:
        return QStringLiteral("spectrum");
    case FigureRole::Heatmap:
        return QStringLiteral("heatmap");
    case FigureRole::Waterfall:
        return QStringLiteral("waterfall");
    default:
        break;
    }
    return QString();
}

FigureRole figureRoleFromString(const QString& role)
{
    if (role == QLatin1String("wave")) {
        return FigureRole::Wave;
    }
    if (role == QLatin1String("spectrum")) {
        return FigureRole::Spectrum;
    }
    if (role == QLatin1String("heatmap")) {
        return FigureRole::Heatmap;
    }
    if (role == QLatin1String("waterfall")) {
        return FigureRole::Waterfall;
    }
    return FigureRole::Unknown;
}

/// 按 plot id 识别 2D 子图角色（保存时写入 role，读取时可跳过 XML 顺序）
FigureRole roleForPlotId(const DASignalAnalysisView* view, const QString& plotId)
{
    if (!view || plotId.isEmpty()) {
        return FigureRole::Unknown;
    }
    if (DAChartWidget* chart = view->waveChart(); chart && chart->plotId() == plotId) {
        return FigureRole::Wave;
    }
    if (DAChartWidget* chart = view->spectrumChart(); chart && chart->plotId() == plotId) {
        return FigureRole::Spectrum;
    }
    if (DAChartWidget* chart = view->heatmapChart(); chart && chart->plotId() == plotId) {
        return FigureRole::Heatmap;
    }
    return FigureRole::Unknown;
}

/// 角色 → 视图里已存在的 2D 子图
DAChartWidget* chartForRole(DASignalAnalysisView* view, FigureRole role)
{
    if (!view) {
        return nullptr;
    }
    switch (role) {
    case FigureRole::Wave:
        return view->waveChart();
    case FigureRole::Spectrum:
        return view->spectrumChart();
    case FigureRole::Heatmap:
        return view->heatmapChart();
    default:
        break;
    }
    return nullptr;
}

/// 旧存档（无 role）按网格坐标回退：0,0→波形；1,0→频谱；1,1→热图
FigureRole roleFromGridCell(const QDomElement& ele)
{
    bool okRow = false;
    bool okCol = false;
    const int row = ele.attribute(QStringLiteral("grid-row")).toInt(&okRow);
    const int col = ele.attribute(QStringLiteral("grid-col")).toInt(&okCol);
    if (!okRow || !okCol) {
        return FigureRole::Unknown;
    }
    if (row == 0 && col == 0) {
        return FigureRole::Wave;
    }
    if (row == 1 && col == 0) {
        return FigureRole::Spectrum;
    }
    if (row == 1 && col == 1) {
        return FigureRole::Heatmap;
    }
    return FigureRole::Unknown;
}

/// 解析逗号分隔的实数列表（与 DAXmlHelper 的 <grid> rowSizes/colSizes 格式一致）
QList< qreal > parseSizeList(const QString& text)
{
    QList< qreal > result;
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    result.reserve(parts.size());
    for (const QString& part : parts) {
        bool ok = false;
        const qreal value = part.trimmed().toDouble(&ok);
        if (ok) {
            result.append(value);
        }
    }
    return result;
}

/// 给频谱 chart 元素里的峰值图元打 peak="1"（按 itemList 非 transient 顺序与 <item> 一一对应）
void markPeakItemElements(DAChartWidget* chart, const QList< QwtPlotMarker* >& peakMarkers, QDomElement& chartEle)
{
    if (!chart || peakMarkers.isEmpty()) {
        return;
    }
    QSet< const QwtPlotItem* > peakSet;
    for (QwtPlotMarker* marker : peakMarkers) {
        peakSet.insert(marker);
    }
    QDomElement itemsEle = chartEle.firstChildElement(QStringLiteral("items"));
    if (itemsEle.isNull()) {
        return;
    }
    int xmlIndex                = 0;
    const QList< QwtPlotItem* > items = chart->itemList();
    for (QwtPlotItem* item : items) {
        if (!item || chart->isItemTransient(item)) {
            continue;
        }
        const QDomNode node = itemsEle.childNodes().at(xmlIndex);
        ++xmlIndex;
        QDomElement itemEle = node.toElement();
        if (itemEle.isNull() || itemEle.tagName() != QStringLiteral("item")) {
            continue;
        }
        if (peakSet.contains(item)) {
            itemEle.setAttribute(QStringLiteral("peak"), 1);
        }
    }
}

/// 给 figure 下的 chart/chart3d 元素写 role 属性（chart3d 只有瀑布一种角色）
void markFigureRoles(DASignalAnalysisView* view, QDomElement& figureEle)
{
    if (!view || figureEle.isNull()) {
        return;
    }
    QDomElement chartsEle = figureEle.firstChildElement(QStringLiteral("charts"));
    if (!chartsEle.isNull()) {
        const QList< QwtPlotMarker* > peaks = view->peakMarkers();
        for (QDomElement chartEle = chartsEle.firstChildElement(QStringLiteral("chart")); !chartEle.isNull();
             chartEle             = chartEle.nextSiblingElement(QStringLiteral("chart"))) {
            const FigureRole role = roleForPlotId(view, chartEle.attribute(QStringLiteral("id")));
            if (role == FigureRole::Unknown) {
                continue;
            }
            chartEle.setAttribute(QStringLiteral("role"), figureRoleToString(role));
            if (role == FigureRole::Spectrum) {
                markPeakItemElements(view->spectrumChart(), peaks, chartEle);
            }
        }
    }
    for (QDomElement chart3dEle = figureEle.firstChildElement(QStringLiteral("chart3d")); !chart3dEle.isNull();
         chart3dEle             = chart3dEle.nextSiblingElement(QStringLiteral("chart3d"))) {
        chart3dEle.setAttribute(QStringLiteral("role"), QStringLiteral("waterfall"));
    }
}

/// 从频谱 chart 元素里收集 peak="1" 的图元 key
QStringList peakItemKeys(const QDomElement& spectrumEle)
{
    QStringList keys;
    if (spectrumEle.isNull()) {
        return keys;
    }
    const QDomElement itemsEle = spectrumEle.firstChildElement(QStringLiteral("items"));
    if (itemsEle.isNull()) {
        return keys;
    }
    for (QDomElement itemEle = itemsEle.firstChildElement(QStringLiteral("item")); !itemEle.isNull();
         itemEle             = itemEle.nextSiblingElement(QStringLiteral("item"))) {
        if (itemEle.attribute(QStringLiteral("peak")) != QLatin1String("1")) {
            continue;
        }
        const QString key = itemEle.attribute(QStringLiteral("key"));
        if (!key.isEmpty()) {
            keys.append(key);
        }
    }
    return keys;
}
}  // namespace

/**
 * @brief 生成信号 figure 的 XML 字节
 *
 * 每个 view 写一个 @c figure 元素：由 @ref DAXmlHelper::makeElement(DAFigureWidget*) 生成
 * 完整 figure XML（背景/主题/网格/子图/轴/非 transient 图元 key），再补：
 * - @c signal-name 属性（恢复时按信号名匹配 figure）；
 * - 每个 @c chart 的 @c role 属性（wave/spectrum/heatmap），@c chart3d 的 @c role="waterfall"；
 * - 频谱 chart 里峰值标的 item 的 @c peak="1"。
 * @param views 待保存的信号视图（一个信号一个视图）
 * @param itemsMgr 图元管理者（输出：记录所有非 transient 图元的 key，随后由
 *        DAChartItemArchive 写 chart-data）；为空时无法生成，返回空字节
 * @return @c signal-figures XML 字节；无有效视图时返回只有根节点的合法 XML
 */
QByteArray DASignalFigureArchive::saveFigures(const QList< DASignalAnalysisView* >& views, DAChartItemsManager* itemsMgr)
{
    if (!itemsMgr) {
        qWarning() << "DASignalFigureArchive::saveFigures: items manager is null";
        return QByteArray();
    }
    QDomDocument doc(QStringLiteral("signal-figures"));
    QDomElement root = doc.createElement(QStringLiteral("signal-figures"));
    root.setAttribute(QStringLiteral("version"), 1);
    doc.appendChild(root);
    for (DASignalAnalysisView* view : views) {
        if (!view || !view->figure() || !view->signal()) {
            qWarning() << "DASignalFigureArchive::saveFigures: skip an invalid signal view";
            continue;
        }
        QDomElement figureEle =
            DAXmlHelper::makeElement(view->figure(), QStringLiteral("figure"), &doc, itemsMgr);
        if (figureEle.isNull()) {
            qWarning() << "DASignalFigureArchive::saveFigures: failed to make figure element for"
                       << view->signal()->name();
            continue;
        }
        figureEle.setAttribute(QStringLiteral("signal-name"), view->signal()->name());
        markFigureRoles(view, figureEle);
        root.appendChild(figureEle);
    }
    return doc.toByteArray();
}

/**
 * @brief 在 figures.xml 文档里按信号名查找 figure 元素
 *
 * 优先按 @c signal-name 属性匹配（信号改名后仍能匹配保存时的名字用不上，故同时回退按
 * figure id @c "sig-"+signalName 匹配，兼容未写 signal-name 的旧存档）。
 * @param doc 已解析的 figures.xml
 * @param signalName 信号显示名
 * @return 匹配到的 @c figure 元素；找不到返回空元素
 */
QDomElement DASignalFigureArchive::findFigureElement(const QDomDocument& doc, const QString& signalName)
{
    const QDomElement root = doc.documentElement();
    if (root.isNull()) {
        return QDomElement();
    }
    for (QDomElement figureEle = root.firstChildElement(QStringLiteral("figure")); !figureEle.isNull();
         figureEle             = figureEle.nextSiblingElement(QStringLiteral("figure"))) {
        if (figureEle.attribute(QStringLiteral("signal-name")) == signalName) {
            return figureEle;
        }
    }
    const QString figureId = QStringLiteral("sig-%1").arg(signalName);
    for (QDomElement figureEle = root.firstChildElement(QStringLiteral("figure")); !figureEle.isNull();
         figureEle             = figureEle.nextSiblingElement(QStringLiteral("figure"))) {
        if (figureEle.attribute(QStringLiteral("id")) == figureId) {
            return figureEle;
        }
    }
    return QDomElement();
}

/**
 * @brief 把 figure XML 恢复到已存在的信号视图上
 *
 * 恢复顺序（都作用在视图已有的子图上，不新建子图）：
 * 1. figure 级：id / 背景 / 主题 / <grid> 行列比例（只接受 2×2，比例数量匹配才应用）；
 * 2. chart 级：按 role 匹配子图；无 role 时按 grid-row/grid-col 回退，再按 XML 顺序
 *    wave/spectrum/heatmap 回退（跳过已被其它元素占用的目标子图）；
 * 3. chart3d：交给 @ref DAXmlHelper::loadElement(DAChart3DWidget*) 恢复视图/主题/坐标轴；
 * 4. 频谱 chart 里 peak="1" 的 item key → 图元管理者取回 QwtPlotMarker 并交给视图接管；
 * 5. 刷新视图（replot/轴标题/瀑布模式显隐），不触发新的分析请求。
 *
 * XML 缺失/损坏（无 <charts>/<chart>）时记录 qWarning 并返回 false，视图保持默认布局。
 * @param view 已创建的目标视图（同一信号）
 * @param figureEle figures.xml 里的 figure 元素
 * @param itemsMgr chart-data 读回的图元管理者；可为空（此时只恢复样式，不恢复图元）
 * @return 恢复成功返回 true
 */
bool DASignalFigureArchive::restoreFigure(DASignalAnalysisView* view,
                                          const QDomElement& figureEle,
                                          const DAChartItemsManager* itemsMgr)
{
    if (!view || !view->figure()) {
        qWarning() << "DASignalFigureArchive::restoreFigure: null view or figure";
        return false;
    }
    if (figureEle.isNull() || figureEle.tagName() != QStringLiteral("figure")) {
        qWarning() << "DASignalFigureArchive::restoreFigure: invalid figure element";
        return false;
    }
    // 先校验再改动：损坏的 figure（没有 charts/chart）直接失败，视图保持默认布局
    const QDomElement chartsEle = figureEle.firstChildElement(QStringLiteral("charts"));
    if (chartsEle.isNull() || chartsEle.firstChildElement(QStringLiteral("chart")).isNull()) {
        qWarning() << "DASignalFigureArchive::restoreFigure: figure has no <charts>/<chart>, keep default layout";
        return false;
    }
    DAFigureWidget* figure = view->figure();
    //! 1.figure 级：id / 背景 / 主题
    const QString figureId = figureEle.attribute(QStringLiteral("id"));
    if (!figureId.isEmpty()) {
        figure->setFigureId(figureId);
    }
    const QDomElement backgroundEle = figureEle.firstChildElement(QStringLiteral("background"));
    if (!backgroundEle.isNull()) {
        QBrush brush;
        if (DAXMLFileInterface::loadElement(brush, &backgroundEle)) {
            figure->setBackgroundColor(brush);
        } else {
            qWarning() << "DASignalFigureArchive::restoreFigure: invalid <background>, skipped";
        }
    }
    const QDomElement colorThemeEle = figureEle.firstChildElement(QStringLiteral("colortheme"));
    if (!colorThemeEle.isNull()) {
        DAColorTheme theme;
        if (DAXmlHelper::loadElement(&theme, &colorThemeEle)) {
            figure->setColorTheme(theme);
        } else {
            qWarning() << "DASignalFigureArchive::restoreFigure: invalid <colortheme>, skipped";
        }
    }
    //! 2.figure 级：网格行列比例（信号视图固定 2×2；比例数量匹配才应用，优先于 signals.json 旧键）
    const QDomElement gridEle = figureEle.firstChildElement(QStringLiteral("grid"));
    if (!gridEle.isNull()) {
        const int rows = gridEle.attribute(QStringLiteral("rows")).toInt();
        const int cols = gridEle.attribute(QStringLiteral("cols")).toInt();
        if (rows == 2 && cols == 2) {
            const QList< qreal > rowSizes = parseSizeList(gridEle.attribute(QStringLiteral("rowSizes")));
            const QList< qreal > colSizes = parseSizeList(gridEle.attribute(QStringLiteral("colSizes")));
            if (rowSizes.size() == rows && colSizes.size() == cols) {
                view->setGridSizes(rowSizes, colSizes);
            } else {
                qWarning() << "DASignalFigureArchive::restoreFigure: grid size count mismatch, expected" << rows
                           << cols << "got" << rowSizes.size() << colSizes.size();
            }
        } else {
            qWarning() << "DASignalFigureArchive::restoreFigure: unsupported grid" << rows << cols
                       << ", keep the default 2x2 layout";
        }
    }
    //! 3.子图级：逐步把 XML 应用到已有子图，不新建子图
    DAChartItemsManager fallbackItemsMgr;
    const DAChartItemsManager* chartItems = itemsMgr ? itemsMgr : &fallbackItemsMgr;
    QSet< QWidget* > usedTargets;
    QDomElement spectrumEle;
    // 3.1 三维瀑布：视图只有一个 3D 子图
    for (QDomElement chart3dEle = figureEle.firstChildElement(QStringLiteral("chart3d")); !chart3dEle.isNull();
         chart3dEle             = chart3dEle.nextSiblingElement(QStringLiteral("chart3d"))) {
        DAChart3DWidget* target = view->waterfall3DChart();
        if (!target || usedTargets.contains(target)) {
            qWarning() << "DASignalFigureArchive::restoreFigure: no 3D chart left for a <chart3d> element, skipped";
            continue;
        }
        if (DAXmlHelper::loadElement(target, &chart3dEle)) {
            usedTargets.insert(target);
        } else {
            qWarning() << "DASignalFigureArchive::restoreFigure: failed to load a <chart3d> element";
        }
    }
    // 3.2 2D 子图：role → grid-row/grid-col → XML 顺序
    QList< QDomElement > pendingCharts;
    for (QDomElement chartEle = chartsEle.firstChildElement(QStringLiteral("chart")); !chartEle.isNull();
         chartEle             = chartEle.nextSiblingElement(QStringLiteral("chart"))) {
        FigureRole role = figureRoleFromString(chartEle.attribute(QStringLiteral("role")));
        DAChartWidget* target = chartForRole(view, role);
        if (!target) {
            target = chartForRole(view, roleFromGridCell(chartEle));
        }
        if (!target || usedTargets.contains(target)) {
            pendingCharts.append(chartEle);
            continue;
        }
        if (!DAXmlHelper::loadElement(target, &chartEle, chartItems)) {
            qWarning() << "DASignalFigureArchive::restoreFigure: failed to load a <chart> element, continuing";
            continue;
        }
        usedTargets.insert(target);
        if (target == view->spectrumChart()) {
            spectrumEle = chartEle;
        }
    }
    // 3.3 剩余的 chart 元素按 XML 顺序回退到未被占用的 wave/spectrum/heatmap
    const QList< DAChartWidget* > fallbackOrder = { view->waveChart(), view->spectrumChart(), view->heatmapChart() };
    for (const QDomElement& chartEle : std::as_const(pendingCharts)) {
        DAChartWidget* target = nullptr;
        for (DAChartWidget* candidate : fallbackOrder) {
            if (candidate && !usedTargets.contains(candidate)) {
                target = candidate;
                break;
            }
        }
        if (!target) {
            qWarning() << "DASignalFigureArchive::restoreFigure: no sub chart left for a <chart> element, skipped";
            continue;
        }
        if (!DAXmlHelper::loadElement(target, &chartEle, chartItems)) {
            qWarning() << "DASignalFigureArchive::restoreFigure: failed to load a fallback <chart> element, continuing";
            continue;
        }
        usedTargets.insert(target);
        if (target == view->spectrumChart()) {
            spectrumEle = chartEle;
        }
    }
    //! 4.峰值标记：存档优先，交给视图接管（之后 rebuildPeakMarkers 不按重算结果覆盖）
    if (!spectrumEle.isNull() && itemsMgr) {
        const QStringList peakKeys = peakItemKeys(spectrumEle);
        QList< QwtPlotMarker* > restoredMarkers;
        for (const QString& key : peakKeys) {
            QwtPlotItem* item = itemsMgr->keyToItem(key);
            auto* marker      = dynamic_cast< QwtPlotMarker* >(item);
            if (!marker) {
                qWarning() << "DASignalFigureArchive::restoreFigure: peak item key" << key
                           << "cannot be resolved to a QwtPlotMarker, skipped";
                continue;
            }
            restoredMarkers.append(marker);
        }
        if (!restoredMarkers.isEmpty()) {
            view->adoptRestoredPeakMarkers(restoredMarkers);
        }
    }
    //! 5.刷新（不触发新的分析请求）
    view->refreshAfterFigureRestore();
    return true;
}
}  // namespace DA
