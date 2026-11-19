#include "DASignalAnalysisUI.h"
#include "DASignalAnalysisUIHelpers.h"

// Qt
#include <QMenu>
#include <QActionGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMainWindow>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolBar>
#include <QToolButton>

// 3rdparty (SARibbon / ADS / qwt)
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonContextCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonParamGrid.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "qwt_colormap_preset.h"

// DA / DAFigure
#include "DAUIInterface.h"
#include "DARibbonAreaInterface.h"
#include "DADockingAreaInterface.h"
#include "DAActionsInterface.h"
#include "DAUiObjectNames.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"

namespace DA
{
/**
 * @brief 经宿主动作管理器创建带图标的动作
 *
 * 动作归宿主 DAActionsInterface 所有（parent 是宿主接口），插件 finalize() 统一
 * removeAction 销毁；丝带顶级标签页与上下文标签页共用本入口，避免各写一份。
 * @param objectName 动作 objectName（样式/查找用，全插件唯一）
 * @param iconPath 图标资源路径（插件 icon/ 或宿主通用图标）
 * @param checkable 是否可勾选（勾选初值由同步逻辑回填）
 */
QAction* DASignalAnalysisUI::createRibbonIconAction(const char* objectName, const QString& iconPath, bool checkable)
{
    if (nullptr == mActions) {
        qWarning() << "DASignalAnalysisUI::createRibbonIconAction: action interface is unavailable";
        return nullptr;
    }
    const QByteArray iconPathUtf8 = iconPath.toUtf8();
    return mActions->createAction(objectName, iconPathUtf8.constData(), checkable, false, nullptr);
}

/**
 * @brief 主页「创建」panel 注入「信号分析」入口（插件不再占用顶级丝带标签页）
 *
 * 动作 objectName 沿用 actionSignalOpenPage，点击走 openSignalAnalysisPage()（页面惰性创建）；
 * 宿主面板按 DA::UiNames::Ribbon::MainCreatePanel 查找，找不到时只告警、不影响其它功能
 * （工程恢复等路径仍可创建页面）。卸载时只摘按钮，不删宿主 panel（见 finalize()）。
 */
void DASignalAnalysisUI::installHomeEntry()
{
    if (nullptr == mActions) {
        qWarning() << "DASignalAnalysisUI::installHomeEntry: action interface is unavailable";
        return;
    }
    actionOpenSignalPage = mActions->createAction(
        "actionSignalOpenPage",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("showSignalAnalysis.svg")).toUtf8().constData());
    DARibbonAreaInterface* ribbonArea = (mUI ? mUI->getRibbonArea() : nullptr);
    SARibbonPanel* createPanel =
        (ribbonArea ? ribbonArea->getPanelByObjectName(QString::fromUtf8(UiNames::Ribbon::MainCreatePanel)) : nullptr);
    if (nullptr == createPanel) {
        qWarning() << "DASignalAnalysisUI::installHomeEntry: host main create panel not found";
        return;
    }
    createPanel->addLargeAction(actionOpenSignalPage);
}

/**
 * @brief 构建上下文页 1「分析设置」（常用分析参数）
 *
 * 该页必须排在「信号显示」页之前：宿主把上下文标签里「新出现页中的第一个」设为当前页，
 * 排前面的才是默认页。峰值相关动作在步骤 2 拆到两页：常见项留本页、标注类去显示页。
 * @param contextCategory 插件自建的上下文标签（由 buildSignalContextCategory 创建）
 */
