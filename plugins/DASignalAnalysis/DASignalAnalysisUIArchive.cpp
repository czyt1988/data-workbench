#include "DASignalAnalysisUI.h"

// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QDomDocument>
#include <QDomElement>

// 3rdparty (SARibbon / ADS / qwt)
#include "qwt_plot_item.h"

// DA / DAFigure
#include "DACoreInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DAChartItemsManager.h"

// DASignalAnalysis
#include "Signal/DADataFrameSignalSource.h"
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"
#include "Archive/DASignalArchiveTask.h"
#include "Archive/DASignalFigureArchive.h"
#include "Archive/DAChartItemArchive.h"

namespace DA
{
/**
 * @brief 清空页面上全部信号与相关缓存（工程加载前调用，评审 U15/P4）
 */
void DASignalAnalysisUI::clearAllSignals()
{
    if (mPlayer) {
        mPlayer->stop();
        mPlayer->clearSignal();
    }
    if (mPage) {
        mPage->clearAll();
    }
    mParamsPerSignal.clear();
    mParamsVersion.clear();
    mSpectrumCache.clear();
    mSpectrumCacheChannel.clear();
    mMemoryBudgetVerdict.clear();
    clearPeakState();
    setSignalDependentActionsEnabled(false);
    updateRelinkActionState();
    mFirstSignalActivated = false;
    // 清空后无当前视图：上下文标签/底部工具栏回到空状态
    mPeakMarkersVisible = false;
    syncSignalContextControls();
    syncSignalToolbar();
}

/**
 * @brief 宿主清空工程：清理本插件残留（评审 N-U2/P5/U15 / Q7）
 *
 * 宿主在新建工程、加载工程前、快照回滚时调用 DAProjectInterface::clear()，
 * 那一步不会触碰插件（DAAppProject::clear 只清数据与图表）。这里做幂等清理：
 * 页面/峰值窗口从未创建时也要安全返回，只剩缓存与临时文件需要处理。
 */
void DASignalAnalysisUI::onProjectCleaned()
{
    // 后台任务先停：旧工程的信号数据不再需要，避免继续占用内存/临时文件
    if (mWorker) {
        mWorker->cancel();
    }
    mImportTaskActive = false;
    updateImportActionsState();
    if (mPlayer) {
        mPlayer->stop();
        mPlayer->clearSignal();
    }
    if (mPage) {
        clearAllSignals();  // 清信号标签页、峰值表/曲线、频谱缓存与动作使能
    } else {
        mParamsPerSignal.clear();
        mParamsVersion.clear();
        mSpectrumCache.clear();
        mSpectrumCacheChannel.clear();
        mMemoryBudgetVerdict.clear();
        clearPeakState();
        setSignalDependentActionsEnabled(false);
        mFirstSignalActivated = false;
    }
    updateRelinkActionState();
    // 工程清空后无当前视图：上下文标签/底部工具栏回到空状态（页面从未创建时也是安全空操作）
    syncSignalContextControls();
    syncSignalToolbar();
    // 纯「关闭工程」路径也要清掉临时 WAV（评审 P5），不必等到下次加载或插件卸载
    DASignalImporter::cleanupTempFiles();
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 序列化当前状态（信号来源引用 + 参数 + 视图状态）
 */
QByteArray DASignalAnalysisUI::serializeState() const
{
    QJsonArray arr;
    if (mPage) {
        for (const DASignalPtr& sig : mPage->allSignals()) {
            if (!sig) {
                continue;
            }
            QJsonObject o;
            o[ QStringLiteral("name") ] = sig->name();
            // 会话内唯一 id 持久化（BLK-02）：恢复时回填，figures.xml 的 signal-id
            // 存档键按它精确匹配，同名信号不串档；旧工程无该字段走 signal-name 回退
            o[ QStringLiteral("signalId") ] = double(sig->signalId());
            switch (sig->sourceType()) {
            case DASignalSource::DataFrameSource: {
                auto dfSrc                        = std::dynamic_pointer_cast< DADataFrameSignalSource >(sig->source());
                o[ QStringLiteral("sourceType") ] = QStringLiteral("dataframe");
                // 缺失占位也要把工程里记录的数据帧名存回去（评审 U16）
                o[ QStringLiteral("dataframeName") ] = dfSrc ? dfSrc->dataframeName() : QString();
                o[ QStringLiteral("timeColumn") ]    = dfSrc ? dfSrc->timeColumn() : QString();
                QJsonArray cols;
                if (dfSrc) {
                    for (const QString& c : dfSrc->valueColumns()) {
                        cols.append(c);
                    }
                }
                o[ QStringLiteral("valueColumns") ] = cols;
                break;
            }
            case DASignalSource::VideoSource:
                o[ QStringLiteral("sourceType") ] = QStringLiteral("video");
                o[ QStringLiteral("path") ]       = sig->source() ? sig->source()->sourceDescription() : QString();
                break;
            case DASignalSource::FileSource:
            default:
                o[ QStringLiteral("sourceType") ] = QStringLiteral("file");
                o[ QStringLiteral("path") ]       = sig->source() ? sig->source()->sourceDescription() : QString();
                break;
            }
            o[ QStringLiteral("sampleRate") ]  = sig->sampleRate();
            o[ QStringLiteral("timeOrigin") ]  = sig->timeOrigin();
            o[ QStringLiteral("calibration") ] = sig->calibration();
            o[ QStringLiteral("unit") ]        = sig->unit();
            o[ QStringLiteral("params") ]      = paramsForSignal(sig).toJson();
            QJsonObject view;
            view[ QStringLiteral("amplitudeMode") ] = mAmplitudeMode;
            view[ QStringLiteral("dbReference") ]   = mDbReference;
            // 视图状态（P1）：瀑布模式、色带/色标、网格比例、热图跟随
            if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
                view[ QStringLiteral("waterfallMode") ] = v->waterfallMode();
                view[ QStringLiteral("colormap") ]      = v->colormapName();
                view[ QStringLiteral("dbFloor") ]       = v->dbFloor();
                view[ QStringLiteral("autoScroll") ]    = v->isHeatmapAutoScroll();
                // 显示类开关（D6）：当前帧标记 / 联动缩放 / 双光标 / 峰值保持 / 时间格式 / 色标
                view[ QStringLiteral("timeCursor") ]      = v->isTimeCursorVisible();
                view[ QStringLiteral("linkedZoom") ]      = v->isLinkedZoom();
                view[ QStringLiteral("dualCursor") ]      = v->isDualCursorEnabled();
                view[ QStringLiteral("peakHold") ]        = v->isPeakHoldEnabled();
                view[ QStringLiteral("timeFormat") ]      = v->timeFormat();
                view[ QStringLiteral("colorBarVisible") ] = v->isColorBarVisible();
                // 三维瀑布：色带 / dB 色域 / 当前迹线高亮 / 图元样式（样式从图元读回）
                view[ QStringLiteral("waterfallColormap") ]  = v->waterfallColormapName();
                view[ QStringLiteral("waterfallDbFloor") ]   = v->waterfallDbFloor();
                view[ QStringLiteral("waterfallHighlight") ] = v->isWaterfallHighlighted();
                view[ QStringLiteral("waterfall") ]          = v->waterfallStyle();
                // 云图绘制方式：显示模式与插值（从谱图图元/栅格读回，谱图 transient 不随 figures.xml 保存）
                view[ QStringLiteral("heatmapDisplayMode") ] = v->heatmapDisplayMode();
                view[ QStringLiteral("heatmapResample") ]    = v->heatmapResampleMode();
                // 峰值标记开关：视图是状态源（F-09 起按信号存档，见 v->peakMarkersVisible()）
                view[ QStringLiteral("peakMarkersVisible") ] = v->peakMarkersVisible();
                // T7 起布局是保留式网格：存归一化行/列比例（旧 splitter 像素比例在加载时归一化兼容）
                QJsonArray rowsArr;
                const QList< qreal > gridRows = v->gridRowSizes();
                for (qreal size : gridRows) {
                    rowsArr.append(size);
                }
                QJsonArray colsArr;
                const QList< qreal > gridCols = v->gridColumnSizes();
                for (qreal size : gridCols) {
                    colsArr.append(size);
                }
                view[ QStringLiteral("gridRows") ]    = rowsArr;
                view[ QStringLiteral("gridColumns") ] = colsArr;
            }
            o[ QStringLiteral("view") ] = view;
            arr.append(o);
        }
    }
    QJsonObject root;
    root[ QStringLiteral("signals") ] = arr;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * @brief 从 JSON 重建信号（主线程；文件缺失时提示信号源缺失并跳过）
 *
 * T9：工程存档里同时带 figures.xml（figure 文档）与 chart-data（非 transient 图元）。
 * 先按 signals.json 重建信号与视图（旧工程兼容路径），再把 figures.xml 的布局/样式/
 * 峰值标记/用户图元应用到对应视图；分析结果（transient 图元）由视图按参数重新计算。
 * @param json signals.json 字节
 * @param figuresXml figures.xml 字节（旧工程为空，此时只走 signals.json view{} 兼容路径）
 * @param chartItems 存档里读出的非 transient 图元（可为空）
 */
void DASignalAnalysisUI::restoreState(const QByteArray& json, const QByteArray& figuresXml, const DAChartItemsManager* chartItems)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        // 工程包里没有本插件的存档条目（旧工程 / 保存时插件未启用 / 条目损坏）：
        // 之前打开过页面时必须清掉上一工程残留的信号与临时文件，否则旧信号会被
        // 再次保存进新工程（评审 N-U2/U15；宿主 clear() 不触碰插件，无法兜底）
        if (mPage) {
            clearAllSignals();
        }
        DASignalImporter::cleanupTempFiles();
        return;
    }
    const QJsonArray arr = doc.object().value(QStringLiteral("signals")).toArray();
    if (arr.isEmpty()) {
        // 工程里没有信号：保持页面惰性（从未打开过就不创建，避免打开普通工程时多出空页面）；
        // 之前打开过页面的话，清掉上一工程残留的信号与临时文件
        if (mPage) {
            clearAllSignals();
            DASignalImporter::cleanupTempFiles();
        }
        return;
    }
    // 工程里可能有信号记录：惰性创建页面后才能重建信号标签页
    if (!ensurePageCreated()) {
        return;
    }
    // 打开新工程/快照回滚前先清空旧信号与缓存，避免两个工程的信号叠加（评审 U15/P4）
    clearAllSignals();
    // 旧信号的临时抽取 WAV 随旧工程一起清理（评审 P5/I12）：此时信号与播放器均已清空
    DASignalImporter::cleanupTempFiles();
    DADataManagerInterface* mgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString type  = o.value(QStringLiteral("sourceType")).toString();
        const QString name  = o.value(QStringLiteral("name")).toString();
        DASignalPtr sig;
        QString error;
        if (type == QLatin1String("dataframe")) {
            const QString dfName = o.value(QStringLiteral("dataframeName")).toString();
            QStringList cols;
            const QJsonArray colsArr = o.value(QStringLiteral("valueColumns")).toArray();
            for (const QJsonValue& c : colsArr) {
                cols.append(c.toString());
            }
            const QString timeCol = o.value(QStringLiteral("timeColumn")).toString();
            const double savedFs  = o.value(QStringLiteral("sampleRate")).toDouble();
            DAData data;
            if (mgr) {
                const QList< DAData > all = mgr->getAllDatas();
                for (const DAData& d : all) {
                    if (d.isDataFrame() && d.getName() == dfName) {
                        data = d;
                        break;
                    }
                }
            }
            if (!data.isDataFrame()) {
                // 数据帧缺失（评审 U16）：不再直接跳过，而是保留「来源缺失」占位信号与参数，
                // 用户可用丝带「重新指定数据帧来源」重新绑定到现有数据帧
                auto placeholder = std::make_shared< DADataFrameSignalSource >(dfName, timeCol, cols, savedFs);
                sig              = std::make_shared< DASignal >(name, placeholder);
                qWarning() << "DASignalAnalysisUI: dataframe source missing, placeholder kept:" << dfName;
                reportError(tr("Signal source missing: data frame \"%1\" (signal \"%2\"). "
                               "Use \"Relink Data Frame Source\" to point it to an existing data frame.")
                                // cn:信号源缺失：数据帧「%1」（信号「%2」）。可用「重新指定数据帧来源」指向现有数据帧。
                                .arg(dfName, name));
            } else {
                sig = DASignalImporter::importDataFrame(data, timeCol, cols, savedFs, name, &error);
            }
        } else {
            QString path = o.value(QStringLiteral("path")).toString();
            if (!QFileInfo::exists(path)) {
                // 信号源缺失（评审 U16）：给「重新指定」入口，参数与采样率仍按工程里的记录恢复
                QMessageBox box(QMessageBox::Question,
                                tr("Signal Source Missing"),  // cn:信号源缺失
                                tr("Signal \"%1\" refers to a missing file:\n%2\n\nLocate the file again?")
                                    // cn:信号「%1」引用的文件不存在：%2。要重新指定文件吗？
                                    .arg(name, path),
                                QMessageBox::NoButton,
                                mPage);
                QPushButton* locateBtn = box.addButton(tr("Locate..."), QMessageBox::AcceptRole);  // cn:重新指定...
                box.addButton(tr("Skip"), QMessageBox::RejectRole);                                // cn:跳过
                box.setDefaultButton(locateBtn);
                box.exec();
                if (box.clickedButton() != locateBtn) {
                    continue;
                }
                const QString located =
                    QFileDialog::getOpenFileName(mPage,
                                                 tr("Locate Signal Source"),  // cn:重新指定信号源
                                                 QFileInfo(path).absolutePath(),
                                                 tr("Audio/Video files (*.wav *.aiff *.aif *.flac *.ogg *.mp3 *.m4a "
                                                    "*.mp4 *.mkv *.avi);;All files (*)"));
                // cn:音频/视频文件
                if (located.isEmpty()) {
                    continue;  // 用户取消：保持缺失状态，不创建半成品信号
                }
                path = located;
            }
            sig = (type == QLatin1String("video")) ? DASignalImporter::importVideoFile(path, &error)
                                                   : DASignalImporter::importAudioFile(path, &error);
            if (sig) {
                // 用保存时确认过的采样率（不再回读文件头，方案 §4.4）
                const double savedFs = o.value(QStringLiteral("sampleRate")).toDouble();
                if (savedFs > 0) {
                    sig->setSampleRate(savedFs);
                }
            }
        }
        if (!sig) {
            reportOperationFailure(tr("Failed to restore signal \"%1\".").arg(name), error);
            // cn:恢复信号「%1」失败。
            continue;
        }
        sig->setName(name);
        // 回填持久化的信号唯一 id（BLK-02）：figures.xml 的 signal-id 存档键按它匹配；
        // 旧工程没有该字段时保持构造时分配的会话内 id，恢复端走 signal-name 回退
        sig->restoreSignalId(qint64(o.value(QStringLiteral("signalId")).toDouble()));
        sig->setTimeOrigin(o.value(QStringLiteral("timeOrigin")).toDouble());
        sig->setCalibration(o.value(QStringLiteral("calibration")).toDouble(1.0));
        sig->setUnit(o.value(QStringLiteral("unit")).toString());
        mParamsPerSignal[ sig.get() ] = DASignalAnalysisParams::fromJson(o.value(QStringLiteral("params")).toObject());
        const QJsonObject view        = o.value(QStringLiteral("view")).toObject();
        if (view.contains(QStringLiteral("amplitudeMode"))) {
            mAmplitudeMode = view.value(QStringLiteral("amplitudeMode")).toString(mAmplitudeMode);
        }
        if (view.contains(QStringLiteral("dbReference"))) {
            mDbReference = view.value(QStringLiteral("dbReference")).toString(mDbReference);
        }
        // D7 兼容：旧档可能存了「dBFS + 相对 1.0」的矛盾组合，加载后统一按幅值口径重新解释
        mDbReference = DASignalAnalysisParams::dbReferenceForAmplitudeMode(mAmplitudeMode);
        // 峰值标记开关：按信号存档（F-09）。先在 addSignalAndActivate 之前把 UI 默认值对齐
        // （connectSignalView 会用它初始化新视图），随后在视图状态恢复处按本信号的值覆盖
        const bool savedPeakMarkersVisible =
            view.value(QStringLiteral("peakMarkersVisible")).toBool(mPeakMarkersVisible);
        mPeakMarkersVisible = savedPeakMarkersVisible;
        addSignalAndActivate(sig);
        // 恢复视图状态（P1）
        if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
            // 峰值标记显隐按本信号的存档值恢复（不是全局值；F-09）
            v->setPeakMarkersVisible(savedPeakMarkersVisible);
            v->setWaterfallMode(view.value(QStringLiteral("waterfallMode")).toInt(0));
            const QString cmap = view.value(QStringLiteral("colormap")).toString();
            if (!cmap.isEmpty()) {
                v->setColormapName(cmap);
            }
            if (view.contains(QStringLiteral("dbFloor"))) {
                v->setDbFloor(view.value(QStringLiteral("dbFloor")).toDouble(v->dbFloor()));
            }
            v->setHeatmapAutoScroll(view.value(QStringLiteral("autoScroll")).toBool(false));
            // 显示类开关（D6）：缺键取默认（旧档兼容）
            v->setTimeCursorVisible(view.value(QStringLiteral("timeCursor")).toBool(true));
            v->setLinkedZoom(view.value(QStringLiteral("linkedZoom")).toBool(false));
            v->setDualCursorEnabled(view.value(QStringLiteral("dualCursor")).toBool(false));
            // 峰值保持只在「跟随播放」开启时可用，而跟随开关不随工程保存：恢复时一律关闭
            // （保留 peakHold 键仅作前向兼容），避免出现「勾选了但不可用」的开关
            if (view.value(QStringLiteral("peakHold")).toBool(false) && v->isFollowSlider()) {
                v->setPeakHoldEnabled(true);
            }
            v->setTimeFormat(view.value(QStringLiteral("timeFormat")).toInt(0));
            // 三维瀑布显示状态（D12）：色带 / dB 色域 / 当前迹线高亮 / 图元样式
            const QString waterfallColormap = view.value(QStringLiteral("waterfallColormap")).toString();
            if (!waterfallColormap.isEmpty()) {
                v->setWaterfallColormapName(waterfallColormap);
            }
            if (view.contains(QStringLiteral("waterfallDbFloor"))) {
                v->setWaterfallDbFloor(view.value(QStringLiteral("waterfallDbFloor")).toDouble(v->waterfallDbFloor()));
            }
            v->setWaterfallHighlight(view.value(QStringLiteral("waterfallHighlight")).toBool(true));
            // 瀑布样式：STFT 异步，图元未建时视图先记为待应用，首建后套用
            v->applyWaterfallStyle(view.value(QStringLiteral("waterfall")).toObject());
            // 云图绘制方式：显示模式与插值（谱图 transient，恢复时先记为待应用）
            v->setHeatmapDisplayMode(view.value(QStringLiteral("heatmapDisplayMode")).toInt(0));
            v->setHeatmapResampleMode(view.value(QStringLiteral("heatmapResample")).toInt(0));
            if (view.contains(QStringLiteral("colorBarVisible"))) {
                const bool colorBar = view.value(QStringLiteral("colorBarVisible")).toBool(mColorBarVisible);
                v->setColorBarVisible(colorBar);
                mColorBarVisible = colorBar;  // 偏好跟随最后一个恢复的信号
            }
            // 网格比例：优先新键 gridRows/gridColumns；旧工程只有 splitterVertical/splitterHorizontal
            // （像素值），交给 setGridSizes 内部归一化，保证旧存档仍能还原比例
            QList< qreal > gridRows;
            QList< qreal > gridCols;
            const QJsonArray rowsArr = view.value(QStringLiteral("gridRows")).toArray();
            if (!rowsArr.isEmpty()) {
                for (const QJsonValue& s : rowsArr) {
                    gridRows.append(s.toDouble());
                }
            } else {
                for (const QJsonValue& s : view.value(QStringLiteral("splitterVertical")).toArray()) {
                    gridRows.append(s.toDouble());
                }
            }
            const QJsonArray colsArr = view.value(QStringLiteral("gridColumns")).toArray();
            if (!colsArr.isEmpty()) {
                for (const QJsonValue& s : colsArr) {
                    gridCols.append(s.toDouble());
                }
            } else {
                for (const QJsonValue& s : view.value(QStringLiteral("splitterHorizontal")).toArray()) {
                    gridCols.append(s.toDouble());
                }
            }
            v->setGridSizes(gridRows, gridCols);
            v->setAmplitudeDisplay(mAmplitudeMode, mDbReference);
        }
    }
    // T9：把随信号存档的 figure 文档（网格比例/子图样式/峰值标记/用户图元）应用到恢复后的视图；
    // 旧工程没有 figures.xml 时跳过，沿用 signals.json view{} 的兼容恢复结果
    applyFiguresXml(figuresXml, chartItems);
    // 丝带上下文标签与底部工具栏跟随恢复后的当前视图（含瀑布模式/通道/色带/工具开关）
    syncSignalContextControls();
    syncSignalToolbar();
    // 恢复后当前信号可能是「来源缺失」占位：刷新重新指定入口可用性（评审 U16）
    updateRelinkActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 创建工程存档任务（isSave 时收集状态；加载时在回调里重建信号）
 *
 * T9：保存时在主线程生成 signals.json + figures.xml，并把非 transient 图元登记到
 * DAChartItemsManager；BLK-01：登记后立刻在 GUI 线程 makeSnapshots 预序列化成字节
 * 快照，交给后台任务的是**快照列表**（不是 manager）——保存期间 GUI 线程（嵌套
 * 事件循环继续派发定时器）可能删除/重建峰值标记等图元，后台线程只写字节，与活指针
 * 完全解耦。加载时任务先读 chart-data 与 figures.xml，回调里再恢复信号并应用 figure 文档。
 */
