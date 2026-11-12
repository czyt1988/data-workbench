#ifndef DASIGNALCHARTEXPORT_H
#define DASIGNALCHARTEXPORT_H
#include <QPixmap>
#include <QString>
#include "DAFigureWidget.h"

namespace DA
{
class DASignalAnalysisView;

/**
 * @brief 信号分析图表的导出工具（整页 / 当前子图 / 剪贴板）
 *
 * 纯静态工具类：不持有状态、不是 QObject，供插件 UI 的导出菜单/动作调用。
 * - 整页：优先 @ref DASignalAnalysisView::grabForExport（已把三维帧缓冲合成到页面位图），
 *   返回空图时回退 QWidget::grab；
 * - 当前子图：2D 走 DAFigureWidget::exportChartToFile（PNG/SVG/PDF），3D 走
 *   Qwt3DPlot::renderPixmap（仅位图，非 PNG 请求按 PNG 保存并记录偏差）；
 * - 剪贴板：抓图后 QClipboard::setPixmap，空图不写入并返回 false。
 *
 * 位图尺寸口径：HiDPI 下 grab()/grabFramebuffer() 的像素尺寸 = 逻辑尺寸 × devicePixelRatio，
 * 本类保持设备像素导出（像素尺寸更大、更清晰），不做额外缩放。
 */
class DASignalChartExport
{
public:
    // 抓取整页位图（优先 grabForExport，空则回退 grab；view 为空返回空图）
    static QPixmap grabWholePage(DASignalAnalysisView* view);
    // 抓取当前子图位图（3D 激活时取帧缓冲，否则取当前 2D 子图）
    static QPixmap grabCurrentSubChart(DASignalAnalysisView* view);
    // 整页保存为 PNG（空 view/空图/写失败返回 false）
    static bool saveWholePage(DASignalAnalysisView* view, const QString& filePath);
    // 当前子图保存（2D 支持 Png/Svg/Pdf；3D 仅位图，非 Png 按 Png 保存）
    static bool saveCurrentSubChart(DASignalAnalysisView* view, const QString& filePath, DAFigureExportFormat format);
    // 整页复制到剪贴板（空图返回 false）
    static bool copyWholePageToClipboard(DASignalAnalysisView* view);
    // 当前子图复制到剪贴板（空图返回 false）
    static bool copyCurrentSubChartToClipboard(DASignalAnalysisView* view);
};
}  // namespace DA

#endif  // DASIGNALCHARTEXPORT_H