void DASignalAnalysisUI::buildAnalysisSettingsPage(SARibbonContextCategory* contextCategory)
{
    if (nullptr == contextCategory) {
        qWarning() << "DASignalAnalysisUI::buildAnalysisSettingsPage: context category is null";
        return;
    }
    mContextAnalysisPage = contextCategory->addCategoryPage(tr("Analysis Settings"));  // cn:分析设置
    if (nullptr == mContextAnalysisPage) {
        qWarning() << "DASignalAnalysisUI::buildAnalysisSettingsPage: addCategoryPage failed";
        return;
    }
    mContextAnalysisPage->setObjectName(QString::fromUtf8(cContextAnalysisPageObjectName));

    // ---------- 导入导出：导入▾ / 导出数据▾ / 导出图片▾ ----------
    // 步骤 3 把原「信号」「导出」两个 panel 合并：导入 4 个入口收进统一「导入」菜单，
    // 与两类导出共用本 panel（导入是进入信号分析的第一步，导出是分析结果的出口）。
    mPanelImportExport = mContextAnalysisPage->addPanel(tr("Import / Export"));  // cn:导入导出
    mPanelImportExport->setObjectName(QString::fromUtf8(cPanelImportExportName));
    // 导入父动作（大按钮，点击展开菜单）
    actionImport = mActions->createAction(
        "actionSignalImport", (QString::fromUtf8(cIconPrefix) + QStringLiteral("import.svg")).toUtf8().constData());
    actionImportAudio =
        mActions->createAction("actionSignalImportAudio",
                               (QString::fromUtf8(cIconPrefix) + QStringLiteral("importAudio.svg")).toUtf8().constData());
    actionImportDataFrame = mActions->createAction(
        "actionSignalImportDataFrame",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("importDataframe.svg")).toUtf8().constData());
    actionImportVideo =
        mActions->createAction("actionSignalImportVideo",
                               (QString::fromUtf8(cIconPrefix) + QStringLiteral("importVideo.svg")).toUtf8().constData());
    // 重新指定数据帧来源（评审 U16）：当前信号的数据帧在工程里丢失时用它重新绑定
    actionRelinkSource = mActions->createAction(
        "actionSignalRelinkSource",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("relinkSource.svg")).toUtf8().constData());
    actionRelinkSource->setEnabled(false);  // 只有当前信号来源缺失时才可用
    // 导入菜单：工具提示必须可见（FFmpeg 缺失时菜单里的视频项要靠 tooltip 说明原因）
    mImportMenu = new QMenu();
    mImportMenu->setToolTipsVisible(true);
    mImportMenu->addAction(actionImportAudio);
    mImportMenu->addAction(actionImportDataFrame);
    mImportMenu->addAction(actionImportVideo);
    mImportMenu->addSeparator();
    mImportMenu->addAction(actionRelinkSource);
    actionImport->setMenu(mImportMenu);
    mPanelImportExport->addLargeAction(actionImport);
    // 导出：峰值/频谱/功率谱/热图 CSV（actionExportPeaks 的菜单在 initialize 里建）
    actionExportPeaks =
        mActions->createAction("actionSignalExportPeaks",
                               (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportTable.svg")).toUtf8().constData());
    // 导出图片：整页 / 当前子图 / 复制剪贴板（actionExportImage 的菜单在 initialize 里建）
    actionExportImage =
        mActions->createAction("actionSignalExportImage",
                               (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportImage.svg")).toUtf8().constData());
    mPanelImportExport->addLargeAction(actionExportPeaks);
    mPanelImportExport->addLargeAction(actionExportImage);

    // ---------- 播放：动作只绑到底部播放器控制条（D5，不再占丝带 panel） ----------
    // 播放/暂停升格为唯一动作（可勾选：勾选=正在播放），暂停动作已删除；
    // 播放器控制条由 buildSignalToolbar() 在页面创建时建好，动作先在这里创建以便 initialize 连接。
    actionPlay = mActions->createAction(
        "actionSignalPlay", (QString::fromUtf8(cIconPrefix) + QStringLiteral("play.svg")).toUtf8().constData(), true);
    actionStop = mActions->createAction(
        "actionSignalStop", (QString::fromUtf8(cIconPrefix) + QStringLiteral("stop.svg")).toUtf8().constData());

    // ---------- 预处理：去直流 / 去趋势（S） ----------
    // 影响频谱、热图、峰值与 dBFS 参考峰值（§5），改后按 ParamScope::Preprocess 分级重算
    mPanelPreprocess = mContextAnalysisPage->addPanel(tr("Preprocess"));  // cn:预处理
    mPanelPreprocess->setObjectName(QString::fromUtf8(cPanelPreprocessName));
    mPreprocessDcCheck = new QCheckBox(tr("Remove DC"), mPanelPreprocess);  // cn:去直流
    mPreprocessDcCheck->setObjectName(QStringLiteral("da-signal-preprocess-removedc"));
    connect(mPreprocessDcCheck, &QCheckBox::toggled, this, &DASignalAnalysisUI::onPreprocessDcToggled);
    mDetrendCombo = new QComboBox(mPanelPreprocess);
    mDetrendCombo->setObjectName(QStringLiteral("da-signal-preprocess-detrend"));
    mDetrendCombo->setMinimumWidth(96);
    mDetrendCombo->addItem(tr("None"), QStringLiteral("none"));          // cn:无
    mDetrendCombo->addItem(tr("Constant"), QStringLiteral("constant"));  // cn:常数
    mDetrendCombo->addItem(tr("Linear"), QStringLiteral("linear"));      // cn:线性
    connect(mDetrendCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DASignalAnalysisUI::onDetrendChanged);
    // 去直流复选框与去趋势下拉框放进同一个参数网格：复选框落在控件列，与下拉框左边界对齐
    auto* preprocessGrid = new SARibbonParamGrid(mPanelPreprocess);
    preprocessGrid->setFlowPolicy(SARibbonParamGrid::ColumnFirstFlow);
    preprocessGrid->addRow(mPreprocessDcCheck);
    mDetrendLabel = preprocessGrid->leadingLabel(preprocessGrid->addRow(tr("Detrend"), mDetrendCombo));  // cn:去趋势
    mPanelPreprocess->addLargeWidget(preprocessGrid);

    // ---------- 频谱：通道 / 窗函数 / 幅值口径 / 频率分辨率（S） ----------
    // 通道下拉从「信号显示」页迁入本页（D8）：通道本就是分析输入，显示页要腾宽度；
    // 幅值口径从「信号显示」页迁入本页（D7：dB 参考由幅值口径派生，不再单独入口）；
    // 频率分辨率与「时频」页的窗长联动（Δf = 1/窗长）。
    mPanelSpectrum = mContextAnalysisPage->addPanel(tr("Spectrum"));  // cn:频谱
    mPanelSpectrum->setObjectName(QString::fromUtf8(cPanelSpectrumName));
    // 通道选择：一次分析一个通道（多通道信号），单通道时置灰
    mChannelCombo = new QComboBox(mPanelSpectrum);
    mChannelCombo->setObjectName(QStringLiteral("da-signal-spectrum-channel"));
    mChannelCombo->setMinimumWidth(120);
    mChannelCombo->setToolTip(tr("Channel"));  // cn:通道
    connect(mChannelCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DASignalAnalysisUI::onChannelComboChanged);
    mWindowCombo = new QComboBox(mPanelSpectrum);
    mWindowCombo->setObjectName(QStringLiteral("da-signal-spectrum-window"));
    mWindowCombo->setMinimumWidth(96);
    mWindowCombo->addItem(tr("Rectangular"), QStringLiteral("rectangular"));  // cn:矩形
    mWindowCombo->addItem(tr("Hann"), QStringLiteral("hann"));                // cn:汉宁
    mWindowCombo->addItem(tr("Hamming"), QStringLiteral("hamming"));          // cn:汉明
    mWindowCombo->addItem(tr("Blackman"), QStringLiteral("blackman"));        // cn:布莱克曼
    mWindowCombo->addItem(tr("Flat top"), QStringLiteral("flattop"));         // cn:平顶
    connect(mWindowCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DASignalAnalysisUI::onWindowChanged);
    mAmplitudeModeCombo = new QComboBox(mPanelSpectrum);
    mAmplitudeModeCombo->setObjectName(QStringLiteral("da-signal-spectrum-amplitude"));
    mAmplitudeModeCombo->setMinimumWidth(118);
    mAmplitudeModeCombo->addItem(tr("Peak amplitude"), QStringLiteral("peak"));  // cn:峰值幅度
    mAmplitudeModeCombo->addItem(tr("RMS"), QStringLiteral("rms"));              // cn:均方根
    mAmplitudeModeCombo->addItem(tr("dB (FS)"), QStringLiteral("dbFS"));         // cn:dB（满量程）
    mAmplitudeModeCombo->addItem(tr("dB (re 1.0)"), QStringLiteral("dbRef1"));   // cn:dB（相对 1.0）
    connect(mAmplitudeModeCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onAmplitudeModeComboChanged);
    mResolutionSpin = new QDoubleSpinBox(mPanelSpectrum);
    mResolutionSpin->setObjectName(QStringLiteral("da-signal-spectrum-resolution"));
    mResolutionSpin->setRange(0.01, 100000.0);
    mResolutionSpin->setDecimals(2);
    mResolutionSpin->setSingleStep(1.0);
    mResolutionSpin->setSuffix(QStringLiteral(" Hz"));
    mResolutionSpin->setMinimumWidth(112);
    mResolutionSpin->setKeyboardTracking(false);  // 输入过程中不逐字符触发重算
    connect(mResolutionSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onResolutionChanged);
    // 通道 / 窗函数 / 幅值口径 / 频率分辨率四行放进同一个参数网格：标签列与控件列共享列宽；
    // 三行样式下自动摊成 2+2 两列，两行/一行样式下按可用行数继续分列，不会被压扁
    auto* spectrumGrid = new SARibbonParamGrid(mPanelSpectrum);
    spectrumGrid->setFlowPolicy(SARibbonParamGrid::ColumnFirstFlow);
    mChannelLabel = spectrumGrid->leadingLabel(spectrumGrid->addRow(tr("Channel"), mChannelCombo));  // cn:通道
    mWindowLabel  = spectrumGrid->leadingLabel(spectrumGrid->addRow(tr("Window"), mWindowCombo));    // cn:窗函数
    mAmplitudeModeLabel = spectrumGrid->leadingLabel(spectrumGrid->addRow(tr("Amplitude"), mAmplitudeModeCombo));  // cn:幅值口径
    mResolutionLabel = spectrumGrid->leadingLabel(spectrumGrid->addRow(tr("Resolution"), mResolutionSpin));  // cn:频率分辨率
    mPanelSpectrum->addLargeWidget(spectrumGrid);

    // ---------- 时频：窗长 / 跳步重叠 / Δf（S） ----------
    // 窗长与频谱页的频率分辨率联动（改一个同步另一个）；Δf 是只读提示，不参与输入。
    mPanelTimeFreq = mContextAnalysisPage->addPanel(tr("Time-Frequency"));  // cn:时频
    mPanelTimeFreq->setObjectName(QString::fromUtf8(cPanelTimeFreqName));
    mStftWindowSpin = new QDoubleSpinBox(mPanelTimeFreq);
    mStftWindowSpin->setObjectName(QStringLiteral("da-signal-stft-window"));
    mStftWindowSpin->setRange(1.0, 10000.0);
    mStftWindowSpin->setDecimals(1);
    mStftWindowSpin->setSingleStep(10.0);
    mStftWindowSpin->setSuffix(QStringLiteral(" ms"));
    mStftWindowSpin->setMinimumWidth(108);
    mStftWindowSpin->setKeyboardTracking(false);
    connect(mStftWindowSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onStftWindowChanged);
    mStftOverlapSpin = new QDoubleSpinBox(mPanelTimeFreq);
    mStftOverlapSpin->setObjectName(QStringLiteral("da-signal-stft-overlap"));
    mStftOverlapSpin->setRange(0.0, 95.0);
    mStftOverlapSpin->setDecimals(0);
    mStftOverlapSpin->setSingleStep(5.0);
    mStftOverlapSpin->setSuffix(QStringLiteral(" %"));
    mStftOverlapSpin->setMinimumWidth(96);
    mStftOverlapSpin->setKeyboardTracking(false);
    connect(mStftOverlapSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onStftOverlapChanged);
    mDeltaFLabel = new QLabel(mPanelTimeFreq);
    mDeltaFLabel->setObjectName(QStringLiteral("da-signal-stft-deltaf"));
    mDeltaFLabel->setText(QStringLiteral("Δf —"));
    // 窗长 / 跳步重叠 / Δf 提示放进同一个参数网格（Δf 是跨列的只读提示）
    auto* timeFreqGrid = new SARibbonParamGrid(mPanelTimeFreq);
    mStftWindowLabel = timeFreqGrid->leadingLabel(timeFreqGrid->addRow(tr("Window length"), mStftWindowSpin));  // cn:窗长
    mStftOverlapLabel = timeFreqGrid->leadingLabel(timeFreqGrid->addRow(tr("Hop overlap"), mStftOverlapSpin));  // cn:跳步重叠
    timeFreqGrid->addSpanRow(mDeltaFLabel);
    mPanelTimeFreq->addLargeWidget(timeFreqGrid);

    // ---------- Peaks：峰值（全项目唯一峰值入口，D13） ----------
    // 峰值相关入口全部在本面板，不再拆到「信号显示」页：
    // 峰值标记（L）/ 标记数量（L）/ 模式+相对门限（参数网格）/ 标记显示（S）/ 峰值窗口（L）。
    mPanelPeaks = mContextAnalysisPage->addPanel(tr("Peaks"));  // cn:峰值
    mPanelPeaks->setObjectName(QString::fromUtf8(cPanelPeaksName));
    // 峰值标记：显示/隐藏频谱图上的峰值标记（状态源仍是 mPeakMarkersVisible）
    actionPeakMarker = createRibbonIconAction("actionSignalPeakMarker", signalPluginIcon("peaks.svg"), true);
    connect(actionPeakMarker, &QAction::toggled, this, &DASignalAnalysisUI::onPeakMarkerToggled);
    mPanelPeaks->addLargeAction(actionPeakMarker);
    actionPeakTopN = createRibbonIconAction("actionSignalPeakTopN", signalPluginIcon("peakCount.svg"), false);
    mPanelPeaks->addLargeAction(actionPeakTopN);
    // 峰值查找：模式 / 相对门限（参数网格，标签列与控件列共享列宽；标签随语言切换重译）
    mPeakModeCombo = new QComboBox(mPanelPeaks);
    mPeakModeCombo->setObjectName(QStringLiteral("da-signal-peak-mode"));
    mPeakModeCombo->setMinimumWidth(84);
    mPeakModeCombo->addItem(tr("Simple"), QStringLiteral("simple"));      // cn:简单
    mPeakModeCombo->addItem(tr("Advanced"), QStringLiteral("advanced"));  // cn:高级
    connect(mPeakModeCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DASignalAnalysisUI::onPeakModeChanged);
    mPeakRelDbSpin = new QDoubleSpinBox(mPanelPeaks);
    mPeakRelDbSpin->setObjectName(QStringLiteral("da-signal-peak-reldb"));
    mPeakRelDbSpin->setRange(0.0, 200.0);
    mPeakRelDbSpin->setDecimals(1);
    mPeakRelDbSpin->setSingleStep(5.0);
    mPeakRelDbSpin->setSuffix(QStringLiteral(" dB"));
    mPeakRelDbSpin->setMinimumWidth(96);
    // 关闭键盘跟踪：输入过程中不逐字符触发重算，回车/失焦或点上下箭头时才提交
    mPeakRelDbSpin->setKeyboardTracking(false);
    connect(mPeakRelDbSpin, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, &DASignalAnalysisUI::onPeakRelDbChanged);
    // 模式 / 相对门限两行放进同一个参数网格：标签列与控件列共享列宽
    auto* peaksGrid = new SARibbonParamGrid(mPanelPeaks);
    mPeakModeLabel  = peaksGrid->leadingLabel(peaksGrid->addRow(tr("Mode"), mPeakModeCombo));       // cn:模式
    mPeakRelDbLabel = peaksGrid->leadingLabel(peaksGrid->addRow(tr("Threshold"), mPeakRelDbSpin));  // cn:相对门限
    mPanelPeaks->addLargeWidget(peaksGrid);
    // 标记显示：半高按钮，弹出峰值标记文本模板编辑对话框（刻意保持 Small 尺寸，D13）
    actionPeakMarkerDisplay =
        createRibbonIconAction("actionSignalPeakMarkerDisplay", signalPluginIcon("peakLabel.svg"), false);
    connect(actionPeakMarkerDisplay, &QAction::triggered, this, &DASignalAnalysisUI::openPeakMarkerDisplaySettings);
    mPanelPeaks->addLargeAction(actionPeakMarkerDisplay);
    // 峰值窗口：峰值表 + 每帧前 N 曲线停靠窗口的唯一开关
    actionPeakWindow = createRibbonIconAction("actionSignalPeakWindow", signalPluginIcon("peaksWindow.svg"), true);
    connect(actionPeakWindow, &QAction::toggled, this, &DASignalAnalysisUI::onPeakWindowToggled);
    mPanelPeaks->addLargeAction(actionPeakWindow);
    buildPeakTopNMenu();

    // ---------- 更多：高级设置…（对话框唯一入口） ----------
    // 常用参数已全部上丝带，本 panel 只留「高级设置…」打开对话框（零填充/功率谱/峰值高级/标定）。
    mPanelMore = mContextAnalysisPage->addPanel(tr("More"));  // cn:更多
    mPanelMore->setObjectName(QString::fromUtf8(cPanelMoreName));
    actionAnalysisSetting = mActions->createAction(
        "actionSignalAnalysisSetting",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("analysisSetting.svg")).toUtf8().constData());
    mPanelMore->addLargeAction(actionAnalysisSetting);
}

/**
 * @brief 构建插件自建的 Ribbon 上下文标签（「分析设置」「信号显示」两页）
 *
 * 上下文标签只在信号分析页 dock 激活时显示（provider 调 setSignalContextCategoryVisible），
 * objectName 固定为 cContextCategoryObjectName，供宿主接口按名字 show/hide。
 * 「信号显示」页按对象分组（时频图设置 / 云图设置 / 瀑布图设置），不再把显示项堆进一个面板；
 * 该页单独开启面板标题（D9），样式切换后由 ribbonStyleChanged 重新下发。
 */
void DASignalAnalysisUI::buildSignalContextCategory()
{
    DARibbonAreaInterface* ribbonArea = (mUI ? mUI->getRibbonArea() : nullptr);
    SARibbonBar* ribbon               = (ribbonArea ? ribbonArea->ribbonBar() : nullptr);
    if (nullptr == ribbon) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: ribbon bar is unavailable";
        return;
    }
    // 图标路径辅助：插件自带图标（文件列表见 icon/）
    auto pluginIcon = [](const char* file) -> QString {
        return QString::fromUtf8(cIconPrefix) + QString::fromUtf8(file);
    };
    // 经宿主动作管理器创建带图标的动作（checkable 动作初始未勾选，状态由同步逻辑回填）
    auto createIconAction = [ this ](const char* objName, const QString& iconPath, bool checkable) -> QAction* {
        return createRibbonIconAction(objName, iconPath, checkable);
    };
    // 色带菜单：QwtColorMapPreset::availablePresets() 动态填充。二维云图与三维瀑布各一份，
    // 两者是独立视图状态（D12），不能共用同一个勾选组
    auto makeColormapMenu = [ this ](QActionGroup** groupOut) -> QMenu* {
        QMenu* menu = new QMenu();
        auto* group = new QActionGroup(this);
        group->setExclusive(true);
        const QStringList presets = QwtColorMapPreset::availablePresets();
        for (const QString& preset : presets) {
            QAction* presetAction = menu->addAction(preset);
            presetAction->setCheckable(true);
            presetAction->setData(preset);
            presetAction->setActionGroup(group);
            // 渐变预览：菜单项直接显示该预设的真实色带
            presetAction->setIcon(makeColormapPreviewIcon(preset));
            if (preset == QLatin1String("viridis")) {
                presetAction->setChecked(true);  // 默认色带与视图默认一致
            }
        }
        *groupOut = group;
        return menu;
    };

    mContextCategory = ribbon->addContextCategory(tr("Signal Analysis"), QColor(), QVariant());  // cn:信号分析
    if (nullptr == mContextCategory) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: addContextCategory failed";
        return;
    }
    mContextCategory->setObjectName(QString::fromUtf8(cContextCategoryObjectName));
    // 页 1「分析设置」必须排在最前：showContextCategory 会把「新出现页中的第一个」设为当前页
    buildAnalysisSettingsPage(mContextCategory);
    mContextDisplayPage = mContextCategory->addCategoryPage(tr("Signal Display"));  // cn:信号显示
    if (nullptr == mContextDisplayPage) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: addCategoryPage failed";
        return;
    }
    mContextDisplayPage->setObjectName(QString::fromUtf8(cContextDisplayPageObjectName));
    // D9：只给本页开启面板标题（SARibbonBar 在两行/单行紧凑样式下会强制关闭全部页的标题，
    // 样式切换后由 ribbonStyleChanged 重新下发，见 applyDisplayPagePanelTitles()）
    mContextDisplayPage->setEnableShowPanelTitle(true);

    // ---------- 时频图设置：显示模式 + 当前帧标记 ----------
    // 二维/三维/隐藏互斥：右下单元格在二维云图与三维瀑布之间切换（D12）
    mPanelTimeFreqView = mContextDisplayPage->addPanel(tr("Time-Frequency View"));  // cn:时频图设置
    mPanelTimeFreqView->setObjectName(QString::fromUtf8(cPanelContextTimeFreqViewName));
    mWaterfallModeGroup = new QActionGroup(this);
    mWaterfallModeGroup->setExclusive(true);
    actionWaterfall2D   = createIconAction("actionSignalWaterfall2D", pluginIcon("waterfall2d.svg"), true);
    actionWaterfall3D   = createIconAction("actionSignalWaterfall3D", pluginIcon("waterfall3d.svg"), true);
    actionWaterfallHide = createIconAction("actionSignalWaterfallHide", pluginIcon("waterfallHide.svg"), true);
    actionWaterfall2D->setActionGroup(mWaterfallModeGroup);
    actionWaterfall3D->setActionGroup(mWaterfallModeGroup);
    actionWaterfallHide->setActionGroup(mWaterfallModeGroup);
    actionWaterfall2D->setChecked(true);  // 默认二维（D12）
    mPanelTimeFreqView->addMediumAction(actionWaterfall2D);
    mPanelTimeFreqView->addMediumAction(actionWaterfall3D);
    mPanelTimeFreqView->addMediumAction(actionWaterfallHide);
    // 当前帧标记：二维云图的半透明虚线竖线 + 三维瀑布的当前迹线高亮，独立开关（默认开）
    actionTimeCursor = createIconAction("actionSignalTimeCursor", signalAppIcon("chart-vline-marker.svg"), true);
    connect(actionTimeCursor, &QAction::toggled, this, &DASignalAnalysisUI::onTimeCursorToggled);
    mPanelTimeFreqView->addLargeAction(actionTimeCursor);
    // 测量 ▾：联动缩放（双光标/峰值保持在后续步骤加入同一菜单）
    actionMeasure    = createIconAction("actionSignalMeasure", pluginIcon("measure.svg"), false);
    mMeasureMenu     = new QMenu();
    actionLinkedZoom = mMeasureMenu->addAction(tr("Linked zoom"));  // cn:联动缩放
    actionLinkedZoom->setObjectName(QStringLiteral("actionSignalLinkedZoom"));
    actionLinkedZoom->setCheckable(true);
    connect(actionLinkedZoom, &QAction::toggled, this, &DASignalAnalysisUI::onLinkedZoomToggled);
    actionDualCursor = mMeasureMenu->addAction(tr("Dual cursors"));  // cn:双光标测量
    actionDualCursor->setObjectName(QStringLiteral("actionSignalDualCursor"));
    actionDualCursor->setCheckable(true);
    connect(actionDualCursor, &QAction::toggled, this, &DASignalAnalysisUI::onDualCursorToggled);
    // 峰值保持：只在「跟随播放」开启时可用（否则没有逐帧谱来源）
    actionPeakHold = mMeasureMenu->addAction(tr("Peak hold"));  // cn:峰值保持
    actionPeakHold->setObjectName(QStringLiteral("actionSignalPeakHold"));
    actionPeakHold->setCheckable(true);
    connect(actionPeakHold, &QAction::toggled, this, &DASignalAnalysisUI::onPeakHoldToggled);
    actionResetPeakHold = mMeasureMenu->addAction(tr("Reset peak hold"));  // cn:重置峰值保持
    actionResetPeakHold->setObjectName(QStringLiteral("actionSignalResetPeakHold"));
    connect(actionResetPeakHold, &QAction::triggered, this, &DASignalAnalysisUI::onResetPeakHold);
    actionMeasure->setMenu(mMeasureMenu);
    mPanelTimeFreqView->addLargeAction(actionMeasure);
    // 视图 ▾：时间格式 / 布局比例 / 重置显示（都只影响显示，不重算分析）
    actionViewOptions = createIconAction("actionSignalViewOptions", pluginIcon("viewOptions.svg"), false);
    mViewOptionsMenu  = new QMenu();
    mTimeFormatMenu   = mViewOptionsMenu->addMenu(tr("Time format"));  // cn:时间格式
    mTimeFormatGroup  = new QActionGroup(this);
    mTimeFormatGroup->setExclusive(true);
    // 时间格式三态：0=秒 / 1=毫秒 / 2=mm:ss.s（data 里存格式编号）
    const QStringList timeFormatNames = { tr("Seconds"), tr("Milliseconds"), tr("mm:ss.s") };
    // cn:秒 / 毫秒 / 分:秒.十分位
    for (int i = 0; i < timeFormatNames.size(); ++i) {
        QAction* act = mTimeFormatMenu->addAction(timeFormatNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mTimeFormatGroup);
        mTimeFormatActions.append(act);
    }
    connect(mTimeFormatGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onTimeFormatTriggered);
    mLayoutPresetMenu = mViewOptionsMenu->addMenu(tr("Layout"));  // cn:布局比例
    // 布局比例预设：0=1:1 / 1=波形优先 / 2=分析优先 / 3=恢复默认（data 里存预设编号）
    const QStringList layoutPresetNames = { tr("1:1"), tr("Waveform priority"), tr("Analysis priority"), tr("Restore default") };
    // cn:1:1 / 波形优先 / 分析优先 / 恢复默认
    for (int i = 0; i < layoutPresetNames.size(); ++i) {
        QAction* act = mLayoutPresetMenu->addAction(layoutPresetNames.at(i));
        act->setData(i);
        mLayoutPresetActions.append(act);
    }
    connect(mLayoutPresetMenu, &QMenu::triggered, this, &DASignalAnalysisUI::onLayoutPresetTriggered);
    mViewOptionsMenu->addSeparator();
    actionDisplayReset = mViewOptionsMenu->addAction(tr("Reset display"));  // cn:重置显示
    actionDisplayReset->setObjectName(QStringLiteral("actionSignalDisplayReset"));
    connect(actionDisplayReset, &QAction::triggered, this, &DASignalAnalysisUI::resetDisplaySettings);
    actionViewOptions->setMenu(mViewOptionsMenu);
    mPanelTimeFreqView->addMediumAction(actionViewOptions);

    // ---------- 云图设置：色带 / 色标（只影响二维时频热图） ----------
    mPanelHeatmap = mContextDisplayPage->addPanel(tr("Heatmap"));  // cn:云图设置
    mPanelHeatmap->setObjectName(QString::fromUtf8(cPanelContextHeatmapName));
    actionColormap = createIconAction("actionSignalColormap", pluginIcon("colorMap.svg"), false);
    mColormapMenu  = makeColormapMenu(&mColormapGroup);
    connect(mColormapGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onContextColormapTriggered);
    actionColormap->setMenu(mColormapMenu);
    mPanelHeatmap->addLargeAction(actionColormap);
    // 色标（colorbar）显隐：时频热图右侧的色标轴，紧挨「色带」放置；只影响显示，不重算
    actionColorBar = createIconAction("actionSignalColorBar", pluginIcon("colorBar.svg"), true);
    actionColorBar->setChecked(mColorBarVisible);  // 初始勾选跟随已加载的偏好
    connect(actionColorBar, &QAction::toggled, this, &DASignalAnalysisUI::onColorBarToggled);
    mPanelHeatmap->addLargeAction(actionColorBar);
    // dB 范围：二维色域下限 [floor, 0]（D5：从窗口底部播放器控制条迁入，显示设置归显示页）
    mHeatmapDbFloorSpin = new QDoubleSpinBox(mPanelHeatmap);
    mHeatmapDbFloorSpin->setObjectName(QStringLiteral("da-signal-heatmap-dbfloor"));
    mHeatmapDbFloorSpin->setRange(-200.0, 0.0);
    mHeatmapDbFloorSpin->setDecimals(0);
    mHeatmapDbFloorSpin->setSingleStep(5.0);
    mHeatmapDbFloorSpin->setValue(-80.0);
    mHeatmapDbFloorSpin->setSuffix(QStringLiteral(" dB"));
    mHeatmapDbFloorSpin->setMinimumWidth(96);
    mHeatmapDbFloorSpin->setKeyboardTracking(false);  // 输入过程中不逐字符重绘
    connect(mHeatmapDbFloorSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onHeatmapDbFloorChanged);
    auto* heatmapGrid    = new SARibbonParamGrid(mPanelHeatmap);
    mHeatmapDbFloorLabel = heatmapGrid->leadingLabel(heatmapGrid->addRow(tr("dB"), mHeatmapDbFloorSpin));
    mPanelHeatmap->addLargeWidget(heatmapGrid);
    // 显示 ▾：图像/等高线/图像+等高线 与 最近邻/双线性（只影响二维云图的绘制方式）
    actionHeatmapDisplay = createIconAction(
        "actionSignalHeatmapDisplay", QStringLiteral(":/DAGui/icon/chart-type/chart-spectrogram.svg"), false);
    mHeatmapDisplayMenu      = new QMenu();
    mHeatmapDisplayModeGroup = new QActionGroup(this);
    mHeatmapDisplayModeGroup->setExclusive(true);
    mHeatmapDisplayMenu->addSection(tr("Display mode"));  // cn:显示模式
    mHeatmapDisplayModeSection         = mHeatmapDisplayMenu->actions().last();
    const QStringList displayModeNames = { tr("Image"), tr("Contour"), tr("Image + Contour") };
    // cn:图像 / 等高线 / 图像+等高线
    for (int i = 0; i < displayModeNames.size(); ++i) {
        QAction* act = mHeatmapDisplayMenu->addAction(displayModeNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mHeatmapDisplayModeGroup);
        mHeatmapDisplayModeActions.append(act);
    }
    mHeatmapResampleGroup = new QActionGroup(this);
    mHeatmapResampleGroup->setExclusive(true);
    mHeatmapDisplayMenu->addSection(tr("Interpolation"));  // cn:插值
    mHeatmapResampleSection         = mHeatmapDisplayMenu->actions().last();
    const QStringList resampleNames = { tr("Nearest neighbour"), tr("Bilinear") };
    // cn:最近邻 / 双线性
    for (int i = 0; i < resampleNames.size(); ++i) {
        QAction* act = mHeatmapDisplayMenu->addAction(resampleNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mHeatmapResampleGroup);
        mHeatmapResampleActions.append(act);
    }
    connect(mHeatmapDisplayModeGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onHeatmapDisplayModeTriggered);
    connect(mHeatmapResampleGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onHeatmapResampleTriggered);
    actionHeatmapDisplay->setMenu(mHeatmapDisplayMenu);
    mPanelHeatmap->addMediumAction(actionHeatmapDisplay);

    // ---------- 瀑布图设置：色带 / dB / 当前迹线高亮（只影响三维瀑布） ----------
    // 三维瀑布有自己独立的色带与 dB 色域（D12），与二维云图互不影响
    mPanelWaterfall = mContextDisplayPage->addPanel(tr("Waterfall"));  // cn:瀑布图设置
    mPanelWaterfall->setObjectName(QString::fromUtf8(cPanelContextWaterfallName));
    // 瀑布设置…：切右侧 3D 设置面板并定位到瀑布图元（详细样式在那里改）
    actionWaterfallSettings = createIconAction("actionSignalWaterfallSettings", pluginIcon("analysisSetting.svg"), false);
    connect(actionWaterfallSettings, &QAction::triggered, this, &DASignalAnalysisUI::onWaterfallSettingsRequested);
    mPanelWaterfall->addLargeAction(actionWaterfallSettings);
    actionWaterfallColormap = createIconAction("actionSignalWaterfallColormap", pluginIcon("colorMap.svg"), false);
    mWaterfallColormapMenu  = makeColormapMenu(&mWaterfallColormapGroup);
    connect(mWaterfallColormapGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onWaterfallColormapTriggered);
    actionWaterfallColormap->setMenu(mWaterfallColormapMenu);
    mPanelWaterfall->addLargeAction(actionWaterfallColormap);
    // dB 范围：三维固定色域 [floor, 0]（与二维云图的 dB 相互独立）
    mWaterfallDbFloorSpin = new QDoubleSpinBox(mPanelWaterfall);
    mWaterfallDbFloorSpin->setObjectName(QStringLiteral("da-signal-waterfall-dbfloor"));
    mWaterfallDbFloorSpin->setRange(-200.0, 0.0);
    mWaterfallDbFloorSpin->setDecimals(0);
    mWaterfallDbFloorSpin->setSingleStep(5.0);
    mWaterfallDbFloorSpin->setValue(-80.0);
    mWaterfallDbFloorSpin->setSuffix(QStringLiteral(" dB"));
    mWaterfallDbFloorSpin->setMinimumWidth(96);
    mWaterfallDbFloorSpin->setKeyboardTracking(false);
    connect(mWaterfallDbFloorSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onWaterfallDbFloorChanged);
    auto* waterfallGrid    = new SARibbonParamGrid(mPanelWaterfall);
    mWaterfallDbFloorLabel = waterfallGrid->leadingLabel(waterfallGrid->addRow(tr("dB"), mWaterfallDbFloorSpin));
    mPanelWaterfall->addLargeWidget(waterfallGrid);
    // 当前迹线高亮：三维瀑布里把当前帧对应的迹线着色（单色着色时无效并置灰）
    actionWaterfallHighlight =
        createIconAction("actionSignalWaterfallHighlight", pluginIcon("waterfallHighlight.svg"), true);
    connect(actionWaterfallHighlight, &QAction::toggled, this, &DASignalAnalysisUI::onWaterfallHighlightToggled);
    mPanelWaterfall->addLargeAction(actionWaterfallHighlight);

    // 初始不显示：由 provider 在信号分析页激活时显示
    ribbon->hideContextCategory(mContextCategory);
}

