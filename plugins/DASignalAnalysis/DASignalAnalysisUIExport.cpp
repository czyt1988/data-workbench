#include "DASignalAnalysisUI.h"

// Qt
#include <cmath>
#include <limits>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

// DA / DAFigure
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "pandas/DAPyDataFrame.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADataManagerInterface.h"
#include "DAFigureWidget.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Analysis/DASignalAmplitude.h"
#include "Page/DASignalChartExport.h"

namespace DA
{
// ---------------------------------------------------------------- 导出与工程保存

namespace
{
/// 调 Python 导出模块并执行一个返回 bool 的导出函数
bool callPythonExport(const char* funcName, pybind11::tuple args)
{
    try {
        DAPyModule exporter  = DAPyModule("DASignalAnalysis.exporter");
        pybind11::object res = exporter.attr(funcName)(*args);
        return res.cast< bool >();
    } catch (const std::exception& e) {
        qWarning() << "signal export failed:" << e.what();
        return false;
    }
}

pybind11::tuple doubleTuple(const QVector< double >& v)
{
    pybind11::list lst;
    for (double x : v) {
        lst.append(x);
    }
    pybind11::module_ np = pybind11::module_::import("numpy");
    return pybind11::tuple(np.attr("asarray")(lst, pybind11::arg("dtype") = "float64"));
}

QString askSavePath(QWidget* parent, const QString& title, const QString& filter)
{
    return QFileDialog::getSaveFileName(parent, title, QString(), filter);
}

/// 当前幅值模式的英文列名（CSV 表头，保持英文便于二次处理）
QString dasignalAmplitudeLabel(const DASignalAnalysisParams& params)
{
    if (params.amplitudeMode == QLatin1String("rms")) {
        return QStringLiteral("RMS");
    }
    if (params.amplitudeMode == QLatin1String("dbFS")) {
        return QStringLiteral("dB (FS)");
    }
    if (params.amplitudeMode == QLatin1String("dbRef1")) {
        return QStringLiteral("dB (re 1.0)");
    }
    return QStringLiteral("Amplitude");
}

/// 峰值幅值列表 → 当前显示口径（与频谱图/峰值表同一换算函数）
QVector< double >
peakMagnitudesForDisplay(const DAPeakResult& peaks, const DASignalAnalysisParams& params, double calibratedSignalPeak)
{
    QVector< double > out;
    out.reserve(peaks.magnitudes.size());
    for (int i = 0; i < peaks.magnitudes.size(); ++i) {
        out.append(DASignalAmplitude::toDisplay(peaks.magnitudes.at(i),
                                                peaks.frequencies.value(i) <= 0.0,
                                                params.amplitudeMode,
                                                params.dbReference,
                                                params.calibration,
                                                calibratedSignalPeak));
    }
    return out;
}
}  // namespace

/**
 * @brief 导出峰值表 CSV
 */
void DASignalAnalysisUI::exportPeaksCsv()
{
    if (!mCurrentPeaks.isValid()) {
        reportError(tr("No peak data to export."));  // cn:没有可导出的峰值数据。
        return;
    }
    const QString path = askSavePath(mPage, tr("Export Peak Table"), tr("CSV Files (*.csv)"));  // cn:导出峰值表
    if (path.isEmpty()) {
        return;
    }
    // 与屏幕一致：幅值列按当前显示口径换算（评审 I9）
    DASignalPtr sig                     = mPage ? mPage->currentSignal() : DASignalPtr();
    const DASignalAnalysisParams params = paramsForSignal(sig);
    double signalPeak                   = 0.0;
    if (mPage) {
        if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
            signalPeak = v->signalPeak();
        }
    }
    const double peakCal = signalPeak * params.calibration;
    QVector< double > magnitudes;
    magnitudes.reserve(mCurrentPeaks.magnitudes.size());
    for (int i = 0; i < mCurrentPeaks.magnitudes.size(); ++i) {
        magnitudes.append(DASignalAmplitude::toDisplay(mCurrentPeaks.magnitudes.at(i),
                                                       mCurrentPeaks.frequencies.value(i) <= 0.0,
                                                       params.amplitudeMode,
                                                       params.dbReference,
                                                       params.calibration,
                                                       peakCal));
    }
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(mCurrentPeaks.frequencies),
                                                doubleTuple(magnitudes),
                                                doubleTuple(mCurrentPeaks.relDb),
                                                doubleTuple(mCurrentPeaks.share),
                                                dasignalAmplitudeLabel(params).toStdString());
    if (callPythonExport("write_peaks_csv", args)) {
        mUI->addInfoLogMessage(tr("Peak table exported: %1").arg(path));  // cn:峰值表已导出：%1
    } else {
        reportError(tr("Failed to export the peak table."));  // cn:峰值表导出失败。
    }
}