std::shared_ptr< DAAbstractArchiveTask > DASignalAnalysisUI::createArchiveTask(bool isSave)
{
    if (isSave) {
        DAChartItemsManager chartItems;
        const QByteArray figuresXml = buildFiguresXml(&chartItems);
        // BLK-01：GUI 线程先把图元序列化成字节快照，后台任务只写字节（不携带 QwtPlotItem*）
        const QList< DAChartItemSnapshot > itemSnapshots = DAChartItemArchive::makeSnapshots(&chartItems);
        auto task = std::make_shared< DASignalArchiveTask >(serializeState(), figuresXml, itemSnapshots);
        return task;
    }
    auto task = std::make_shared< DASignalArchiveTask >();
    task->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) {
        auto sigTask = std::static_pointer_cast< DASignalArchiveTask >(t);
        if (!sigTask) {
            return;
        }
        if (!sigTask->archiveEntryFound()) {
            // 工程包里没有 signal-analysis/signals.json（旧工程/保存时插件未启用/条目损坏）：
            // restoreState 的空数据分支会清掉上一工程残留信号与临时文件（评审 N-U2）
            qInfo() << "DASignalAnalysis: project has no signal-analysis archive entry; stale signals are cleared";
        }
        if (sigTask->archiveEntryFound() && !sigTask->figuresEntryFound()) {
            // 旧工程（T9 之前保存）没有 figures.xml：走 signals.json view{} 的兼容恢复路径
            qInfo() << "DASignalAnalysis: project has no signal-analysis/figures.xml; restore legacy view state";
        }
        // chartItems 指向任务内的图元管理者，任务在回调期间仍存活（shared_ptr 由调用方持有）
        restoreState(sigTask->loadedData(), sigTask->loadedFiguresXml(), &sigTask->chartItemsManager());
    });
    return task;
}

