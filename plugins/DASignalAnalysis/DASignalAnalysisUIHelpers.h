#ifndef DASIGNALANALYSISUIHELPERS_H
#define DASIGNALANALYSISUIHELPERS_H

// DASignalAnalysisUI 各实现文件共享的常量与无状态 helper。
// 这些内容原先定义在 DASignalAnalysisUI.cpp 的匿名 namespace 里，拆分为多个
// translation unit 后必须提到头文件，否则每个 TU 各有一份副本（易漂移）。
// 本头文件只被本插件的实现文件包含，不对外安装。

#include <QAction>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QString>
#include <memory>

#include "SARibbonPanel.h"
#include "SARibbonPanelLayout.h"
#include "qwt_colormap_preset.h"

namespace DA
{
/// 丝带标签页 / 面板 / 动作的 object name（唯一名，供样式与查找使用）
// 插件不再占用顶级丝带标签页：主页「创建」panel 一个入口 + 上下文标签两页。
inline constexpr const char* cPanelImportExportName = "da-panel-signal.importExport";
inline constexpr const char* cPanelPreprocessName   = "da-panel-signal.preprocess";
inline constexpr const char* cPanelSpectrumName     = "da-panel-signal.spectrum";
inline constexpr const char* cPanelTimeFreqName     = "da-panel-signal.timefreq";
inline constexpr const char* cPanelPeaksName        = "da-panel-signal.peaks";
inline constexpr const char* cPanelMoreName         = "da-panel-signal.more";
inline constexpr const char* cPageDockObjectName    = "da-dock-signal-analysis-page";
// 插件自建的 Ribbon 上下文标签（SARibbon 按 objectName 查找，任务卡固定值）
inline constexpr const char* cContextCategoryObjectName     = "ContextCategorySignalAnalysis";
inline constexpr const char* cContextAnalysisPageObjectName = "da-category-signal-analysis";
inline constexpr const char* cContextDisplayPageObjectName  = "da-category-signal-context";
inline constexpr const char* cPanelContextTimeFreqViewName  = "da-panel-signal.display.view";
inline constexpr const char* cPanelContextHeatmapName       = "da-panel-signal.display.heatmap";
inline constexpr const char* cPanelContextWaterfallName     = "da-panel-signal.display.waterfall";
// 窗口底部播放工具栏
inline constexpr const char* cToolbarObjectName = "da-signal-playback-toolbar";

// 图标资源前缀
inline constexpr const char* cIconPrefix = ":/DASignalAnalysis/icon/";
// 宿主应用级图标前缀（复用宿主已有图标，不为单个动作新增 SVG）
inline constexpr const char* cAppIconPrefix = ":/app/bright/Icon/";
// 内存预算（方案 §11 / 评审 D5 / N-C1c）：频谱、峰值表、包络与热图共用同一预算门
inline constexpr qint64 cAnalysisBudgetBytes = qint64(1.5 * 1024 * 1024 * 1024);
// 内存预算判定（QHash 值）：0=未询问 / 1=用户已确认继续 / 2=用户拒绝
inline constexpr int cBudgetUnasked  = 0;
inline constexpr int cBudgetAllowed  = 1;
inline constexpr int cBudgetDeclined = 2;
// 状态栏计算进度的防抖窗口（毫秒）：短任务在窗口内结束就完全不显示进度条与停止按钮，
// 避免小计算让状态栏闪一下；超过窗口仍在算才显示。
inline constexpr int cProgressIndicatorDelayMs = 500;

/// 插件自带图标路径
inline QString signalPluginIcon(const char* file)
{
    return QString::fromUtf8(cIconPrefix) + QString::fromUtf8(file);
}

/**
 * @brief 布局比例预设的行高（归一化，和≈1）
 *
 * 2×2 网格：第 1 行是波形（跨两列）、第 2 行是频谱 / 时频图。
 * 0=默认（与 buildFigure 一致）/ 1=1:1 / 2=波形优先 / 3=分析优先。
 * @param preset 预设编号，越界按默认处理
 * @return 两行比例
 */
inline QList< qreal > layoutRowsForPreset(int preset)
{
    switch (preset) {
    case 1:
        return { 0.50, 0.50 };
    case 2:
        return { 0.65, 0.35 };
    case 3:
        return { 0.35, 0.65 };
    case 0:
    default:
        return { 0.56, 0.44 };
    }
}

/// 布局预设的列宽（左右 1:1，所有预设一致）
inline QList< qreal > layoutColumnsDefault()
{
    return { 0.50, 0.50 };
}

/// 宿主应用级图标路径（复用宿主通用图标）
inline QString signalAppIcon(const char* file)
{
    return QString::fromUtf8(cAppIconPrefix) + QString::fromUtf8(file);
}

/**
 * @brief 用预设色带生成菜单用的渐变预览图标（64×16）
 *
 * 色带菜单每项带一条真实渐变色条（取自 QwtColorMapPreset::create 的 rgb 输出），
 * 用户不必先应用再判断颜色。未知预设给透明图标（菜单项仍可用）。
 * @param preset QwtColorMapPreset 预设名
 * @return 64×16 图标的 QIcon
 */
inline QIcon makeColormapPreviewIcon(const QString& preset)
{
    constexpr int cPreviewWidth  = 64;
    constexpr int cPreviewHeight = 16;
    QImage image(cPreviewWidth, cPreviewHeight, QImage::Format_ARGB32);
    const std::unique_ptr< QwtLinearColorMap > map = QwtColorMapPreset::create(preset);
    if (!map) {
        image.fill(Qt::transparent);
        return QIcon(QPixmap::fromImage(image));
    }
    const double span = double(cPreviewWidth > 1 ? cPreviewWidth - 1 : 1);
    for (int x = 0; x < cPreviewWidth; ++x) {
        const double value = double(x) / span;
        const QRgb color   = map->rgb(0.0, 1.0, value);
        for (int y = 0; y < cPreviewHeight; ++y) {
            image.setPixel(x, y, color);
        }
    }
    return QIcon(QPixmap::fromImage(image));
}

/**
 * @brief 从宿主 ribbon 面板中摘除某个动作对应的按钮（不删宿主面板）
 *
 * 插件往宿主主页「创建」panel 注入过「信号分析」大按钮；热卸载时只摘按钮、
 * 不删宿主 panel（面板归宿主所有）。写法与 plugins/DataAnalysis/DataAnalysisUI.cpp 一致。
 * @param panel 宿主面板，可为 nullptr
 * @param act 待摘除的动作，可为 nullptr
 */
inline void removeActionFromPanel(SARibbonPanel* panel, QAction* act)
{
    if (nullptr == panel || nullptr == act) {
        return;
    }
    SARibbonPanelLayout* lay = panel->panelLayout();
    if (nullptr == lay) {
        return;
    }
    const int idx = lay->indexByAction(act);
    if (idx >= 0) {
        // takeAt 内部已对按钮控件 hide + deleteLater，item 本身需手动释放
        QLayoutItem* item = lay->takeAt(idx);
        delete item;
    }
}
}  // namespace DA

#endif  // DASIGNALANALYSISUIHELPERS_H