/**
 * @brief 导出频谱曲线 CSV（两列：频率、当前幅值模式下的幅值）
 */
void DASignalAnalysisUI::exportSpectrumCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const DASpectrumResult spec = spectrumForSignal(sig, channelOfSignal(sig));
    if (!spec.isValid()) {
        reportError(tr("No spectrum data to export."));  // cn:没有可导出的频谱数据。
        return;
    }
    const QString path = askSavePath(mPage, tr("Export Spectrum"), tr("CSV Files (*.csv)"));  // cn:导出频谱
    if (path.isEmpty()) {
        return;
    }
    // 按当前幅值模式换算（与显示口径一致，评审 I9）
    const DASignalAnalysisParams params = paramsForSignal(sig);
    double signalPeak                   = 0.0;
    if (mPage) {
        if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
            signalPeak = v->signalPeak();
        }
    }
    QVector< double > values;
    values.reserve(spec.amplitudes.size());
    for (int i = 0; i < spec.amplitudes.size(); ++i) {
        values.append(DASignalAmplitude::toDisplay(spec.amplitudes.at(i),
                                                   i == 0,
                                                   params.amplitudeMode,
                                                   params.dbReference,
                                                   params.calibration,
                                                   signalPeak * params.calibration));
    }
    const QString yLabel = dasignalAmplitudeLabel(params);
    pybind11::tuple args =
        pybind11::make_tuple(path.toStdString(), doubleTuple(spec.freqs), doubleTuple(values), yLabel.toStdString());
    if (callPythonExport("write_spectrum_csv", args)) {
        mUI->addInfoLogMessage(tr("Spectrum exported: %1").arg(path));  // cn:频谱已导出：%1
    } else {
        reportError(tr("Failed to export the spectrum."));  // cn:频谱导出失败。
    }
}

/**
 * @brief 导出韦尔奇功率谱密度 CSV（频率、PSD(dB re 1.0/Hz)）
 *
 * D5 把韦尔奇功率谱列入 v1 算法边界，但此前算完没有任何显示或导出入口（评审 C10）。
 * v1 不为 PSD 单独加一张图，先把结果以 CSV 导出打通，口径与计算层一致：
 * P[f] 为单边功率谱密度，转 dB 用 10·log10（功率量）。
 */
