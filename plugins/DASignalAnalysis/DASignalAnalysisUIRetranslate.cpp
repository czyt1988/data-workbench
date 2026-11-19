#include "DASignalAnalysisUI.h"

// Qt
#include <QMenu>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>

// 3rdparty (SARibbon / ADS / qwt)
#include "SARibbonCategory.h"
#include "SARibbonContextCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonParamGrid.h"
#include "DockWidget.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Playback/DASignalPlayer.h"

namespace DA
{
/**
 * @brief 重译界面文本：标签页名、面板名、全部动作文本
 */
void DASignalAnalysisUI::retranslateUi()
{
    // 页面 dock（插件不再有顶级丝带标签页：主页入口 + 上下文两页）
    if (mPageDock) {
        mPageDock->setWindowTitle(tr("Signal Analysis"));  // cn:信号分析
    }
    // 峰值停靠窗口标题必须自己重译：宿主 resetText() 只处理宿主自己的 dock（评审 §七-5）
    if (mPeakDock) {
        mPeakDock->setWindowTitle(tr("Peaks"));  // cn:峰值
    }
    // 上下文标签页 1「分析设置」的面板
    if (mPanelImportExport) {
        mPanelImportExport->setPanelName(tr("Import / Export"));  // cn:导入导出
    }
    if (mPanelPreprocess) {
        mPanelPreprocess->setPanelName(tr("Preprocess"));  // cn:预处理
    }
    if (mPanelSpectrum) {
        mPanelSpectrum->setPanelName(tr("Spectrum"));  // cn:频谱
    }
    if (mPanelTimeFreq) {
        mPanelTimeFreq->setPanelName(tr("Time-Frequency"));  // cn:时频
    }
    if (mPanelPeaks) {
        mPanelPeaks->setPanelName(tr("Peaks"));  // cn:峰值
    }
    if (mPanelMore) {
        mPanelMore->setPanelName(tr("More"));  // cn:更多
    }
    // 插件上下文标签：标题 / 两页 / 显示页的按对象分组面板
    if (mContextCategory) {
        mContextCategory->setContextTitle(tr("Signal Analysis"));  // cn:信号分析
    }
    if (mContextAnalysisPage) {
        mContextAnalysisPage->setCategoryName(tr("Analysis Settings"));  // cn:分析设置
    }
    if (mContextDisplayPage) {
        mContextDisplayPage->setCategoryName(tr("Signal Display"));  // cn:信号显示
    }
    if (mPanelTimeFreqView) {
        mPanelTimeFreqView->setPanelName(tr("Time-Frequency View"));  // cn:时频图设置
    }
    if (mPanelHeatmap) {
        mPanelHeatmap->setPanelName(tr("Heatmap"));  // cn:云图设置
    }
    if (mPanelWaterfall) {
        mPanelWaterfall->setPanelName(tr("Waterfall"));  // cn:瀑布图设置
    }
    // 主页入口
    if (actionOpenSignalPage) {
        actionOpenSignalPage->setText(tr("Signal Analysis"));                        // cn:信号分析
        actionOpenSignalPage->setToolTip(tr("Open the signal analysis workspace"));  // cn:打开信号分析界面
    }
    // 信号
    if (actionImport) {
        actionImport->setText(tr("Import"));  // cn:导入
        actionImport->setToolTip(tr("Import audio, a data frame or a video, or relink a missing data frame source"));
        // cn:导入音频/数据帧/视频，或重新指定缺失的数据帧来源
    }
    if (actionImportAudio) {
        actionImportAudio->setText(tr("Import Audio"));                                 // cn:导入音频
        actionImportAudio->setToolTip(tr("Import an audio file (WAV/AIFF/FLAC/OGG)"));  // cn:导入音频文件
    }
    if (actionImportDataFrame) {
        actionImportDataFrame->setText(tr("Import Data Frame"));                              // cn:导入数据帧
        actionImportDataFrame->setToolTip(tr("Import columns from an existing data frame"));  // cn:从已有数据帧指定列导入
    }
    if (actionImportVideo) {
        actionImportVideo->setText(tr("Import Video"));  // cn:导入视频
        refreshFFmpegAvailability();                     // tooltip 按 FFmpeg 是否可用给出不同说明
    }
    if (actionRelinkSource) {
        actionRelinkSource->setText(tr("Relink Data Frame Source"));  // cn:重新指定数据帧来源
        actionRelinkSource->setToolTip(tr("Point a signal whose data frame is missing to an existing data frame"));
        // cn:把数据帧缺失的信号重新指向现有数据帧
    }
    // 播放（▶/⏸ 文案与图标随播放状态在 syncSignalToolbar/onPlayerStateChanged 中动态切换）
    if (actionPlay) {
        if (!(mPlayer && mPlayer->isPlaying())) {
            actionPlay->setText(tr("Play"));               // cn:播放
            actionPlay->setToolTip(tr("Start playback"));  // cn:开始播放
        }
    }
    if (actionStop) {
        actionStop->setText(tr("Stop"));  // cn:停止
        actionStop->setToolTip(tr("Stop playback and return to the beginning"));
        // cn:停止播放并回到开头
    }
    if (mToolbarTimeLabel) {
        mToolbarTimeLabel->setToolTip(tr("Playback position / total duration"));
        // cn:播放位置 / 总时长
    }
    // 分析（对话框：常用参数已上丝带，这里只留高级项）
    if (actionAnalysisSetting) {
        actionAnalysisSetting->setText(tr("Advanced Settings..."));  // cn:高级设置...
        actionAnalysisSetting->setToolTip(
            tr("Configure zero padding, power spectrum, advanced peak and calibration options"));
        // cn:配置零填充、功率谱、峰值高级项与标定
    }
    if (actionCancelAnalysis) {
        actionCancelAnalysis->setText(tr("Stop Calculation"));  // cn:停止计算
        actionCancelAnalysis->setToolTip(tr("Stop all running signal analysis computations"));
        // cn:停止全部正在进行的信号分析计算
    }
    // 峰值
    if (actionPeakMarker) {
        actionPeakMarker->setText(tr("Peak Markers"));                                // cn:峰值标记
        actionPeakMarker->setToolTip(tr("Mark the detected peaks on the spectrum"));  // cn:在频谱图上标出检测到的峰值
    }
    if (actionPeakTopN) {
        actionPeakTopN->setText(tr("Marker Count"));                           // cn:标记数量
        actionPeakTopN->setToolTip(tr("Number of peaks to mark (10/20/50)"));  // cn:标记的峰值数量（10/20/50）
    }
    if (actionPeakMarkerDisplay) {
        actionPeakMarkerDisplay->setText(tr("Marker Display"));                             // cn:标记显示
        actionPeakMarkerDisplay->setToolTip(tr("Edit the text shown above peak markers"));  // cn:编辑峰值标记上方显示的文字
    }
    if (mPeakModeLabel) {
        mPeakModeLabel->setText(tr("Mode"));  // cn:模式
    }
    if (mPeakRelDbLabel) {
        mPeakRelDbLabel->setText(tr("Threshold"));  // cn:相对门限
    }
    if (mPeakModeCombo) {
        const QSignalBlocker blocker(mPeakModeCombo);
        mPeakModeCombo->setItemText(0, tr("Simple"));                            // cn:简单
        mPeakModeCombo->setItemText(1, tr("Advanced"));                          // cn:高级
        mPeakModeCombo->setToolTip(tr("Peak search mode: simple or advanced"));  // cn:峰值查找模式：简单/高级
    }
    if (mPeakRelDbSpin) {
        mPeakRelDbSpin->setToolTip(tr("Only keep peaks within this range below the strongest peak"));
        // cn:只保留低于最强峰不超过该门限的峰值
    }
    if (actionPeakWindow) {
        actionPeakWindow->setText(tr("Peak Window"));                           // cn:峰值窗口
        actionPeakWindow->setToolTip(tr("Show or hide the peak dock window"));  // cn:显示或隐藏峰值停靠窗口
    }
    // 显示
    if (actionWaterfall2D) {
        actionWaterfall2D->setText(tr("2D"));                                     // cn:二维
        actionWaterfall2D->setToolTip(tr("Show the 2D time-frequency heatmap"));  // cn:显示二维时频热图
    }
    if (actionWaterfall3D) {
        actionWaterfall3D->setText(tr("3D"));                                // cn:三维
        actionWaterfall3D->setToolTip(tr("Show the 3D surface waterfall"));  // cn:显示三维曲面瀑布
    }
    if (actionWaterfallHide) {
        actionWaterfallHide->setText(tr("Hide"));                        // cn:隐藏
        actionWaterfallHide->setToolTip(tr("Hide the waterfall view"));  // cn:隐藏瀑布视图
    }
    if (actionTimeCursor) {
        actionTimeCursor->setText(tr("Time Cursor"));  // cn:当前帧标记
        actionTimeCursor->setToolTip(
            tr("Mark the current frame: a dashed line on the heatmap and a highlighted trace in the 3D waterfall"));
        // cn:标记当前帧：二维云图上的虚线，三维瀑布里的当前迹线高亮
    }
    if (actionViewOptions) {
        actionViewOptions->setText(tr("View"));  // cn:视图
        actionViewOptions->setToolTip(tr("Time axis format, sub-chart layout and reset display"));
        // cn:时间轴格式、子图布局比例与重置显示
    }
    if (mTimeFormatMenu) {
        mTimeFormatMenu->setTitle(tr("Time format"));  // cn:时间格式
    }
    // 时间格式三态菜单项（顺序与 mTimeFormatActions 一致：秒 / 毫秒 / mm:ss.s）
    if (mTimeFormatActions.size() >= 3) {
        mTimeFormatActions.at(0)->setText(tr("Seconds"));       // cn:秒
        mTimeFormatActions.at(1)->setText(tr("Milliseconds"));  // cn:毫秒
        mTimeFormatActions.at(2)->setText(tr("mm:ss.s"));       // cn:分:秒.十分位
    }
    if (mLayoutPresetMenu) {
        mLayoutPresetMenu->setTitle(tr("Layout"));  // cn:布局比例
    }
    // 布局比例菜单项（顺序与 mLayoutPresetActions 一致：1:1 / 波形优先 / 分析优先 / 恢复默认）
    if (mLayoutPresetActions.size() >= 4) {
        mLayoutPresetActions.at(0)->setText(tr("1:1"));                // cn:1:1
        mLayoutPresetActions.at(1)->setText(tr("Waveform priority"));  // cn:波形优先
        mLayoutPresetActions.at(2)->setText(tr("Analysis priority"));  // cn:分析优先
        mLayoutPresetActions.at(3)->setText(tr("Restore default"));    // cn:恢复默认
    }
    if (actionDisplayReset) {
        actionDisplayReset->setText(tr("Reset display"));  // cn:重置显示
        actionDisplayReset->setToolTip(tr("Restore all display settings to their defaults"));
        // cn:把所有显示类设置恢复为默认值
    }
    if (actionMeasure) {
        actionMeasure->setText(tr("Measure"));  // cn:测量
        actionMeasure->setToolTip(tr("Linked zoom, dual cursors and peak hold"));
        // cn:联动缩放、双光标测量与峰值保持
    }
    if (actionLinkedZoom) {
        actionLinkedZoom->setText(tr("Linked zoom"));  // cn:联动缩放
        actionLinkedZoom->setToolTip(
            tr("Keep the waveform/heatmap time axes and the spectrum/heatmap frequency axes in sync"));
        // cn:波形与云图的时间轴、频谱与云图的频率轴保持同步缩放
    }
    if (actionDualCursor) {
        actionDualCursor->setText(tr("Dual cursors"));  // cn:双光标测量
        actionDualCursor->setToolTip(
            tr("Click a sub-chart to place cursor A, then B: readouts show delta time/frequency and amplitudes"));
        // cn:点一下子图放 A、再点放 B：读数给出 Δt/Δf 与对应幅值
    }
    if (actionPeakHold) {
        actionPeakHold->setText(tr("Peak hold"));  // cn:峰值保持
        actionPeakHold->setToolTip(tr("Keep the maximum spectrum while following playback (requires Follow playback)"));
        // cn:跟随播放时保留历史最大值谱（需先开启跟随播放）
    }
    if (actionResetPeakHold) {
        actionResetPeakHold->setText(tr("Reset peak hold"));  // cn:重置峰值保持
        actionResetPeakHold->setToolTip(tr("Clear the accumulated maximum spectrum"));
        // cn:清空已累积的最大值谱
    }
    // 分析设置页参数控件（标签 / combo 项文本；动作文本另见下方「分析」）
    if (mChannelLabel) {
        mChannelLabel->setText(tr("Channel"));  // cn:通道
    }
    if (mChannelCombo) {
        mChannelCombo->setToolTip(tr("Channel analysed by the spectrum, the time-frequency view and the peak table"));
        // cn:频谱、时频图与峰值表分析的通道
    }
    if (mDetrendLabel) {
        mDetrendLabel->setText(tr("Detrend"));  // cn:去趋势
    }
    if (mDetrendCombo) {
        const QSignalBlocker blocker(mDetrendCombo);
        mDetrendCombo->setItemText(0, tr("None"));      // cn:无
        mDetrendCombo->setItemText(1, tr("Constant"));  // cn:常数
        mDetrendCombo->setItemText(2, tr("Linear"));    // cn:线性
        mDetrendCombo->setToolTip(tr("Remove the DC offset or a linear trend before analysis"));
        // cn:分析前去掉直流分量或线性趋势
    }
    if (mPreprocessDcCheck) {
        mPreprocessDcCheck->setText(tr("Remove DC"));  // cn:去直流
        mPreprocessDcCheck->setToolTip(tr("Remove the DC offset before analysis"));
        // cn:分析前去掉直流分量
    }
    if (mWindowLabel) {
        mWindowLabel->setText(tr("Window"));  // cn:窗函数
    }
    if (mWindowCombo) {
        const QSignalBlocker blocker(mWindowCombo);
        mWindowCombo->setItemText(0, tr("Rectangular"));  // cn:矩形
        mWindowCombo->setItemText(1, tr("Hann"));         // cn:汉宁
        mWindowCombo->setItemText(2, tr("Hamming"));      // cn:汉明
        mWindowCombo->setItemText(3, tr("Blackman"));     // cn:布莱克曼
        mWindowCombo->setItemText(4, tr("Flat top"));     // cn:平顶
        mWindowCombo->setToolTip(tr("Window function shared by the spectrum and the time-frequency view"));
        // cn:频谱与时频视图共用的窗函数
    }
    if (mAmplitudeModeLabel) {
        mAmplitudeModeLabel->setText(tr("Amplitude"));  // cn:幅值口径
    }
    if (mAmplitudeModeCombo) {
        const QSignalBlocker blocker(mAmplitudeModeCombo);
        mAmplitudeModeCombo->setItemText(0, tr("Peak amplitude"));  // cn:峰值幅度
        mAmplitudeModeCombo->setItemText(1, tr("RMS"));             // cn:均方根
        mAmplitudeModeCombo->setItemText(2, tr("dB (FS)"));         // cn:dB（满量程）
        mAmplitudeModeCombo->setItemText(3, tr("dB (re 1.0)"));     // cn:dB（相对 1.0）
        mAmplitudeModeCombo->setToolTip(tr("Amplitude scale used by the plots, the peak table and exports"));
        // cn:频谱图、峰值表与导出使用的幅值口径
    }
    if (mResolutionLabel) {
        mResolutionLabel->setText(tr("Resolution"));  // cn:频率分辨率
    }
    if (mResolutionSpin) {
        mResolutionSpin->setToolTip(tr("Frequency resolution of the spectrum (linked to the STFT window length)"));
        // cn:频谱频率分辨率（与 STFT 窗长联动）
    }
    if (mStftWindowLabel) {
        mStftWindowLabel->setText(tr("Window length"));  // cn:窗长
    }
    if (mStftWindowSpin) {
        mStftWindowSpin->setToolTip(
            tr("Window length of the time-frequency view (linked to the frequency resolution)"));
        // cn:时频视图的窗长（与频率分辨率联动）
    }
    if (mStftOverlapLabel) {
        mStftOverlapLabel->setText(tr("Hop overlap"));  // cn:跳步重叠
    }
    if (mStftOverlapSpin) {
        mStftOverlapSpin->setToolTip(tr("Overlap between consecutive time-frequency frames"));
        // cn:相邻时频帧之间的重叠率
    }
    if (mDeltaFLabel) {
        // 文案随窗长变化，重译时按当前参数重算一次
        const DASignalAnalysisParams params = (mPage && mPage->currentSignal()) ? paramsForSignal(mPage->currentSignal())
                                                                                : DASignalAnalysisParams::defaults();
        updateDeltaFLabel(params);
    }
    // 信号显示页新增动作（云图设置：色带 / 色标）
    if (actionColormap) {
        actionColormap->setText(tr("Color Map"));                                      // cn:色带
        actionColormap->setToolTip(tr("Color map of the 2D time-frequency heatmap"));  // cn:二维时频热图的色带
    }
    if (actionColorBar) {
        actionColorBar->setText(tr("Color bar"));  // cn:色标
        actionColorBar->setToolTip(tr("Show or hide the time-frequency color bar"));
        // cn:显示或隐藏时频图色标
    }
    if (mHeatmapDbFloorLabel) {
        mHeatmapDbFloorLabel->setText(tr("dB"));  // cn:dB
    }
    if (mHeatmapDbFloorSpin) {
        mHeatmapDbFloorSpin->setToolTip(tr("Heatmap color scale lower limit (dB)"));
        // cn:热图色标下限（dB）
    }
    // 云图显示 ▾（显示模式 / 插值）
    if (actionHeatmapDisplay) {
        actionHeatmapDisplay->setText(tr("Display"));  // cn:显示
        actionHeatmapDisplay->setToolTip(tr("Heatmap display mode and interpolation"));
        // cn:云图的显示模式与插值方式
    }
    if (mHeatmapDisplayModeSection) {
        mHeatmapDisplayModeSection->setText(tr("Display mode"));  // cn:显示模式
    }
    if (mHeatmapDisplayModeActions.size() >= 3) {
        mHeatmapDisplayModeActions.at(0)->setText(tr("Image"));            // cn:图像
        mHeatmapDisplayModeActions.at(1)->setText(tr("Contour"));          // cn:等高线
        mHeatmapDisplayModeActions.at(2)->setText(tr("Image + Contour"));  // cn:图像+等高线
    }
    if (mHeatmapResampleSection) {
        mHeatmapResampleSection->setText(tr("Interpolation"));  // cn:插值
    }
    if (mHeatmapResampleActions.size() >= 2) {
        mHeatmapResampleActions.at(0)->setText(tr("Nearest neighbour"));  // cn:最近邻
        mHeatmapResampleActions.at(1)->setText(tr("Bilinear"));           // cn:双线性
    }
    // 瀑布图设置（只影响三维瀑布）
    if (actionWaterfallSettings) {
        actionWaterfallSettings->setText(tr("Waterfall Settings..."));  // cn:瀑布设置...
        actionWaterfallSettings->setToolTip(
            tr("Open the 3D settings panel for the waterfall item (style, color mode, opacity, outline)"));
        // cn:在右侧 3D 设置面板里编辑瀑布图元（几何样式/着色方式/不透明度/描边）
    }
    if (actionWaterfallColormap) {
        actionWaterfallColormap->setText(tr("Color Map"));                         // cn:色带
        actionWaterfallColormap->setToolTip(tr("Color map of the 3D waterfall"));  // cn:三维瀑布的色带
    }
    if (mWaterfallDbFloorLabel) {
        mWaterfallDbFloorLabel->setText(tr("dB"));  // cn:dB
    }
    if (mWaterfallDbFloorSpin) {
        mWaterfallDbFloorSpin->setToolTip(tr("3D waterfall color range lower limit (dB)"));
        // cn:三维瀑布色域下限（dB）
    }
    if (actionWaterfallHighlight) {
        actionWaterfallHighlight->setText(tr("Current Trace"));  // cn:当前迹线高亮
        actionWaterfallHighlight->setToolTip(
            tr("Highlight the trace of the current frame (unavailable in solid color mode)"));
        // cn:高亮当前帧对应的迹线（单色着色方式下不可用）
    }
    // 窗口底部工具栏
    if (mToolbarFollowCheck) {
        mToolbarFollowCheck->setText(tr("Follow playback"));  // cn:跟随播放
        mToolbarFollowCheck->setToolTip(tr("Show the current frame while playback advances"));
        // cn:播放推进时显示当前帧
    }
    if (mToolbarTimeSlider) {
        mToolbarTimeSlider->setToolTip(tr("Playback position"));  // cn:播放位置
    }
    if (mToolbarAutoScrollCheck) {
        mToolbarAutoScrollCheck->setText(tr("Auto scroll"));  // cn:自动滚动
        mToolbarAutoScrollCheck->setToolTip(tr("Scroll the time-frequency view while playback advances"));
        // cn:播放推进时滚动时频视图
    }
    // 导出
    if (actionExportPeaks) {
        actionExportPeaks->setText(tr("Export Peaks"));  // cn:导出峰值表
    }
    // 导出菜单项文本也随语言切换（评审 §七-5）
    if (mActionExportPeaksCsv) {
        mActionExportPeaksCsv->setText(tr("Peak table (CSV)"));  // cn:峰值表（CSV）
    }
    if (mActionExportSpectrumCsv) {
        mActionExportSpectrumCsv->setText(tr("Spectrum curve (CSV)"));  // cn:频谱曲线（CSV）
    }
    if (mActionExportPsdCsv) {
        mActionExportPsdCsv->setText(tr("Power spectral density (CSV)"));  // cn:功率谱密度（CSV）
    }
    if (mActionExportHeatmapCsv) {
        mActionExportHeatmapCsv->setText(tr("Heatmap matrix (CSV)"));  // cn:热图矩阵（CSV）
    }
    if (actionExportImage) {
        actionExportImage->setText(tr("Export Image"));  // cn:导出图片
        actionExportImage->setToolTip(
            tr("Export the chart page or the current sub-chart, or copy it to the clipboard"));
        // cn:导出整页/当前子图或复制到剪贴板
    }
    // 图片导出菜单项（整页/当前子图/复制剪贴板，T9）
    if (mActionExportWholePage) {
        mActionExportWholePage->setText(tr("Whole page (PNG)..."));  // cn:整页（PNG）...
    }
    if (mActionExportCurrentSubChart) {
        mActionExportCurrentSubChart->setText(tr("Current sub-chart (PNG/SVG/PDF)..."));
        // cn:当前子图（PNG/SVG/PDF）...
    }
    if (mActionCopyWholePage) {
        mActionCopyWholePage->setText(tr("Copy whole page to clipboard"));  // cn:复制整页到剪贴板
    }
    if (mActionCopyCurrentSubChart) {
        mActionCopyCurrentSubChart->setText(tr("Copy current sub-chart to clipboard"));
        // cn:复制当前子图到剪贴板
    }
    // 参数网格的标签列宽随文案重算：换语言后更长的标签会撑宽标签列，而不是把控件列挤歪
    const QList< SARibbonPanel* > paramPanels = { mPanelPreprocess, mPanelSpectrum, mPanelTimeFreq,
                                                  mPanelPeaks,      mPanelHeatmap,  mPanelWaterfall };
    for (SARibbonPanel* panel : paramPanels) {
        if (!panel) {
            continue;
        }
        const QList< SARibbonParamGrid* > grids = panel->findChildren< SARibbonParamGrid* >();
        for (SARibbonParamGrid* grid : grids) {
            if (grid) {
                grid->recalcColumnWidths();
            }
        }
    }
    // 页面
    if (mPage) {
        mPage->retranslateUi();
    }
}

}  // namespace DA