/**
 * @brief 构建随工程存档的 figures.xml（逐信号 figure XML + 非 transient 图元）
 *
 * 只在主线程调用：figure XML 生成需要读取控件状态，并把非 transient 图元
 * （峰值标记/用户添加的标注等）登记到 itemsMgr；后台存档任务只负责写字节。
 * @param chartItems 输出：图元 key 映射（保存任务据此写 chart-data），不可为空
 * @return figures.xml 字节；没有页面/信号时返回**空字节**（F-08：不再写一个只有根节点的
 *         figures.xml 与空 chart-data，存档任务按 !isEmpty 判据跳过它们）
 */
QByteArray DASignalAnalysisUI::buildFiguresXml(DAChartItemsManager* chartItems) const
{
    QList< DASignalAnalysisView* > views;
    if (mPage) {
        const QList< DASignalPtr > signalList = mPage->allSignals();
        for (const DASignalPtr& sig : signalList) {
            if (!sig) {
                continue;
            }
            if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
                views.append(view);
            }
        }
    }
    if (views.isEmpty()) {
        // 空工程 / 页面从未创建：没有 figure 可写，跳过 figures.xml 与 chart-data（F-08）
        qInfo() << "DASignalAnalysisUI::buildFiguresXml: no signal view, figures.xml is skipped";
        return QByteArray();
    }
    return DASignalFigureArchive::saveFigures(views, chartItems);
}

