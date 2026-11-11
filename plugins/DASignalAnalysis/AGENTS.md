# DASignalAnalysis 插件开发指南

通用时序信号分析插件。主工作区可打开「信号分析」页面（惰性创建，与绘图/表格/工作流并列），
页面内每个信号一个标签页；丝带新增「信号分析」标签页。开发方案见
`tmp/信号分析插件开发方案.md`（决策记录 D1–D31 为定稿口径；**D11 已按用户要求修订为惰性创建**，
见 §页面惰性加载；**D7 已在 v1.3 修订**：FFmpeg 路径入口从分析设置对话框移到插件全局设置页，
探测顺序与配置键不变，见 §v1.3 修正要点；其余不得擅自更改）。

## 目录结构

```
plugins/DASignalAnalysis/
├── DASignalAnalysisPlugin.*      # 插件主类（DAAbstractPlugin，非节点插件）
├── DASignalAnalysisUI.*          # 丝带标签页 + 中央页面 + 动作 + 工程存档入口
├── Page/
│   ├── DASignalAnalysisPage.*    # 页面：QTabBar + QStackedWidget（每信号一个视图）
│   ├── DASignalAnalysisView.*    # 单信号视图：波形 + 频谱 + 热图/三维叠置 + 播放头
│   ├── DASignalChartPanel.*      # 单图容器（标题行 + DAChartWidget）
│   └── DAPeakDockWidget.*        # 峰值停靠窗口（表 2/3 + 曲线 1/3）
├── Signal/
│   ├── DASignal.*                # 信号对象（元数据 + 转发取数）
│   ├── DASignalSource.h          # 来源抽象（readRange 必须可跨线程）
│   ├── DAFileSignalSource.*      # 文件来源（音频/视频抽取结果）
│   ├── DADataFrameSignalSource.* # 数据帧来源（引用式）
│   └── DASignalImporter.*        # 导入编排 + FFmpeg 探测 + 临时文件清理
├── Analysis/
│   ├── DASignalAnalysisParams.*  # 参数结构体（JSON 键名与 Python 编排一致）
│   ├── DASignalAnalysisWorker.*  # Python 守护线程任务 + QTimer 轮询
│   ├── DASpectrumResult.h        # 频谱结果
│   ├── DAStftResult.h            # 时频显示栅格结果
│   ├── DAPeakResult.h            # 峰值表 + 每帧前 N 曲线
│   └── DAPeakTableModel.*        # 峰值表模型
├── Render/
│   ├── DAStftRasterData.*        # QwtMatrixRasterData 子类（显式 Z 色标范围）
│   └── DAWaterfallBuilder.*      # 三维瀑布抽帧抽频点
├── Playback/DASignalPlayer.*     # QMediaPlayer 封装（Qt5/Qt6 分支）
├── Dialogs/                      # 导入对话框 + 参数设置对话框 + 插件全局设置页
│   ├── DASignalImportDialog.*    # 导入对话框（音频 / 视频 / 数据帧）
│   ├── DASignalAnalysisSettingDialog.*  # 信号分析标签页「分析设置」：按信号生效的分析参数
│   └── DASignalAnalysisSettingPage.*    # 插件全局设置页（主页 → 设置 → 信号分析）：FFmpeg 路径
├── Form/DASignalFormSpecBuilder.*# DAFormSpec 表单规格（分组 + visible_when 联动）
├── Archive/DASignalArchiveTask.* # 工程存档任务（只存来源引用与参数）
└── PyScripts/DASignalAnalysis/   # Python 包（安装到 <app>/PyScripts）
    ├── importer.py               # 音频/视频读取与 FFmpeg 转码编排
    ├── spectrum.py               # 整段频谱/功率谱编排（调核心层纯函数）
    ├── stft.py                   # STFT 显示栅格（分块 + 双向池化）
    ├── peaks.py                  # 峰值表 + 每帧前 N 单趟扫描
    ├── envelope.py               # 波形包络降采样
    └── exporter.py               # 峰值/频谱/热图导出与送表格页
```

## 铁律

1. **v2 清单一条都不做**（`tmp/信号分析插件开发方案.md` §2.2）：滤波、重采样、包络解调、倒谱、
   倍频程、阶次分析、热图对数频率轴、声压级换算、选区循环播放、变速、智能体工具、工作流节点、
   矢量导出等。结构留口即可。
