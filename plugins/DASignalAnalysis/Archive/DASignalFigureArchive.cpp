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

/// 在候选集合里按匹配规则查找 figure 元素（不消费）：
/// 1. signal-id 属性精确匹配（新存档；同名信号靠它区分）；
/// 2. 无 signal-id 属性（旧存档）的元素按 signal-name 匹配；
/// 3. 仍无命中时按 figure id "sig-"+signalName 回退（兼容未写 signal-name 的旧档）。
QDomElement matchFigureElement(const QList< QDomElement >& candidates, qint64 signalId, const QString& signalName)
{
    const QString idText = QString::number(signalId);
    for (const QDomElement& figureEle : candidates) {
        if (figureEle.attribute(QStringLiteral("signal-id")) == idText) {
            return figureEle;
        }
    }
    for (const QDomElement& figureEle : candidates) {
        // 旧存档没有 signal-id 属性：按信号名回退（新档的 signal-id 不参与名字匹配）
        if (!figureEle.hasAttribute(QStringLiteral("signal-id"))
            && figureEle.attribute(QStringLiteral("signal-name")) == signalName) {
            return figureEle;
        }
    }
    const QString figureId = QStringLiteral("sig-%1").arg(signalName);
    for (const QDomElement& figureEle : candidates) {
        if (figureEle.attribute(QStringLiteral("id")) == figureId) {
            return figureEle;
        }
    }
    return QDomElement();
}

/// 把 chart 元素 <items> 里已被其它子图消费的图元 key 摘除（防争用：一个 key 只 attach 一次），
/// 新消费的 key 登记进 consumedItemKeys。
void consumeChartItemKeys(QDomElement& chartEle, QSet< QString >& consumedItemKeys)
{
    QDomElement itemsEle = chartEle.firstChildElement(QStringLiteral("items"));
    if (itemsEle.isNull()) {
        return;
    }
    QDomNode itemNode = itemsEle.firstChild();
    while (!itemNode.isNull()) {
        const QDomNode nextNode = itemNode.nextSibling();  // 先取兄弟再决定是否移除
        QDomElement itemEle     = itemNode.toElement();
        if (itemEle.isNull() || itemEle.tagName() != QStringLiteral("item")) {
            itemNode = nextNode;
            continue;
        }
        const QString key = itemEle.attribute(QStringLiteral("key"));
        if (key.isEmpty()) {
            itemNode = nextNode;
            continue;
        }
        if (consumedItemKeys.contains(key)) {
            // 同一图元 key 已 attach 到别的子图：摘除该 <item>，绝不把一个 QwtPlotItem
            // attach 到两个 chart（QwtPlotItem::attach 会把它从上一个 plot 摘下来）
            itemsEle.removeChild(itemNode);
            qWarning() << "DASignalFigureArchive::restoreFigure: chart item key" << key
                       << "already attached to another sub chart, skipped";
        } else {
            consumedItemKeys.insert(key);
        }
        itemNode = nextNode;
    }
}
}  // namespace

/**
 * @brief 生成信号 figure 的 XML 字节
 *
 * 每个 view 写一个 @c figure 元素：由 @ref DAXmlHelper::makeElement(DAFigureWidget*) 生成
 * 完整 figure XML（背景/主题/网格/子图/轴/非 transient 图元 key），再补：
 * - @c signal-name 属性（可读标识）与 @c signal-id 属性（会话内唯一 id，恢复时优先匹配）；
 * - 每个 @c chart 的 @c role 属性（wave/spectrum/heatmap），@c chart3d 的 @c role="waterfall"；
 * - 频谱 chart 里峰值标的 item 的 @c peak="1"。
 * @param views 待保存的信号视图（一个信号一个视图）
 * @param itemsMgr 图元管理者（输出：记录所有非 transient 图元的 key，随后由
 *        DAChartItemArchive 写 chart-data）；为空时无法生成，返回空字节
 * @return @c signal-figures XML 字节；没有可写视图（空工程/页面从未创建）时返回**空字节**，
 *         调用方据此不写 figures.xml 与 chart-data（F-08：不再落一个只有根节点的空档）
 */