void DASignalAnalysisUI::exportPsdCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const DASpectrumResult spec = spectrumForSignal(sig, channelOfSignal(sig));
    if (!spec.isValid() || spec.psdFreqs.isEmpty() || spec.psdValues.isEmpty()) {
        reportError(tr("No power spectral density data to export."));  // cn:没有可导出的功率谱密度数据。
        return;
    }
    const QString path = askSavePath(mPage,
                                     tr("Export Power Spectral Density"),  // cn:导出功率谱密度
                                     tr("CSV Files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    constexpr double cEps = 1e-30;
    QVector< double > values;
    values.reserve(spec.psdValues.size());
    for (double p : std::as_const(spec.psdValues)) {
        values.append(10.0 * std::log10(qMax(p, cEps)));
    }
    const QString yLabel = tr("PSD (dB re 1.0/Hz)");  // cn:功率谱密度（dB，相对 1.0/赫兹）
    pybind11::tuple args =
        pybind11::make_tuple(path.toStdString(), doubleTuple(spec.psdFreqs), doubleTuple(values), yLabel.toStdString());
    if (callPythonExport("write_spectrum_csv", args)) {
        mUI->addInfoLogMessage(tr("Power spectral density exported: %1").arg(path));
        // cn:功率谱密度已导出：%1
    } else {
        reportError(tr("Failed to export the power spectral density."));  // cn:功率谱密度导出失败。
    }
}

/**
 * @brief 导出热图矩阵 CSV（行 = 频率、列 = 时间；导出前给体积提示）
 */
void DASignalAnalysisUI::exportHeatmapCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig || !mPage) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    const DAStftResult stft = view->stftResult();
    if (!stft.isValid()) {
        reportError(tr("No time-frequency data to export."));  // cn:没有可导出的时频数据。
        return;
    }
    const qint64 cells = qint64(stft.rows) * stft.columns;
    // 大矩阵导出前给体积确认，并提供 2 倍降采样（方案 §10.2 / 评审 I10）
    int downsample = 1;
    if (cells > 2000000) {
        // 文本 CSV 每单元实际约 12–16 字节（数值文本 + 分隔符 + 行尾），按 14 B/单元估算（评审 N-I10）
        constexpr double cBytesPerCsvCell = 14.0;
        const double estimatedMb          = double(cells) * cBytesPerCsvCell / (1024.0 * 1024.0);
        QMessageBox box(QMessageBox::Question,
                        tr("Export Heatmap Matrix"),  // cn:导出热图矩阵
                        tr("The heatmap matrix is %1 x %2 = %3 cells (about %4 MB as CSV).\n"
                           "Export the full matrix or downsample by 2?")
                            // cn:热图矩阵为 %1 x %2 = %3 个单元（CSV 约 %4 兆字节）。导出完整矩阵还是 2 倍降采样？
                            .arg(stft.rows)
                            .arg(stft.columns)
                            .arg(cells)
                            .arg(estimatedMb, 0, 'f', 1),
                        QMessageBox::NoButton,
                        mPage);
        QPushButton* fullBtn   = box.addButton(tr("Export full"), QMessageBox::AcceptRole);    // cn:导出完整矩阵
        QPushButton* downBtn   = box.addButton(tr("Downsample x2"), QMessageBox::ActionRole);  // cn:2 倍降采样
        QPushButton* cancelBtn = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(cancelBtn);  // 安全默认：不误触大文件导出
        box.exec();
        if (box.clickedButton() == cancelBtn) {
            return;
        }
        if (box.clickedButton() == downBtn) {
            downsample = 2;
        } else if (box.clickedButton() != fullBtn) {
            return;
        }
    }
    mUI->addInfoLogMessage(
        tr("Heatmap matrix: %1 x %2 = %3 cells, the CSV may be large.").arg(stft.rows).arg(stft.columns).arg(cells));
    // cn:热图矩阵：%1 x %2 = %3 个单元，CSV 可能较大
    const QString path = askSavePath(mPage, tr("Export Heatmap Matrix"), tr("CSV Files (*.csv)"));  // cn:导出热图矩阵
    if (path.isEmpty()) {
        return;
    }
    QVector< double > outTimes  = stft.times;
    QVector< double > outFreqs  = stft.freqs;
    QVector< double > outValues = stft.values;
    int outRows                 = stft.rows;
    int outCols                 = stft.columns;
    if (downsample > 1) {
        // 块最大值降采样（保留峰值，不丢窄峰）
        outRows = (stft.rows + downsample - 1) / downsample;
        outCols = (stft.columns + downsample - 1) / downsample;
        QVector< double > reduced(outRows * outCols, std::numeric_limits< double >::quiet_NaN());
        for (int r = 0; r < stft.rows; ++r) {
            const int rr = r / downsample;
            for (int c = 0; c < stft.columns; ++c) {
                const double v = stft.values.value(r * stft.columns + c);
                if (!std::isfinite(v)) {
                    continue;
                }
                double& dst = reduced[ rr * outCols + (c / downsample) ];
                dst         = std::isfinite(dst) ? qMax(dst, v) : v;
            }
        }
        QVector< double > t(outCols);
        for (int c = 0; c < outCols; ++c) {
            t[ c ] = stft.times.value(qMin(stft.columns - 1, c * downsample + downsample / 2));
        }
        QVector< double > f(outRows);
        for (int r = 0; r < outRows; ++r) {
            f[ r ] = stft.freqs.value(qMin(stft.rows - 1, r * downsample + downsample / 2));
        }
        outTimes  = t;
        outFreqs  = f;
        outValues = reduced;
    }
    pybind11::tuple args = pybind11::make_tuple(
        path.toStdString(), doubleTuple(outTimes), doubleTuple(outFreqs), doubleTuple(outValues), outRows, outCols);
    if (callPythonExport("write_heatmap_csv", args)) {
        mUI->addInfoLogMessage(tr("Heatmap matrix exported: %1").arg(path));  // cn:热图矩阵已导出：%1
    } else {
        reportError(tr("Failed to export the heatmap matrix."));  // cn:热图矩阵导出失败。
    }
}