2. **决策记录 D1–D31 是定稿口径**：采样率必须用户确认（不静默用推断值）；峰值曲线每帧独立取前 N；
   瀑布默认二维；深色主题 v1 不做。发现冲突先停下报告。
3. **DSP 纯函数放核心层**（`src/PyScripts/DAWorkbench/DAPyBase/spectral.py`），
   插件只放面向信号的编排；不得在 `DAGui` 写 DSP，不得在 `DAFigure` 放界面控件。
4. **并发只走一套**：Python 守护线程 + `thread_status_manager` + C++ `QTimer` 轮询
   （`DASignalAnalysisWorker`）。不要新起 `QThread`；界面线程禁止 O(N) 计算。
5. **STFT 不整矩阵常驻**：`stft.py` 分块（256 帧）计算并池化进显示栅格后丢弃块矩阵；
   峰值曲线单趟扫描（512 帧/块，只留前 N）。
6. **`.cpp` 里出现 Qt↔Python 类型转换必须包含 `DAPybind11QtCaster.hpp`**；
   pybind11 类型入口用 `DAPybind11InQt.h`（内含 `#undef slots`），不要直接 include `python.h`。
7. **参数单一口径**：`DASignalAnalysisParams` 的 JSON 键名 = Python 编排读取的键名
   （`spectrum.py`/`stft.py`/`peaks.py`）；窗函数只有一份（频谱与 STFT 共用 `window`）。
8. **`overlap` 单位是百分比**（50 表示 50%），Python 侧自行除以 100。
9. **界面文本**：`tr("English") //cn:中文`；非 QObject 类（`DASignalFormSpecBuilder`）
   必须用 `QCoreApplication::translate("上下文", "字面量")`（lupdate 只认字面量）。
10. **动作经宿主 `DAActionsInterface` 创建与销毁**；停靠窗口用 `CDockWidget` 并在 `finalize()` 显式清理；
    插件持有的 `CDockWidget*` 与页面指针必须用 `QPointer` 保存（宿主可能在插件卸载前销毁 dockManager，
    裸指针会悬空）；对话框以插件页面为父（`mPage`），避免卸载后残留。
11. **信号来源干净**：文件来源读块、数据帧来源引用式（按块转换）；临时 WAV 随信号/控制器生命周期清理。
12. **工程只存来源引用、参数与视图状态**（`Archive/DASignalArchiveTask`），不存分析结果/STFT 矩阵/峰值数据；
    任务在后台线程只做文件读写，界面重建走 `loadedCallBack`。
13. **全局设置与按信号参数分开**：与具体信号/数据无关的全局项（当前为 FFmpeg 可执行文件路径）放插件设置页
    （`DASignalAnalysisPlugin::createSettingPage()` → `Dialogs/DASignalAnalysisSettingPage`，入口是 主页 → 设置）；
    信号分析标签页的「分析设置」对话框（`DASignalAnalysisSettingDialog`）只放按信号生效的分析参数，
    不得加入与数据无关的全局项。界面文案引用设置入口时统一写「设置 → 信号分析」。

## 页面惰性加载（修订 D11）

- `DASignalAnalysisUI::initialize()` 只创建丝带标签页、动作与后台 worker，**不创建**中央页面和峰值停靠窗口：
  大部分用户用不到信号分析，程序启动时不应多出一个空的中央标签页。
- 页面创建统一走 `ensurePageCreated()`：创建页面 + 页面内导入/标签事件连接 + 峰值停靠窗口 + 上次色带偏好；
  所有依赖页面的入口（三个导入槽、`addSignalAndActivate`、`restoreState`、丝带「打开信号分析窗口」）先调用它。
- `openSignalAnalysisPage()` 由丝带「窗口」面板的「打开信号分析窗口」大按钮触发：首次调用创建页面，
  页面已关闭时恢复显示并 raise（相当于 PPT 的「编辑模板」入口）。
- `serializeState()` / `finalize()` / `retranslateUi()` / 各后台结果回填必须容忍 `mPage == nullptr`
  （页面从未打开过的会话）；`DASignalAnalysisUI::page()` 未创建时返回 nullptr。