QByteArray DASignalFigureArchive::saveFigures(const QList< DASignalAnalysisView* >& views, DAChartItemsManager* itemsMgr)
{
    if (!itemsMgr) {
        qWarning() << "DASignalFigureArchive::saveFigures: items manager is null";
        return QByteArray();
    }
    if (views.isEmpty()) {
        qWarning() << "DASignalFigureArchive::saveFigures: no signal view to save, figures.xml is skipped";
        return QByteArray();
    }
    QDomDocument doc(QStringLiteral("signal-figures"));
    QDomElement root = doc.createElement(QStringLiteral("signal-figures"));
    root.setAttribute(QStringLiteral("version"), 1);
    doc.appendChild(root);
    int savedCount = 0;
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
        // 会话内唯一 id：同名信号在恢复端按它精确匹配，不串档（BLK-02）
        figureEle.setAttribute(QStringLiteral("signal-id"), QString::number(view->signal()->signalId()));
        markFigureRoles(view, figureEle);
        root.appendChild(figureEle);
        ++savedCount;
    }
    if (savedCount <= 0) {
        // 全部视图无效（figure 已销毁等）：同样不写空档，交给调用方跳过 figures.xml/chart-data
        qWarning() << "DASignalFigureArchive::saveFigures: every signal view was invalid, figures.xml is skipped";
        return QByteArray();
    }
    return doc.toByteArray();
}

/**
 * @brief 收集文档里全部 figure 元素（恢复端逐个消费的候选集合）
 *
 * @param doc 已解析的 figures.xml
 * @return 根节点下的全部 @c figure 元素（保持文档顺序）
 */
QList< QDomElement > DASignalFigureArchive::figureElements(const QDomDocument& doc)
{
    QList< QDomElement > result;
    const QDomElement root = doc.documentElement();
    if (root.isNull()) {
        return result;
    }
    for (QDomElement figureEle = root.firstChildElement(QStringLiteral("figure")); !figureEle.isNull();
         figureEle             = figureEle.nextSiblingElement(QStringLiteral("figure"))) {
        result.append(figureEle);
    }
    return result;
}

/**
 * @brief 在 XML 文档里按 signal-id（回退 signal-name，兼容旧存档）查找 figure 元素
 *
 * 非消费式查找（同一元素可被多次命中）；恢复流程请用 @ref takeFigureElement 逐个消费。
 * @param doc 已解析的 figures.xml
 * @param signalId 信号会话内唯一 id
 * @param signalName 信号显示名
 * @return 匹配到的 @c figure 元素；找不到返回空元素
 */
QDomElement DASignalFigureArchive::findFigureElement(const QDomDocument& doc, qint64 signalId, const QString& signalName)
{
    return matchFigureElement(figureElements(doc), signalId, signalName);
}

/**
 * @brief 从候选集合里取走匹配的 figure 元素（同一元素只能被消费一次）
 *
 * 恢复端（applyFiguresXml）用它保证：一个 figure 元素只被一个视图消费；重复命中
 * （如旧存档里两个同名信号）由消费语义天然错开，不会把同一批图元恢复给两个视图。
 * @param candidates 候选元素集合（匹配成功时从中移除该元素）
 * @param signalId 信号会话内唯一 id
 * @param signalName 信号显示名
 * @return 匹配到的 @c figure 元素（已从候选集合移除）；找不到返回空元素
 */
QDomElement DASignalFigureArchive::takeFigureElement(QList< QDomElement >& candidates,
                                                     qint64 signalId,
                                                     const QString& signalName)
{
    const QDomElement matched = matchFigureElement(candidates, signalId, signalName);
    if (!matched.isNull()) {
        candidates.removeOne(matched);  // QDomElement 相等即同一底层节点
    }
    return matched;
}