/**
 * @brief 导出整页（2D 网格 + 三维帧缓冲合帧）为 PNG
 *
 * 整页位图走 DASignalChartExport::grabWholePage：三维瀑布可见时把 OpenGL 帧缓冲
 * 按子图位置合成到页面位图（沿用工程评审 I11 的 grabForExport 思路）。
 */
void DASignalAnalysisUI::exportWholePageImage()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const QString pngFilter = tr("PNG Image (*.png)");                                 // cn:PNG 图片 (*.png)
    QString path            = askSavePath(mPage, tr("Export Whole Page"), pngFilter);  // cn:导出整页
    if (path.isEmpty()) {
        return;
    }
    // 整页固定 PNG：按过滤器补齐后缀（F-07，与子图导出同一段规范化逻辑；不动 askSavePath
    // 本体，避免影响 CSV 等其它调用方）
    path = DAFigureWidget::applyExportFilterSuffix(path, pngFilter);
    if (DASignalChartExport::saveWholePage(view, path)) {
        mUI->addInfoLogMessage(tr("Whole page exported: %1").arg(path));  // cn:整页已导出：%1
    } else {
        reportError(tr("Failed to export the whole page."));  // cn:整页导出失败。
    }
}

/**
 * @brief 导出当前子图（2D 支持 PNG/SVG/PDF；三维子图本期固定 PNG）
 *
 * 后缀口径与绘图区「导出当前子图」一致：用户输入未知后缀时按所选过滤器补真实后缀，
 * 避免「内容与后缀不一致」。3D 子图激活时文件对话框过滤器收窄为 PNG（SF-14）：
 * qwt3d 提供 saveVector（gl2ps 已编入）但依赖 GL 上下文、目标环境可用性未验证，
 * 本期固定 PNG，矢量导出留作后续项。降级分支保留作防御：万一拿到非 PNG 请求
 * （如旧工程/程序化调用），改存 PNG 并给用户可见提示（不只写开发日志）。
 */
void DASignalAnalysisUI::exportCurrentSubChartImage()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    // 3D 子图只有 PNG 位图路径：过滤器收窄，用户从一开始就选不到 SVG/PDF（SF-14）
    const bool is3D = view->is3DChartActive();
    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(mPage,
                                                tr("Export Sub-chart"),  // cn:导出当前子图
                                                QStringLiteral("chart.png"),
                                                is3D ? tr("PNG Image (*.png)")  // cn:PNG 图片 (*.png)
                                                     : DAFigureWidget::exportFileFilter(),
                                                &selectedFilter);
    if (path.isEmpty()) {
        return;
    }
    // 以所选过滤器为权威修正后缀（SF-05）：默认名 chart.png 在用户切到 SVG/PDF 过滤器后
    // 仍保留 .png，只按后缀推断格式会「选 SVG 却存成 PNG」；统一走框架侧助手收口
    path                        = DAFigureWidget::applyExportFilterSuffix(path, selectedFilter);
    DAFigureExportFormat format = DAFigureWidget::exportFormatFromFilePath(path);
    if (is3D && format != DAFigureExportFormat::Png) {
        // 防御分支：3D 子图仅支持 PNG 位图导出，改存 PNG 并给用户可见提示（SF-14）
        const QFileInfo fi(path);
        path   = fi.path().isEmpty() ? fi.completeBaseName() + QStringLiteral(".png")
                                     : fi.path() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".png");
        format = DAFigureExportFormat::Png;
        qWarning() << "DASignalAnalysisUI::exportCurrentSubChartImage: 3D sub-chart supports PNG export "
                      "only, saved as PNG:"
                   << path;
        mUI->addWarningLogMessage(
            tr("3D sub-chart supports PNG export only; saved as PNG."));  // cn:三维子图仅支持 PNG 导出，已按 PNG 保存。
    }
    if (DASignalChartExport::saveCurrentSubChart(view, path, format)) {
        mUI->addInfoLogMessage(tr("Sub-chart exported: %1").arg(path));  // cn:当前子图已导出：%1
    } else {
        reportError(tr("Failed to export the sub-chart."));  // cn:当前子图导出失败。
    }
}