- 页面 dock 图标为 `icon/showSignalAnalysis.svg`（与绘图/表格/工作流中央页图标同一「窗口」语义）。

## 构建与测试

```powershell
.\scripts\build.ps1 -Target DASignalAnalysis        # 构建插件
.\scripts\build.ps1 -Target DASignalAnalysisTest    # 构建 C++ 测试（SUITE data）
.\scripts\test.ps1 -Suite data                      # 跑 data 套件（含本测试）
python -I -m pytest tests/test_signal_dsp.py tests/test_signal_import.py -q   # 纯 Python 测试
```

新增源文件放在上表的子目录后，需要重新 `cmake -S . -B build`（各目录用 `file(GLOB)` 收集）。

## v1.1 修正要点（评审整改后新增约定）

- **后台结果路由**：任务上下文统一用 `Analysis/DASignalTaskContext.h`（信号 + 通道 + 参数版本），
  worker 的 `*Ready` 信号首参都是它，界面层 `isContextCurrent()` 校验后才回填；
  不要再用 `mPendingSignal` 之类的单值回填槽（评审 U3/B4）。
- **参数版本**：参数变化（对话框确定、Top N、通道切换）时 `bumpParamsVersion()`，迟到结果直接丢弃；
  `poll()` 必须先摘除已完成任务再发信号，遍历 `mTasks` 用快照（评审 B2）。
- **幅值口径**：Python 计算层始终返回峰值幅度，均方根/dB 只在显示与导出层换算，
  唯一入口是 `Analysis/DASignalAmplitude.h`（频谱图、峰值表、CSV 共用，评审 C2/C4/I9）。
- **参数语义**：`zeroPadding` 是「额外点数」（频谱/峰值表/STFT 一致）；频率分辨率决定频谱与峰值表段长，
  STFT 计算仍按 `stftWindowMs` 取窗，但参数对话框里两者**二选一联动**（Δf = 1/窗长，换算函数
  `signalStftWindowMsFromResolution` / `signalResolutionFromStftWindowMs`），保证改分辨率时热图同步变化（评审 C3/C4）。
- **卸载清理**：`finalize()` 必须清理峰值停靠窗口、页面停靠窗口、播放器、worker、丝带标签页与动作；
  `mUI`/`mCategory` 为空时不能提前 return（评审 B5）。
- **峰值归属**：峰值表/曲线缓存带 `mCurrentPeaksSignal`/`mCurrentPeaksChannel`；切信号走
  `onCurrentSignalChanged()` 先清空再按新信号重算，禁止把上一个信号的峰值停在新标签页下（评审 U4）。
- **缺失信号源**：`restoreState()` 遇到文件不存在时弹「重新指定」，恢复后保留采样率与参数；
  数据帧缺失在 v1.2 起保留「来源缺失」占位信号并提供「重新指定数据帧来源」入口（见下节，评审 U16/Q7）。
- **PSD 出口**：韦尔奇功率谱密度已提供 CSV 导出（导出下拉里的「功率谱密度（CSV）」），
  换算口径为 10·log10（功率量），不随幅值模式变化（评审 C10）。
- **Qt 运行时**：插件私有的 `Multimedia` 不在主程序 exe 依赖链里。插件 CMake 在构建后以插件 DLL 为扫描目标
  调 `windeployqt` 补齐 `build/bin`；安装目录由顶层 `dafun_install_deploy_qt_runtime(... REQUIRED_QT_MODULES Multimedia)`
  在安装期校验，缺 DLL 直接安装失败（评审 D1）。
- **音频解码降级**：运行时缺 `soundfile` 时，**WAV 走标准库自实现的 RIFF 头解析 + 按帧 seek**
  （支持 PCM 8/16/24/32 位、IEEE float32/64、EXTENSIBLE 格式；`probe_audio` 只读文件头，
  不再整文件读入），AIFF/FLAC/OGG 在 `DASignalImporter::importAudioFile()` 里自动改走 FFmpeg 转码，
  没有 FFmpeg 才给出明确错误，不允许静默失败（评审 I1/N-I1a）；
  `scipy.io.wavfile` 仅用于**导出** WAV 的写路径。