/**
 * @brief 按视图顺序把候选 figure 元素分派给视图
 *
 * 恢复端的「谁拿哪个 figure 元素」这一步不依赖任何界面对象，抽出来便于单测：
 * 每个视图按 signal-id（精确）→ signal-name（旧档回退）取走一个元素，取走的元素不再参与
 * 后续匹配（同名信号按顺序错开，不串档）；没有命中 figure 的视图得到空元素。
 * @param candidates 候选元素集合（按引用深拷贝传入，函数内部消费）
 * @param views 目标视图列表（保持信号顺序）
 * @return 与 @p views 等长的元素列表，未命中的位置为空元素
 */
QList< QDomElement > DASignalFigureArchive::assignFigureElements(QList< QDomElement > candidates,
                                                                 const QList< DASignalAnalysisView* >& views)
{
    QList< QDomElement > assigned;
    assigned.reserve(views.size());
    for (DASignalAnalysisView* view : views) {
        if (nullptr == view || !view->signal()) {
            assigned.append(QDomElement());
            continue;
        }
        assigned.append(takeFigureElement(candidates, view->signal()->signalId(), view->signal()->name()));
    }
    return assigned;
}

/**
 * @brief 把 figure XML 恢复到已存在的信号视图上
 *
 * 恢复顺序（都作用在视图已有的子图上，不新建子图）：
 * 1. figure 级：id / 背景 / 主题 / <grid> 行列比例（只接受 2×2，比例数量匹配才应用）；
 * 2. chart 级：按 role 匹配子图；无 role 时按 grid-row/grid-col 回退。两者都认不出来时：
 *    整档都没有 role（旧存档）才按 XML 顺序 wave/spectrum/heatmap 回退（跳过已占用子图），
 *    否则跳过该 <chart> 并 qWarning（F-11：不把认不出的子图误套到下一个空子图）；
 * 3. chart3d：交给 @ref DAXmlHelper::loadElement(DAChart3DWidget*) 恢复视图/主题/坐标轴；
 * 4. 频谱 chart 里 peak="1" 的 item key → 图元管理者取回 QwtPlotMarker 并交给视图接管；
 * 5. 刷新视图（replot/轴标题/瀑布模式显隐），不触发新的分析请求。
 *
 * XML 缺失/损坏（无 <charts>/<chart>）时记录 qWarning 并返回 false，视图保持默认布局。
 * @param view 已创建的目标视图（同一信号）
 * @param figureEle figures.xml 里的 figure 元素
 * 防争用（BLK-02）：@p consumedItemKeys 跨视图共享，记录已 attach 的 chart-data 图元 key；
 * 恢复某个子图前先把其中已消费的 key 从 <items> 摘除（qWarning 记录），保证一个
 * QwtPlotItem 只 attach 到一个 chart。为空时在函数内建局部集合（仅本 figure 内防争用）。
 * @param itemsMgr chart-data 读回的图元管理者；可为空（此时只恢复样式，不恢复图元）
 * @param consumedItemKeys 已消费图元 key 集合（跨视图共享；可空）
 * @return 恢复成功返回 true
 */