/**
 * @brief 给「信号显示」页重新下发面板标题开关（D9 spike）
 *
 * SARibbonBar::setRibbonStyle() 会按样式统一改写所有页的面板标题（两行/单行样式强制关闭），
 * 因此样式切换后必须把本页的开关再打开一次；页不存在时是安全的空操作。
 */
void DASignalAnalysisUI::applyDisplayPagePanelTitles()
{
    if (mContextDisplayPage) {
        mContextDisplayPage->setEnableShowPanelTitle(true);
    }
}

/**
 * @brief 构建峰值停靠窗口（右侧停靠区，与设置/智能体同区做标签页）
 */
void DASignalAnalysisUI::buildPeakDock()
{
    if (nullptr == mUI) {
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        return;
    }
    // 创建期间的显隐变化不算用户偏好（可能是创建即激活/随后隐藏）：
    // 用 mCreatingPeakDock 屏蔽，避免把偏好覆盖成「隐藏」（评审 N-U1）
    mCreatingPeakDock = true;
    mPeakPanel        = new DAPeakDockWidget();
    mPeakPanel->setObjectName(QStringLiteral("da-signal-peak-panel"));
    // 与「设置」同区作为标签页
    ads::CDockWidget* settingDock = dockingArea->dockingAreaToDockWidget(DADockingAreaInterface::DockingAreaSetting);
    ads::CDockAreaWidget* settingArea = settingDock ? settingDock->dockAreaWidget() : nullptr;
    if (settingArea) {
        mPeakDock = dockingArea->createDockWidgetAsTab(mPeakPanel, tr("Peaks"), settingArea);  // cn:峰值
    } else {
        mPeakDock = dockingArea->createDockWidget(mPeakPanel, ads::RightDockWidgetArea, tr("Peaks"));
        // cn:峰值
    }
    if (mPeakDock) {
        mPeakDock->setObjectName(QStringLiteral("da-dock-signal-peaks"));
        mPeakDock->setWindowTitle(tr("Peaks"));  // cn:峰值
        // 首次显示峰值窗口时按需计算每帧前 N 峰值曲线；显隐状态同步丝带按钮（评审 U14）
        connect(mPeakDock, &ads::CDockWidget::visibilityChanged, this, [ this ](bool visible) {
            if (actionPeakWindow) {
                const QSignalBlocker blocker(actionPeakWindow);
                actionPeakWindow->setChecked(visible);
            }
            if (!mCreatingPeakDock) {
                mPeakDockVisiblePref = visible ? 1 : 0;  // 记录用户偏好（N-U1/P2）
            }
            if (visible && !mPeakCurvesRequested) {
                requestPeaks(true);
            }
        });
        // 创建即激活会让插件加载后右侧直接跳到空的峰值页（评审 D24/U14）：先隐藏，
        // 首次导入成功后再按偏好自动切一次（见 showPeakDockForFirstSignal）
        mPeakDock->toggleView(false);
    }
    mCreatingPeakDock = false;
    connect(mPeakPanel, &DAPeakDockWidget::sendToTableRequested, this, &DASignalAnalysisUI::sendPeaksToTable);
    connect(mPeakPanel, &DAPeakDockWidget::exportCsvRequested, this, &DASignalAnalysisUI::exportPeaksCsv);
    connect(mPeakPanel, &DAPeakDockWidget::settingsRequested, this, &DASignalAnalysisUI::openAnalysisSettings);
}