- **测试选择**：`plugins/DASignalAnalysis/` 的改动在 `scripts/test.ps1 -Changed` 下会选中 `data` 套件。

## v1.2 修正要点（2026-11-09 复核整改）

- **工程生命周期**：`DASignalAnalysisUI::initialize()` 连接宿主 `DAProjectInterface::projectIsCleaned`
  （经 `core->getProjectInterface()`）。新建工程 / 加载工程前 / 快照回滚时宿主 `clear()` 会发该信号，
  `onProjectCleaned()` 幂等清理：停 worker/播放器、清信号与峰值缓存、`cleanupTempFiles()`。
  这样「工程包里没有 `signal-analysis/signals.json`」也不会把上一工程残留信号带进新工程（评审 N-U2/P5/U15）；
  `restoreState()` 的 `!doc.isObject()` 分支仍保留防御性清理，`DASignalArchiveTask::archiveEntryFound()`
  记录条目是否存在供界面判断。
- **临时文件按会话隔离**：插件临时目录为 `<tempDir>/session-<uuid>/`，统一由
  `DASignalImporter::sessionTempDir()/newSessionTempWavPath()/registerTempFile()` 分配与登记，
  `cleanupTempFiles()` 只删本会话登记过的文件；**禁止**再按 `*.wav` 后缀通删共享临时目录（评审 N-I12）。
  播放器（`DASignalPlayer`）不再自己按后缀删文件，只清自己的缓存表。
- **数据帧播放音频后台导出**：数据帧来源没有真实音频文件，`syncPlayerSource()` 先查
  `DASignalPlayer::cachedWavPath(信号, 通道)`，未缓存则 `DASignalAnalysisWorker::computePlaybackWav(ctx)`
  在 Python 守护线程导出（`playbackWavReady` 回填）；导出期间 `mPlaybackWavPendingKey` 非空，
  播放/暂停/停止动作置灰、状态栏显示进度，完成后 `setPreparedDataFrameWav()` 切源并恢复。
  迟到的导出结果按 `isContextCurrent()` 校验丢弃（评审 U9）。
- **取消入口**：丝带「分析」面板「取消计算」`actionCancelAnalysis`，enabled 绑
  `mWorker->runningTaskCount()>0`；触发 `cancelAll()`，每个任务走 `canceled(taskId)` →
  `onWorkerCanceled()` 只清进度不弹失败（评审 N-C7 UI 侧）。
- **幅值口径单一同步点**：参数对话框确定后必须调用 `syncAmplitudeUiFromParams(params)`，
  用 `QSignalBlocker` 同步 `actionAmpPeak/Rms/DbFs/DbRef1` 勾选并更新 `mAmplitudeMode/mDbReference`，
  否则峰值表/导出/工程存档会与视图口径分叉（评审 N-U3）。
- **内存预算门**：频谱、峰值表、包络、热图共用 `cAnalysisBudgetBytes`（1.5 GB）与
  `checkMemoryBudget()`；判定结果按信号缓存在 `mMemoryBudgetVerdict`（1=确认继续、2=拒绝），
  参数变化（对话框确定）时清除重新询问（评审 N-C1c/D19）。
- **dBFS 参考口径**：`DASignalAnalysisView::signalPeak()` 返回**去直流/去趋势后**的峰值
  （`updateSignalPeaks()` 由包络统计），原始峰值只用于波形/STFT 矩形纵向范围；
  `DASignalAmplitude::toDisplay()` 在参考 `<=0` 时回退 1.0（不再用 cEps 算出 +600 dB 的荒谬值，评审 N-C2）。
- **交互阈值**：波形画布上的 STFT 矩形拖动必须「按下只记候选 → 位移超过
  `QApplication::startDragDistance()` 才进入拖动 → 未拖动的释放交回 QwtPicker 定位」，
  命中判定用矩形包围盒（X/Y 都判断）；否则矩形内单击会被拖动逻辑吞掉（评审 N-U5）。
- **峰值窗口显隐偏好**：`peakDockVisible` 在 `ensurePageCreated()` 读取、`finalize()` 写回；
  首次导入信号时 `showPeakDockForFirstSignal()` 按偏好决定是否自动切到峰值窗口，
  用户关闭过的偏好不再被「首次导入自动显示」覆盖（评审 N-U1/P2）。
