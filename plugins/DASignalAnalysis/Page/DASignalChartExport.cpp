#include "DASignalChartExport.h"
// Qt
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QSize>
// DAFigure
#include "DAChart3DWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
// 插件
#include "Page/DASignalAnalysisView.h"

namespace DA
{
/**
 * @file DASignalChartExport.cpp
 * @brief 信号分析图表导出实现
 *
 * 位图尺寸口径（Qt5/Qt6 一致）：@c QWidget::grab() 与 @c QOpenGLWidget::grabFramebuffer()
 * 返回的是设备像素，像素尺寸 = 控件逻辑尺寸 × devicePixelRatio，位图自身携带 DPR。
 * 因此 HiDPI 屏幕上整页/子图导出的 PNG 会比逻辑尺寸大（更清晰），这是预期行为；
 * 本类不做缩放或 DPR 归一，PNG 的物理像素即打印/粘贴时的实际像素。矢量格式（SVG/PDF）
 * 只对 2D 子图开放，走 DAFigureWidget::exportChartToFile 的逻辑尺寸渲染路径。
 */

/**
 * @brief 抓取整页位图
 *
 * 优先调用 @ref DASignalAnalysisView::grabForExport：瀑布模式为三维时它会把 OpenGL
 * 帧缓冲按网格矩形合成进页面位图（QWidget::grab 抓不到 QOpenGLWidget 内容）；
 * 返回空图（例如视图尚未布局）时回退到普通 QWidget::grab。
 *
 * @param view 信号分析视图，可为 nullptr
 * @return 整页位图；view 为空或两次抓取都失败时返回空 QPixmap
 */
QPixmap DASignalChartExport::grabWholePage(DASignalAnalysisView* view)
{
    if (nullptr == view) {
        qWarning() << "DASignalChartExport::grabWholePage: the view is null";
        return QPixmap();
    }
    QPixmap pixmap = view->grabForExport();
    if (pixmap.isNull()) {
        pixmap = view->grab();
    }
    if (pixmap.isNull()) {
        qWarning() << "DASignalChartExport::grabWholePage: the grabbed page pixmap is null";
    }
    return pixmap;
}

/**
 * @brief 抓取当前子图位图
 *
 * - 三维瀑布激活（@ref DASignalAnalysisView::is3DChartActive）：
 *   Qwt3DPlot::renderPixmap 取帧缓冲，尺寸请求为控件 size（qwt 当前实现以帧缓冲实际
 *   尺寸为准，返回设备像素，见文件头尺寸口径说明）；
 * - 否则：QWidget::grab 抓当前二维子图（@ref DASignalAnalysisView::currentChart）。
 *
 * @param view 信号分析视图，可为 nullptr
 * @return 当前子图位图；view/子图为空、3D 控件尺寸为空或帧缓冲为空时返回空 QPixmap
 */
QPixmap DASignalChartExport::grabCurrentSubChart(DASignalAnalysisView* view)
{
    if (nullptr == view) {
        qWarning() << "DASignalChartExport::grabCurrentSubChart: the view is null";
        return QPixmap();
    }
    if (view->is3DChartActive()) {
        DAChart3DWidget* chart3d = view->waterfall3DChart();
        if (nullptr == chart3d) {
            qWarning() << "DASignalChartExport::grabCurrentSubChart: the 3D sub-chart is not available";
            return QPixmap();
        }
        const QSize size = chart3d->size();
        if (size.isEmpty()) {
            qWarning() << "DASignalChartExport::grabCurrentSubChart: the 3D sub-chart size is empty";
            return QPixmap();
        }
        QPixmap pixmap = chart3d->renderPixmap(size.width(), size.height());
        if (pixmap.isNull()) {
            qWarning() << "DASignalChartExport::grabCurrentSubChart: the 3D frame buffer pixmap is null";
        }
        return pixmap;
    }
    DAChartWidget* chart = view->currentChart();
    if (nullptr == chart) {
        qWarning() << "DASignalChartExport::grabCurrentSubChart: the current 2D sub-chart is not available";
        return QPixmap();
    }
    QPixmap pixmap = chart->grab();
    if (pixmap.isNull()) {
        qWarning() << "DASignalChartExport::grabCurrentSubChart: the 2D sub-chart pixmap is null";
    }
    return pixmap;
}

/**
 * @brief 整页保存为 PNG 文件
 *
 * 固定写 PNG 内容（QPixmap::save 的格式显式指定，不依赖文件后缀；调用方负责让路径
 * 以 .png 结尾，避免内容与后缀不一致）。
 *
 * @param view 信号分析视图
 * @param filePath 目标文件路径
 * @return 成功返回 true；路径为空、view 为空、抓图为空或写文件失败返回 false
 */
bool DASignalChartExport::saveWholePage(DASignalAnalysisView* view, const QString& filePath)
{
    if (filePath.isEmpty()) {
        qWarning() << "DASignalChartExport::saveWholePage: the file path is empty";
        return false;
    }
    const QPixmap pixmap = grabWholePage(view);
    if (pixmap.isNull()) {
        qWarning() << "DASignalChartExport::saveWholePage: no pixmap to save, file path is" << filePath;
        return false;
    }
    if (!pixmap.save(filePath, "PNG")) {
        qWarning() << "DASignalChartExport::saveWholePage: failed to write PNG file" << filePath;
        return false;
    }
    return true;
}

/**
 * @brief 当前子图保存到文件（2D 支持 PNG/SVG/PDF，3D 本期固定 PNG）
 *
 * - 二维子图：委托 @ref DAFigureWidget::exportChartToFile，按 format 输出 PNG/SVG/PDF；
 * - 三维子图：只有 PNG 位图路径。qwt3d 提供 saveVector（gl2ps 已编入 plot3d），但依赖
 *   GL 上下文、目标环境可用性未验证，本期固定 PNG，矢量导出留作后续项；format 非 Png
 *   时按 PNG 内容保存并用 qWarning 记录英文偏差说明（调用方负责把路径后缀改成 .png
 *   并给用户可见提示），这里不改写路径。
 *
 * @param view 信号分析视图
 * @param filePath 目标文件路径
 * @param format 导出格式（Png/Svg/Pdf）
 * @return 成功返回 true；路径为空、view/figure/子图为空、抓图为空或保存失败返回 false
 */
bool DASignalChartExport::saveCurrentSubChart(DASignalAnalysisView* view,
                                              const QString& filePath,
                                              DAFigureExportFormat format)
{
    if (nullptr == view) {
        qWarning() << "DASignalChartExport::saveCurrentSubChart: the view is null";
        return false;
    }
    if (filePath.isEmpty()) {
        qWarning() << "DASignalChartExport::saveCurrentSubChart: the file path is empty";
        return false;
    }
    if (view->is3DChartActive()) {
        if (DAFigureExportFormat::Png != format) {
            // 3D 子图本期仅 PNG 位图导出（saveVector 依赖 GL 上下文未验证）：SVG/PDF 请求
            // 降级为 PNG 位图；用户可见提示由调用方（exportCurrentSubChartImage）负责
            qWarning() << "DASignalChartExport::saveCurrentSubChart: the 3D sub-chart supports bitmap "
                          "export only; requested non-PNG format, saving PNG content to"
                       << filePath;
        }
        const QPixmap pixmap = grabCurrentSubChart(view);
        if (pixmap.isNull()) {
            qWarning() << "DASignalChartExport::saveCurrentSubChart: no 3D pixmap to save, file path is" << filePath;
            return false;
        }
        if (!pixmap.save(filePath, "PNG")) {
            qWarning() << "DASignalChartExport::saveCurrentSubChart: failed to write PNG file" << filePath;
            return false;
        }
        return true;
    }
    DAFigureWidget* fig  = view->figure();
    DAChartWidget* chart = view->currentChart();
    if (nullptr == fig || nullptr == chart) {
        qWarning() << "DASignalChartExport::saveCurrentSubChart: the figure or the current 2D sub-chart is not "
                      "available";
        return false;
    }
    if (!fig->exportChartToFile(chart, filePath, format)) {
        qWarning() << "DASignalChartExport::saveCurrentSubChart: failed to export the 2D sub-chart to" << filePath;
        return false;
    }
    return true;
}

/**
 * @brief 整页复制到剪贴板
 *
 * @param view 信号分析视图
 * @return 成功返回 true；view 为空或抓图为空（不写入剪贴板）返回 false
 */
bool DASignalChartExport::copyWholePageToClipboard(DASignalAnalysisView* view)
{
    const QPixmap pixmap = grabWholePage(view);
    if (pixmap.isNull()) {
        qWarning() << "DASignalChartExport::copyWholePageToClipboard: no pixmap to copy";
        return false;
    }
    QApplication::clipboard()->setPixmap(pixmap);
    return true;
}

/**
 * @brief 当前子图复制到剪贴板
 *
 * @param view 信号分析视图
 * @return 成功返回 true；view 为空或抓图为空（不写入剪贴板）返回 false
 */
bool DASignalChartExport::copyCurrentSubChartToClipboard(DASignalAnalysisView* view)
{
    const QPixmap pixmap = grabCurrentSubChart(view);
    if (pixmap.isNull()) {
        qWarning() << "DASignalChartExport::copyCurrentSubChartToClipboard: no pixmap to copy";
        return false;
    }
    QApplication::clipboard()->setPixmap(pixmap);
    return true;
}
}  // namespace DA