bool DASignalFigureArchive::restoreFigure(DASignalAnalysisView* view,
                                          const QDomElement& figureEle,
                                          const DAChartItemsManager* itemsMgr,
                                           QSet< QString >* consumedItemKeys)
{
    // 防争用 key 集合：调用方传入则跨视图共享，否则本 figure 内局部生效
    QSet< QString > localConsumedKeys;
    QSet< QString >& consumedKeys = (consumedItemKeys ? *consumedItemKeys : localConsumedKeys);
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
    // 顺序回退只对「整档都没有 role 属性」的旧存档开放：新存档里 role 与 grid 都认不出来
    // 说明该 <chart> 与本视图的子图对不上（版本差异/损坏），此时按顺序套到下一个空子图
    // 会张冠李戴（F-11），直接跳过并记录诊断。
    bool archiveHasRoles = false;
    for (QDomElement chartEle = chartsEle.firstChildElement(QStringLiteral("chart")); !chartEle.isNull();
         chartEle             = chartEle.nextSiblingElement(QStringLiteral("chart"))) {
        if (!chartEle.attribute(QStringLiteral("role")).isEmpty()) {
            archiveHasRoles = true;
            break;
        }
    }
    QList< QDomElement > pendingCharts;
    for (QDomElement chartEle = chartsEle.firstChildElement(QStringLiteral("chart")); !chartEle.isNull();
         chartEle             = chartEle.nextSiblingElement(QStringLiteral("chart"))) {
        FigureRole role       = figureRoleFromString(chartEle.attribute(QStringLiteral("role")));
        DAChartWidget* target = chartForRole(view, role);
        if (!target) {
            target = chartForRole(view, roleFromGridCell(chartEle));
        }
        if (!target || usedTargets.contains(target)) {
            if (archiveHasRoles) {
                // 新存档：role/grid 无法唯一定位（或目标子图已被占用）时不猜，跳过该元素
                qWarning() << "DASignalFigureArchive::restoreFigure: cannot resolve the target sub chart for a "
                              "<chart> element (role"
                           << chartEle.attribute(QStringLiteral("role")) << "), skipped to avoid mis-applying it";
                continue;
            }
            pendingCharts.append(chartEle);
            continue;
        }
        // 防争用：先摘除已被其它子图消费的图元 key，再交给 DAXmlHelper attach
        consumeChartItemKeys(chartEle, consumedKeys);
        if (!DAXmlHelper::loadElement(target, &chartEle, chartItems)) {
            qWarning() << "DASignalFigureArchive::restoreFigure: failed to load a <chart> element, continuing";
            continue;
        }
        usedTargets.insert(target);
        if (target == view->spectrumChart()) {
            spectrumEle = chartEle;
        }
    }
    // 3.3 旧存档（整档都没有 role）：剩余的 chart 元素按 XML 顺序回退到未被占用的
    // wave/spectrum/heatmap。新存档不走这条路径，避免把认不出的子图误套到别的子图上。
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
        // 防争用：回退路径同样先摘除已被消费的图元 key
        QDomElement mutableChartEle = chartEle;
        consumeChartItemKeys(mutableChartEle, consumedKeys);
        if (!DAXmlHelper::loadElement(target, &mutableChartEle, chartItems)) {
            qWarning() << "DASignalFigureArchive::restoreFigure: failed to load a fallback <chart> element, continuing";
            continue;
        }
        usedTargets.insert(target);
        if (target == view->spectrumChart()) {
            spectrumEle = mutableChartEle;
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
/**
 * @brief 释放图元管理者里没有被任何子图消费的图元
 *
 * DAChartItemsManager 只做 key ↔ QwtPlotItem* 映射，**不持有所有权**（析构为空）。加载
 * figures.xml 时图元由 DAXmlHelper/DAChartItemArchive 建出来放进管理者，再由各 <chart> 的
 * <items> 按 key 取回 attach；figure 元素缺失/损坏/角色对不上时这些图元没人消费也每人释放，
 * 会一直泄漏到进程退出（F-10）。这里按「是否挂到某个 plot」判定：plot() 为空即未被消费。
 * @param itemsMgr 读档后的图元管理者；为空直接返回 0
 * @return 释放的图元个数
 */
int DASignalFigureArchive::releaseUnattachedItems(const DAChartItemsManager* itemsMgr)
{
    if (nullptr == itemsMgr) {
        return 0;
    }
    int released            = 0;
    const QList< QwtPlotItem* > items = itemsMgr->items();
    for (QwtPlotItem* item : items) {
        if (nullptr == item || nullptr != item->plot()) {
            continue;  // 已 attach 到子图：归该子图所有，绝不在这里删
        }
        delete item;
        ++released;
    }
    return released;
}
}  // namespace DA