/**
 * @brief 构建窗口底部播放器控制条
 *
 * 顺序：跟随播放 → ▶/⏸ → ■ → 时间滑块 → 时间文本 → 自动滚动。
 * 只留播放相关控件：dB 范围已归「信号显示 → 云图设置」（D5），显示设置不再留在播放条上。
 * 只在信号分析页 dock 激活时显示（provider 调 setSignalToolbarVisible）；
 * 控件只是当前视图/播放器状态的视图层：控件变化写回视图，状态变化经 syncSignalToolbar() 回填。
 * ▶/⏸ 按钮绑 actionPlay（可勾选 = 正在播放），■ 绑 actionStop；时间文本是纯函数格式化结果。
 */
void DASignalAnalysisUI::buildSignalToolbar()
{
    if (mSignalToolbar || nullptr == mUI) {
        return;
    }
    QMainWindow* mainWindow = mUI->getMainWindow();
    if (nullptr == mainWindow) {
        qWarning() << "DASignalAnalysisUI::buildSignalToolbar: main window is unavailable";
        return;
    }
    mSignalToolbar = new QToolBar(mainWindow);
    mSignalToolbar->setObjectName(QString::fromUtf8(cToolbarObjectName));
    mSignalToolbar->setMovable(false);
    mSignalToolbar->setFloatable(false);

    // 跟随播放：频谱/峰值表显示当前帧而不是整段
    mToolbarFollowCheck = new QCheckBox(mSignalToolbar);
    mToolbarFollowCheck->setObjectName(QStringLiteral("da-signal-toolbar-follow"));
    mToolbarFollowCheck->setText(tr("Follow playback"));  // cn:跟随播放
    mToolbarFollowCheck->setToolTip(tr("Show the current frame while playback advances"));
    // cn:播放推进时显示当前帧
    connect(mToolbarFollowCheck, &QCheckBox::toggled, this, [ this ](bool on) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setFollowSlider(on);
        }
    });
    mSignalToolbar->addWidget(mToolbarFollowCheck);
    mSignalToolbar->addSeparator();

    // 播放/暂停（单切换按钮，勾选=正在播放）与停止：动作是唯一状态源，
    // 按钮经 setDefaultAction 跟随动作的 enabled/checked/icon/text
    mToolbarPlayButton = new QToolButton(mSignalToolbar);
    mToolbarPlayButton->setObjectName(QStringLiteral("da-signal-toolbar-play"));
    mToolbarPlayButton->setDefaultAction(actionPlay);
    mToolbarPlayButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mSignalToolbar->addWidget(mToolbarPlayButton);
    mToolbarStopButton = new QToolButton(mSignalToolbar);
    mToolbarStopButton->setObjectName(QStringLiteral("da-signal-toolbar-stop"));
    mToolbarStopButton->setDefaultAction(actionStop);
    mToolbarStopButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mSignalToolbar->addWidget(mToolbarStopButton);
    mSignalToolbar->addSeparator();

    // 时间滑块：毫秒范围（0..信号时长），与视图 currentTime/时间原点口径一致
    mToolbarTimeSlider = new QSlider(Qt::Horizontal, mSignalToolbar);
    mToolbarTimeSlider->setObjectName(QStringLiteral("da-signal-toolbar-time"));
    mToolbarTimeSlider->setRange(0, 1000);
    mToolbarTimeSlider->setMinimumWidth(180);
    mToolbarTimeSlider->setToolTip(tr("Playback position"));  // cn:播放位置
    connect(mToolbarTimeSlider, &QSlider::valueChanged, this, [ this ](int value) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setCurrentTime(value / 1000.0);
        }
    });
    mSignalToolbar->addWidget(mToolbarTimeSlider);

    // 时间文本：当前位置 / 总时长（mm:ss.s，无源显示 --:--.-）
    mToolbarTimeLabel = new QLabel(mSignalToolbar);
    mToolbarTimeLabel->setObjectName(QStringLiteral("da-signal-toolbar-timetext"));
    mToolbarTimeLabel->setToolTip(tr("Playback position / total duration"));  // cn:播放位置 / 总时长
    mSignalToolbar->addWidget(mToolbarTimeLabel);
    mSignalToolbar->addSeparator();

    // 自动滚动：热图随播放推进水平滚动
    mToolbarAutoScrollCheck = new QCheckBox(mSignalToolbar);
    mToolbarAutoScrollCheck->setObjectName(QStringLiteral("da-signal-toolbar-autoscroll"));
    mToolbarAutoScrollCheck->setText(tr("Auto scroll"));  // cn:自动滚动
    mToolbarAutoScrollCheck->setToolTip(tr("Scroll the time-frequency view while playback advances"));
    // cn:播放推进时滚动时频视图
    connect(mToolbarAutoScrollCheck, &QCheckBox::toggled, this, [ this ](bool on) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setHeatmapAutoScroll(on);
        }
    });
    mSignalToolbar->addWidget(mToolbarAutoScrollCheck);

    mainWindow->addToolBar(Qt::BottomToolBarArea, mSignalToolbar);
    mSignalToolbar->setVisible(false);  // 只在信号分析页激活时由 provider 显示
    syncSignalToolbar();
}

/**
 * @brief 构建「标记数量」下拉菜单（10/20/50，互斥）
 */
void DASignalAnalysisUI::buildPeakTopNMenu()
{
    if (nullptr == actionPeakTopN) {
        return;
    }
    mPeakTopNMenu  = new QMenu();
    mPeakTopNGroup = new QActionGroup(this);
    mPeakTopNGroup->setExclusive(true);
    actionPeakTop10 = mPeakTopNMenu->addAction(QStringLiteral("10"));
    actionPeakTop20 = mPeakTopNMenu->addAction(QStringLiteral("20"));
    actionPeakTop50 = mPeakTopNMenu->addAction(QStringLiteral("50"));
    for (QAction* act : { actionPeakTop10, actionPeakTop20, actionPeakTop50 }) {
        act->setCheckable(true);
        act->setActionGroup(mPeakTopNGroup);
    }
    actionPeakTop10->setChecked(true);
    actionPeakTopN->setMenu(mPeakTopNMenu);
}

}  // namespace DA