- **数据帧来源缺失占位**：`DADataFrameSignalSource` 支持「缺失占位」构造（数据帧名/列名/采样率保留、
  `isMissing()==true`、`isValid()==false` 不参与计算）；工程恢复时不再直接跳过，而是保留占位信号，
  丝带「信号」面板「重新指定数据帧来源」（`actionRelinkSource`）从数据管理器重选数据帧后
  `DASignalAnalysisPage::replaceSignalAt()` 原地替换并沿用参数与视图状态（评审 U16）。
- **失败消息口径**：UI 侧统一走 `reportOperationFailure(用户提示, 细节)`：界面只显示 `tr()` 后的提示，
  英文异常原文（worker `failed`、`e.what()`）只写日志（评审 §七-9）。
- **其他**：`ensurePageCreated()` 同时校验 `mPage` 与 `mPageDock`，dock 创建失败时删页面并
  `reportError`（N-U8）；所有「上下文过期丢弃结果」分支也要 `clearProgressIfIdle()`（N-U7）；
  `retranslateUi()` 必须重译峰值停靠窗口标题（§七-5）；视图与峰值窗口首次显示时显式 `setSizes`
  （上下/左右 1:1、峰值表:曲线 2:1，§七-1）；`mWaterfallItem` 重建前用 `getWaterfalls()` 校验挂载
  （N-H4）；视图类中非 const Qt 容器迭代已全部改 `std::as_const`/索引（N-U10）。

## v1.3 修正要点（全局设置拆分，2026-11-10）

- **动机**：`ffmpegPath` 原先混在「分析设置」对话框（`DASignalAnalysisSettingDialog`）里，
  未导入数据时该对话框不可打开，导致无法配置 FFmpeg。
- **新增插件设置页**：`DASignalAnalysisPlugin::createSettingPage()` 返回
  `Dialogs/DASignalAnalysisSettingPage`（`DAAbstractSettingPage` 子类），由宿主
  `AppMainWindow::showSettingDialog()` 追加到设置对话框页列表末尾；页面标题 `tr("Signal Analysis")`
  （cn:信号分析）。
- **页面内容**：`DAPropertyFormWidget` 表单（`tr("External Tools")`/外部工具 分组 + `tr("FFmpeg executable")`/
  FFmpeg 可执行文件 字段 + 浏览按钮），表单下方内联状态标签（留空显示 PATH 探测结果、有效绿色、
  无效橙红「文件不存在」）与一行说明「频率分辨率、窗函数、峰值门限与标定等分析参数在「信号分析」标签页的
  「分析设置」里按信号单独配置」；`setSignalAnalysisUI()` 注入插件 UI，`apply()` 后调用
  `DASignalAnalysisUI::onGlobalSettingsApplied()`（内部 `refreshFFmpegAvailability()` 并在消息面板留结果），
  视频导入入口立即刷新；路径无效时不落盘、表单回退到已保存值，`apply()` 里用 `daWarning` 记一条「路径未保存」。
- **设置页生命周期（与宿主约定）**：宿主 `AppMainWindow::showSettingDialog()` **每次打开都重建设置对话框**
  （`exec()` 返回即销毁），这是为了让插件设置页不会活过提供它的插件——插件在本进程内可被停用并卸载 DLL，
  若对话框常驻缓存，页面析构/绘制会跳进已卸载代码段。因此：插件不得把设置页指针缓存为长期成员；
  `finalize()` 也不需要清理设置页（对话框已先销毁）。宿主侧改动见 `src/APP/AppMainWindow.cpp`。
- **file 字段逐字符变更**：`DAGui` 的 `DAFormEditorRegistry::connectFile` 现在同时连
  `DAFilePathEditWidget::selectedPath` 与 `pathChanged`，表单值随输入即时变化（供内联校验与 dirty 标记）；
  程序化回填经 `QSignalBlocker` 不会误触发。输入路径的即时校验口径与 `DASignalImporter::ffmpegPath()` 一致：
  `exists + isFile + isExecutable`。
- **分析设置对话框**：移除 FFmpeg 路径行，只保留六组按信号生效的分析参数；未导入数据时
  「分析设置」按钮仍不可点（预期语义）。