/**
 * @brief 把 figures.xml 的 figure 状态应用到恢复后的信号视图
 *
 * 匹配按 signal-id 属性（同名信号不串档，BLK-02）；旧存档（无 signal-id）回退按
 * signal-name。恢复端防争用：figure 元素从候选集合里逐个「取走」（同一元素只被一个
 * 视图消费），chart-data 图元 key 跨视图共享登记（一个 QwtPlotItem 只 attach 到一个
 * 子图，重复命中跳过并 qWarning）。
 * 恢复失败只记录诊断（英文）并保持默认布局，不影响信号本体恢复。
 * 旧工程没有 figures.xml 时直接返回（兼容路径由 restoreState 的 view{} 分支负责）。
 */
void DASignalAnalysisUI::applyFiguresXml(const QByteArray& figuresXml, const DAChartItemsManager* chartItems)
{
    // F-10：图元管理者不持有所有权，未被任何子图消费的图元（figure 缺失/损坏/角色对不上）
    // 在这里没人释放。统一回收「没有挂到任何 plot」的图元，并给用户可见提示；
    // 已 attach 的图元归子图所有，绝不在这里删（回收口径见 DASignalFigureArchive::releaseUnattachedItems）。
    auto releaseUnattachedItems = [ this, chartItems ]() {
        const int released = DASignalFigureArchive::releaseUnattachedItems(chartItems);
        if (released <= 0) {
            return;
        }
        qWarning() << "DASignalAnalysisUI::applyFiguresXml: released" << released
                   << "unattached chart item(s) that no sub chart consumed";
        reportError(tr("Some chart items could not be restored and were discarded (%1 item(s)).")
                    // cn:部分图元未能恢复，已丢弃（%1 个）。
                    .arg(released));
    };
    if (figuresXml.isEmpty() || nullptr == mPage) {
        releaseUnattachedItems();
        return;
    }
    QDomDocument figureDoc;
    QString parseError;
    if (!figureDoc.setContent(figuresXml, &parseError)) {
        qWarning() << "DASignalAnalysisUI::applyFiguresXml: cannot parse figures.xml:" << parseError;
        releaseUnattachedItems();
        return;
    }
    // 候选 figure 元素逐个消费：同一元素不会被两个视图命中（旧存档同名信号按顺序错开）
    QList< QDomElement > candidates = DASignalFigureArchive::figureElements(figureDoc);
    // 已 attach 的 chart-data 图元 key：跨视图共享，防止同一 QwtPlotItem 被两个子图争用
    QSet< QString > consumedItemKeys;
    const QList< DASignalPtr > signalList = mPage->allSignals();
    for (const DASignalPtr& sig : signalList) {
        if (!sig) {
            continue;
        }
        DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
        if (!view) {
            continue;
        }
        const QDomElement figureEle = DASignalFigureArchive::takeFigureElement(candidates, sig->signalId(), sig->name());
        if (figureEle.isNull()) {
            qWarning() << "DASignalAnalysisUI::applyFiguresXml: no figure element for signal" << sig->name() << "id"
                       << sig->signalId();
            continue;
        }
        if (!DASignalFigureArchive::restoreFigure(view, figureEle, chartItems, &consumedItemKeys)) {
            qWarning() << "DASignalAnalysisUI::applyFiguresXml: restore figure failed for signal" << sig->name();
        }
        // 峰值标记显隐按信号存档（F-09）：勾选状态取该视图接管到的标记的真实可见性；
        // UI 层只把**当前视图**的值留作「新建视图」的默认值，不再覆盖别的信号标签页
        if (!view->peakMarkers().isEmpty() && view == currentView()) {
            mPeakMarkersVisible = view->peakMarkersVisible();
        }
    }
    // 收尾：没有视图消费到的图元统一释放（F-10），避免加载半损坏存档时泄漏
    releaseUnattachedItems();
}

}  // namespace DA