/**
 * @brief 复制整页（2D + 三维合帧）到剪贴板
 */
void DASignalAnalysisUI::copyWholePageToClipboard()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    if (DASignalChartExport::copyWholePageToClipboard(view)) {
        mUI->addInfoLogMessage(tr("Whole page copied to the clipboard."));  // cn:整页已复制到剪贴板。
    } else {
        reportError(tr("Failed to copy the whole page to the clipboard."));  // cn:复制整页到剪贴板失败。
    }
}

/**
 * @brief 复制当前子图（二维或三维位图）到剪贴板
 */
void DASignalAnalysisUI::copyCurrentSubChartToClipboard()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    if (DASignalChartExport::copyCurrentSubChartToClipboard(view)) {
        mUI->addInfoLogMessage(tr("Current sub-chart copied to the clipboard."));  // cn:当前子图已复制到剪贴板。
    } else {
        reportError(tr("Failed to copy the current sub-chart to the clipboard."));
        // cn:复制当前子图到剪贴板失败。
    }
}

/**
 * @brief 峰值表送表格页（转成数据帧加入数据管理器）
 */
void DASignalAnalysisUI::sendPeaksToTable()
{
    if (!mCurrentPeaks.isValid()) {
        reportError(tr("No peak data to send."));  // cn:没有可发送的峰值数据。
        return;
    }
    if (!mCore) {
        return;
    }
    DADataManagerInterface* dataMgr = mCore->getDataManagerInterface();
    if (!dataMgr) {
        return;
    }
    try {
        DAPyModule exporter = DAPyModule("DASignalAnalysis.exporter");
        DASignalPtr sig     = (mPage && mPage->currentSignal()) ? mPage->currentSignal() : DASignalPtr();
        const DASignalAnalysisParams params = paramsForSignal(sig);
        double signalPeak                   = 0.0;
        if (mPage) {
            if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
                signalPeak = v->signalPeak();
            }
        }
        // 与屏幕一致：幅值列按当前显示口径换算（评审 I9）
        const QVector< double > magnitudes =
            peakMagnitudesForDisplay(mCurrentPeaks, params, signalPeak * params.calibration);
        pybind11::object df = exporter.attr("peaks_to_dataframe")(doubleTuple(mCurrentPeaks.frequencies),
                                                                  doubleTuple(magnitudes),
                                                                  doubleTuple(mCurrentPeaks.relDb),
                                                                  doubleTuple(mCurrentPeaks.share),
                                                                  dasignalAmplitudeLabel(params).toStdString());
        DAPyDataFrame pyDf(df);
        const QString signalName = (mPage && mPage->currentSignal()) ? mPage->currentSignal()->name() : QString();
        QString tableName        = tr("Peaks - %1").arg(signalName);  // cn:峰值 - %1
        DA::DAData data(pyDf);
        data.setName(tableName);
        dataMgr->addData(data);
        mUI->addInfoLogMessage(tr("Peak table added to the data manager as \"%1\".").arg(tableName));
        // cn:峰值表已作为「%1」加入数据管理器。
    } catch (const std::exception& e) {
        // 异常原文（英文）只写日志，界面给翻译后的提示（评审 §七-9）
        reportOperationFailure(tr("Failed to send the peak table."), QString::fromUtf8(e.what()));
        // cn:峰值表发送失败。
    }
}

}  // namespace DA