- **文案**：所有「未检测到 FFmpeg，请在分析设置里指定路径」提示统一改为指向「设置 → 信号分析」。
- **配置不变**：仍为 `DASignalImporter` 读写的 `signal-analysis-config.json`（`ffmpegPath`、`colormap`、
  `peakDockVisible`），设置页与导入层共用同一份。

## v1.4 修正要点（取数架构与异步导入，2026-11-10）

- **取数唯一出口**：`DASignalAnalysisWorker::buildSourceDict()` 除 `type/path/dataframeName/column/channel/sampleRate/sampleCount` 外，
  额外注入 `source["read"]`（pybind11::cpp_function，捕获 `DASignalPtr`，签名为 `read(channel:int, start:int, count:int) -> float32 ndarray`，
  内部走 `DASignalSource::readRange` 的跨线程契约）。**Python 侧取数一律优先走它**：`spectrum._source_provider()` 识别后交给
  `spectral.amplitude_spectrum_segmented_provider / welch_psd_provider` 流式计算；`stft.compute_heatmap_sync` 按 256 帧块请求样本、
  块内池化进显示栅格后丢弃。无 reader 时保留原整段读取路径（pytest / 数据帧回退与旧口径不变）。
- **新增分析任务前先探测可读性**：`ensureAnalyzable()` 在提交任务前 `readRange(channel,0,1)` 探测；来源缺失占位与「数据帧改名/移除、
  文件不可用」分别给出可翻译的明确提示（英文细节只进日志），不要用「静默空结果」兜底。
- **Python 侧契约**：`source["read"]` 返回长度必须等于请求 count（不是则抛 RuntimeError）；`source["sampleCount"]` 必须与可读长度一致，
  否则流式分块会对不上尾部。频谱/STFT 的流式实现由 `tests/test_signal_dsp.py`、`tests/test_signal_import.py` 的 provider 用例锁定
  （帧位置 k*hop、frame_count、median 缓冲、linear detrend 与 scipy 一致性、请求有界且不整段）。
- **导入异步化（I2）**：视频/压缩音频导入拆成「Python 后台准备 + 界面收尾」——`importer.prepare_import_async(path, mode, ffmpeg_path, temp_wav)` 
  在守护线程做音频轨探测、FFmpeg 转码（`run_ffmpeg(..., cancel_check=...)` 用 Popen 轮询等待，可取消、删半成品）与结果可读性校验；
  `DASignalAnalysisWorker::importMediaAsync()` 提交 `ImportTask`，`poll()` 在界面线程调用
  `DASignalImporter::importPreparedMedia()` 做文件头 probe 与信号构造（不做转码），结果走 `importFinished(signal, error)`：
  成功 signal 非空；失败 error 为英文细节（只写日志）；取消时 signal 与 error 都为空（界面据此复位、不弹失败）。
- **导入在途状态**：`DASignalAnalysisUI::mImportTaskActive` 期间三个导入入口置灰（`updateImportActionsState()`），
  切工程/finalize/取消都要复位；临时 WAV 由 `newSessionTempWavPath()` 预分配并在取消/失败/构造失败时删除，
  成功构造的信号持有它，由 `cleanupTempFiles()` 按会话清理。
- **内存预算与降级（D5，2026-11-10 落地）**：估算公式统一在 `Analysis/DASignalMemoryEstimate.h`
  （纯逻辑、DASignalAnalysisTest 覆盖）：频谱/热图为流式，峰值内存只与段长/栅格相关；峰值表/峰值曲线
  含整段 float32 副本并按并发系数放大；包络按可见区段估算。超预算时三选：**降低精度**（热图栅格减半 +
  分块帧数 64；峰值曲线隔帧 `frame_step=2`）/ **只扫可见区段**（`view->visibleTimeRange()` 换算采样点，
  经 worker 传给 Python 的 `start_sample/end_sample`）/ **取消**。峰值曲线局部扫描时
  `DAPeakDockWidget::setPartialScan(true)` 在窗口上标注「局部扫描」；切信号/清空峰值状态必须复位。
  峰值表/峰值曲线仍整段取数（E 阶段再流式化），「降低精度」对它们只减少扫描帧数、不减少整段取数内存。
