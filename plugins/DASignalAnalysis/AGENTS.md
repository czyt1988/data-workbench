# DASignalAnalysis 插件开发指南

通用时序信号分析插件：主工作区注册「信号分析」中央页面（与绘图/表格/工作流并列，惰性创建），
页面内每个信号一个标签页；丝带另有一级「信号分析」标签页。信号只认「采样率 + 通道 + 时间原点」，
音频是一等导入格式，但不是唯一格式。

动手前先读 §铁律 与 §常见陷阱，再按修改点查 §关键机制。设计决策 D1–D31 的完整记录在
`tmp/信号分析插件开发方案.md`；本文只写当前有效口径，两者不一致时以代码和本文为准，先停下报告。

## 一、数据流

```
导入（音频 / 视频 / 数据帧）
  └─ DASignal（元数据）+ DASignalSource（跨线程 readRange 取数）
       └─ DASignalAnalysisWorker（GUI 线程提交、Python 守护线程计算、QTimer 轮询）
            └─ Python 编排（PyScripts/DASignalAnalysis/*.py → 核心层 DAPyBase/spectral.py）
                 └─ 结果结构体（DASpectrumResult / DAStftResult / DAPeakResult）
                      └─ 视图 / 峰值窗口 / 导出；工程只存来源引用、参数、视图状态与非 transient 图元
```

两条分层约束贯穿全插件：

- **计算与显示分离**：Python 只出数值（峰值幅度），显示换算、绘图与存档都在 C++。
- **DSP 纯函数在核心层**（`src/PyScripts/DAWorkbench/DAPyBase/spectral.py`），插件 Python 包只做面向
  信号的编排（读哪段、怎么分块、怎么池化、进度与取消）。不在 `DAGui` 写 DSP，不在 `DAFigure` 放界面控件。

## 二、目录结构

```
plugins/DASignalAnalysis/
├── DASignalAnalysisPlugin.*      # 插件主类：initialize/finalize/retranslate/createArchiveTask/createSettingPage
├── DASignalAnalysisUI.*          # 界面层：丝带标签页 + 动作 + 页面/峰值窗口惰性创建 + 存档编排
├── DASignalAnalysisGlobal.h      # 导出宏
├── DASignalAnalysisResource.qrc  # 图标资源（icon/）
├── Page/
│   ├── DASignalAnalysisPage.*    # 页面：空状态 + QTabBar + QStackedWidget（每信号一个视图）
│   ├── DASignalAnalysisView.*    # 单信号视图：DAFigureWidget 2×2（波形 / 频谱 / 热图或三维瀑布）
│   ├── DASignalAnalysisSettingContext.*  # 设置上下文 provider：停靠页激活时接管设置面板/图元树/上下文标签/底部工具栏
│   ├── DASignalChartExport.*     # 导出助手：整页 PNG（含 3D 合帧）、当前子图 PNG/SVG/PDF、剪贴板
│   ├── DAPeakDockWidget.*        # 峰值停靠窗口：上 2/3 峰值表、下 1/3 每帧前 N 曲线
│   ├── DAPeakCurvePlotSamples.*  # 峰值曲线采样口径：点全画、折线按跳变断开（纯函数）
│   └── DAPeakMarkerSamples.*     # 峰值标记吸附：落到当前已绘制曲线的最近局部极大（纯函数）
├── Signal/
│   ├── DASignal.*                # 信号对象：元数据 + 会话内唯一 id + 转发 readRange
│   ├── DASignalSource.h          # 来源抽象（readRange 必须可跨线程调用）
│   ├── DAFileSignalSource.*      # 文件来源（音频 / 视频抽取结果）
│   ├── DADataFrameSignalSource.* # 数据帧来源（引用式、按块 float64→float32、「来源缺失」占位）
│   └── DASignalImporter.*        # 导入编排 + FFmpeg 探测 + 会话临时文件 + 插件配置读写
├── Analysis/
│   ├── DASignalAnalysisParams.*  # 参数结构体（工程 JSON / 表单 / Python 字典三套映射）
│   ├── DASignalAnalysisWorker.*  # 后台任务协调：提交/轮询/去重/取消/同步短调用
│   ├── DASignalTaskContext.h     # 任务上下文（信号 + 通道 + 参数版本）
│   ├── DASignalAmplitude.h       # 幅值显示换算唯一入口（peak/rms/dbFS/dbRef1）
│   ├── DASignalMemoryEstimate.h  # 各阶段峰值内存估算（纯逻辑，测试直接断言）
│   ├── DASpectrumResult.h        # 频谱 / 功率谱结果
│   ├── DAStftResult.h            # 时频显示栅格结果
│   ├── DAPeakResult.h            # 峰值表 + 每帧前 N 曲线
│   └── DAPeakTableModel.*        # 峰值表模型
├── Render/
│   ├── DAStftRasterData.*        # QwtMatrixRasterData 子类（显式 Z 色标范围）
│   └── DAWaterfallBuilder.*      # 三维瀑布抽帧抽频点
├── Playback/DASignalPlayer.*     # QMediaPlayer 封装（Qt5/Qt6 分支；数据帧来源先后台导出临时 WAV）
├── Dialogs/
│   ├── DASignalImportDialog.*    # 导入对话框（音频 / 视频 / 数据帧；采样率必须用户确认）
│   ├── DASignalAnalysisSettingDialog.*  # 按信号生效的分析参数（六组）
│   └── DASignalAnalysisSettingPage.*    # 插件全局设置页（设置 → 信号分析）：FFmpeg 路径
├── Form/DASignalFormSpecBuilder.*# DAFormSpec 表单规格（分组 + visible_when 联动）
├── Archive/
│   ├── DASignalArchiveTask.*     # 工程存档任务（signals.json + figures.xml + chart-data）
│   └── DASignalFigureArchive.*   # figure XML 保存/恢复（signal-id 键、防争用）
├── icon/                         # 丝带与页面图标（改图标前先读根目录 DESIGN.md）
└── PyScripts/DASignalAnalysis/   # Python 包（安装到 <app>/PyScripts）
    ├── importer.py               # 音频/视频读取、FFmpeg 转码、WAV 兜底解析、异步导入准备
    ├── spectrum.py               # 整段频谱/功率谱编排（流式 provider）
    ├── stft.py                   # 热图栅格（分块池化）+ 单帧真实幅值谱
    ├── peaks.py                  # 峰值表 + 每帧前 N 单趟扫描
    ├── envelope.py               # 波形包络降采样
    └── exporter.py               # 峰值/频谱/热图 CSV 导出
```

## 三、铁律

1. **并发只有一套：Python 守护线程 + `thread_status_manager` + C++ `QTimer` 轮询。**
   不新起 `QThread`，不在界面线程做 O(N)。轮询槽在仍有任务时必须
   `pybind11::gil_scoped_release` + `QThread::msleep`：嵌入解释器由 GUI 线程初始化后一直持有 GIL，
   只轮询不让出时守护线程每帧只能等一个轮询周期，9M 点 @32kHz 的频谱实测从数秒拖到 20 分钟。
   当前 `cPollIntervalMs = 20`、`cGilYieldMs = 12`，调整要同时权衡回填延迟与界面事件处理。
   架构说明见 [embedded-python-gil.md](../../docs/zh/dev-guide/python-binding/embedded-python-gil.md)。

2. **界面线程只做短调用与显示换算。** 单次 C++→Python 调用控制在 ~10 ms 内（如
   `computeFrameSpectrumSync()` 只算一帧，失败写 `qWarning` 不弹消息）；其余一律提交给 worker。

3. **同一份分析只提交一次。** worker 用 `dedupeKey`（任务类型 + 信号 + 通道 + 参数版本；包络、热图、
   峰值曲线再拼请求尺寸/区段，频谱与峰值表只按类型+信号+通道+版本）命中在跑任务即复用。视图会在
   首次显示、外部连接后、resize 防抖、峰值窗口显示等时机重复请求，不挡的话同一份 O(N) 计算会跑多遍、
   出图成倍变慢。新增分析任务必须给出能区分「参数相同才算重复」的键。

4. **取数只有一条出口：`source["read"]`。** `buildSourceDict()` 注入
   `read(channel, start, count) -> float32 ndarray`，Python 侧优先用它流式分块；返回长度必须等于
   请求 count，`sampleCount` 必须与可读长度一致。新增分析任务前先 `ensureAnalyzable()` 探测可读性；
   来源缺失、数据帧改名/移除、文件不可用都要给可翻译的明确提示，**不允许**用「静默空结果」兜底。

5. **参数单一口径。** `DASignalAnalysisParams::toPythonDict()` 的键名就是 `spectrum.py`/`stft.py`/
   `peaks.py` 读取的键名；窗函数只有一份（频谱与 STFT 共用 `window`）；`overlap`/`stftOverlap` 是
   百分比（50 = 50%），Python 侧自行除以 100；`zeroPadding` 是「额外点数」（0 表示等于段长）。

6. **计算层只出峰值幅度，换算只在 `DASignalAmplitude`。** 频谱图、峰值表、峰值曲线、导出一律调用
   `toDisplay()`；dB 参考 <=0 时回退 1.0（不要用 cEps 算出 +600 dB 的荒谬值）。dBFS 参考峰值取
   去直流/去趋势后的统计口径（`signalPeak()`），原始峰值只用于波形与 STFT 矩形纵向范围。

7. **后台结果必须带上下文并经版本校验。** 所有 `*Ready` 信号首参是 `DASignalTaskContext`，界面层
   `isContextCurrent()` 通过才回填；参数或通道变化必须 `bumpParamsVersion()`。`poll()` 先摘除已完成
   任务再发信号，遍历任务表用快照。峰值表/曲线缓存要带所属信号与通道，切信号先清空，否则上一个
   信号的峰值会停在新标签页下。

8. **大内存必须过预算门，不许静默 OOM。** 估算统一在 `DASignalMemoryEstimate.h`；超过预算
   （`cAnalysisBudgetBytes`，1.5 GB）给三选：降低精度 / 只扫可见区段 / 取消。局部扫描要在峰值窗口
   标注「局部扫描」。判定结果按信号缓存，参数变化时清掉重新询问。

9. **工程只存来源引用、参数、视图状态与非 transient 图元。** 不存分析结果图元（包络/频谱/热图/
   瀑布）、STFT 矩阵、峰值表与峰值曲线数据。写模式携带 GUI 线程预序列化的**图元字节快照**
   （`DAChartItemArchive::makeSnapshots`），后台任务只写字节、不碰 `QwtPlotItem*`；界面重建走
   `loadedCallBack`。figure 存档键用 `signal-id`（`signal-name` 只兼容旧档）。

10. **全局设置与按信号参数分开。** 与数据无关的全局项（目前只有 FFmpeg 可执行文件路径）只放插件
    设置页（`createSettingPage()` → `Dialogs/DASignalAnalysisSettingPage`，入口「设置 → 信号分析」）；
    「分析设置」对话框只放按信号生效的分析参数。界面文案引用入口时统一写「设置 → 信号分析」。

11. **插件必须能热卸载干净。** 动作经宿主 `DAActionsInterface` 创建/销毁；`CDockWidget*` 与页面指针
    用 `QPointer`（宿主可能先销毁 dockManager）；对话框以页面为父；`finalize()` 清理峰值窗口、页面
    dock、播放器、worker、丝带标签与动作，且 `mUI`/`mCategory` 为空时**不能提前 return**。
    设置页指针不得长期缓存（宿主每次打开设置对话框都重建，见 §4.9）。

12. **类型转换边界照根 AGENTS.md 执行。** 任何 `.cpp` 出现「Qt 类型进 Python」（`attr(...)(QString)`、
    `pybind11::cast<QVariant>`、`.cast<QString>()` 等）必须 include `DAPybind11QtCaster.hpp`；
    pybind11 入口用 `DAPybind11InQt.h`（内含 `#undef slots`），不直接 include `python.h`。
    遗漏不报编译错误，运行时静默失败。

13. **界面文本与日志各走各的。** 用户可见文本用 `tr("English") //cn:中文`（Python 用
    `_("English")  # cn:中文`）；非 QObject 类（`DASignalFormSpecBuilder`）用
    `QCoreApplication::translate("上下文", "字面量")`，lupdate 只认字面量。技术诊断与异常原文用
    `qWarning/qCritical` 英文写日志；界面失败提示统一走 `reportOperationFailure(提示, 细节)`。

14. **不扩 v2 范围。** 滤波、重采样、包络解调、倒谱、倍频程、阶次分析、声压级换算、选区循环播放、
    变速、深色主题、智能体工具、工作流节点都不做，只留结构口。对数频率轴只做到频谱图，不要扩到
    热图。矢量导出是既有例外：2D 子图 PNG/SVG/PDF，3D 固定 PNG（`saveVector` 依赖 GL 上下文，
    未验证不扩）。

## 四、关键机制

### 4.1 页面惰性加载

- `initialize()` 只建丝带标签页、动作与后台 worker，**不建**中央页面和峰值窗口——多数用户用不到
  信号分析，程序启动不该多出一个空标签页。
- 页面统一走 `ensurePageCreated()`：建页面 + 连接导入/标签事件 + 建峰值窗口 + 读色带偏好。三个导入槽、
  `addSignalAndActivate`、`restoreState`、丝带「打开信号分析窗口」都先调它。
- `openSignalAnalysisPage()`：首次调用创建页面；页面已关闭时恢复显示并 raise。
- `serializeState()` / `finalize()` / `retranslateUi()` / 各后台结果回填都必须容忍 `mPage == nullptr`
  （页面从未打开过的会话）。页面 dock 图标为 `icon/showSignalAnalysis.svg`。
- 页面 dock 激活时由 `DASignalAnalysisSettingContext` 接管设置面板、图元树、上下文标签与底部工具栏；
  失活时还原宿主绘图区目标。

### 4.2 导入

- **音频**：WAV/AIFF/FLAC/OGG 直读；MP3/M4A/AAC 及视频走 FFmpeg 转临时 WAV。运行时缺 `soundfile` 时，
  WAV 用标准库 RIFF 头解析 + 按帧 seek（PCM 8/16/24/32、float32/64、EXTENSIBLE），AIFF/FLAC/OGG
  自动改走 FFmpeg；没有 FFmpeg 给明确错误，不静默失败，视频导入入口置灰。
- **数据帧**：引用式（不复制），时间列可选、采样率**必须用户确认**（推断值只作候选，不静默采用）。
- **异步**：视频/压缩音频导入拆两段——`importer.prepare_import_async()` 在守护线程做音频轨探测与
  FFmpeg 转码（可取消、删半成品）；`poll()` 在界面线程调 `DASignalImporter::importPreparedMedia()`
  只做文件头探测与信号构造（不转码）。结果走 `importFinished(signal, error)`：成功 signal 非空，
  失败 error 是只写日志的英文细节，取消时两者都为空（界面据此复位、不弹失败）。
- 导入在途 `mImportTaskActive` 期间三个导入入口置灰；切工程、取消、finalize 都要复位。
- **临时文件**：插件临时目录 `<tempDir>/session-<uuid>/`，由 `sessionTempDir()`/
  `newSessionTempWavPath()`/`registerTempFile()` 分配登记，`cleanupTempFiles()` 只删本会话登记过的文件。
  **禁止**按 `*.wav` 后缀通删共享临时目录。

### 4.3 后台任务

- 任务种类：频谱（含 PSD）、包络、热图、峰值表、峰值曲线、播放 WAV 导出、媒体导入。全部经
  `DASignalAnalysisWorker` 提交，进度写状态栏；`cancelAll()` 与丝带「取消计算」入口，
  enabled 绑 `runningTaskCount() > 0`；取消只走 `canceled(taskId)` 清进度，不弹失败。
- 失败经 `failed(error)`（英文细节）→ 界面 `reportOperationFailure()` 只显示翻译后的提示，并
  `clearProgressIfIdle()`；上下文过期丢弃结果的分支同样要清进度。
- 视图侧只发请求信号（`spectrumRequested`/`envelopeRequested`/`heatmapRequested`/`followFrameSpectrumRequested`），
  不自己算、也不缓存跨信号结果。

### 4.4 参数、幅值与内存预算

- 参数六组（预处理 / 频谱 / 功率谱 / STFT / 峰值 / 标定），一个信号标签页记一份，不进全局撤销栈；
  改参数后立即重算一次。
- `resolutionHz` 与 `stftWindowMs` 二选一联动（Δf = 1/窗长），换算函数
  `signalStftWindowMsFromResolution` / `signalResolutionFromStftWindowMs`；峰值最小间距 `<=0` 是自动哨兵，
  按 5×实际分辨率派生。
- 参数对话框确定后必须 `syncAmplitudeUiFromParams()`（`QSignalBlocker` 同步丝带勾选与
  `mAmplitudeMode/mDbReference`），否则峰值表、导出与存档口径会和视图分叉。
- 预算：频谱/热图/包络流式或按区段，峰值表/曲线含整段 float32 副本并按并发系数放大；
  详见 `DASignalMemoryEstimate.h` 的公式注释。

### 4.5 视图与三图联动

- 布局：`DAFigureWidget` 2×2 保留式网格——波形 (0,0,1,2)、频谱 (1,0)、热图/三维瀑布 (1,1)；
  瀑布默认二维（`setWaterfallMode` 0=二维 / 1=三维 / 2=隐藏）；播放头、STFT 窗口矩形、峰值标记是
  画布图元。视图内不放 inline 控件：显示类控件在丝带上下文标签、播放类在底部工具栏、外观类在右侧
  设置面板，视图的 setter/getter 是这些入口共同操作的状态源。
- **坐标轴**：`DAFigureWidget::createChart()` 会给新子图显式设 X 0..800 / Y 0..500 占位刻度（同时关掉
  autoscale）。`buildFigure()` 只在建图时对三张 2D 子图调用 `setAxisAutoScale(..., true)`；
  `rebuild*` 不无条件重开，否则用户缩放会被覆盖。工程恢复走存档轴状态。
- **色标**：`applyHeatmapColors()` 统一设 yRight 轴范围 `[mDbFloor, 0]`、色标宽 18、间距 4；隐藏时
  `setAxisVisible(yRight, false)` 不占横向空间。`setDbFloor`/`rebuildHeatmap`/`refreshAfterFigureRestore`
  都走这一入口，避免刻度与颜色对不上。显隐偏好存配置 `colorBarVisible`。
- **跟随播放**：频谱必须画「当前帧真实幅值谱」，不能拿热图显示栅格的当前列——栅格在频率方向做过
  max 池化（Δf_row 远大于峰值网格），两条曲线口径不同，峰值标记会落在下降沿。当前实现走
  `stft.frame_spectrum_sync()` + 帧号换算，与 `peaks.py` 单帧同口径、只读一帧（O(nperseg)）。
- 交互：STFT 矩形拖动必须「按下只记候选 → 超过 `QApplication::startDragDistance()` 才进入拖动 →
  未拖动的释放交回 QwtPicker 定位」，命中判定用包围盒（X/Y 都比）；缩放/平移/十字线启用时关闭
  「点击波形即定位」。

### 4.6 峰值

- 计算语义：**每帧独立取前 N**（D10）。峰值表整段一次算完；峰值曲线单趟扫描（512 帧/块，只留前 N），
  超预算时可按可见区段 + 隔帧降精度。
- 渲染：每帧第 k 名在相邻帧不一定是同一个峰，所以 `DAPeakCurvePlotSamples` 把每个名次拆成两层——
  点全画（缺峰帧跳过），折线只在相邻帧频率跳变 ≤ `8 × fs / nperseg` 时连接，跳变/缺帧处插入 NaN
  断开。不要为了「连成轨迹」改成跨帧匹配，那属于新功能。
- 标记：`snapPeakMarkersToCurve()` 把标记吸附到**当前已绘制曲线**（跟随播放时是当前帧谱）的最近局部
  极大，邻域半径 2、峰值数量由 Top N（10/20/50）决定；峰频超出曲线范围时跳过，无快照时回退旧口径。
  吸附是显示层口径，不改峰值表数值，也不要去改 Python 侧峰值算法的频率网格。
- 存档优先：加载后到第一次重算之间，`mPeakMarkersRestored` 保证不删除重建用户调整过的标记；
  `setPeaks()` 收到新计算结果时复位该窗口。
- 窗口显隐偏好 `peakDockVisible`：`ensurePageCreated()` 读取、`finalize()` 写回；首次导入只按偏好决定
  是否显示峰值窗口，**不得**覆盖用户关闭过的选择。
- 导出：峰值表 / 频谱 / 功率谱密度 / 热图矩阵 CSV，整页或当前子图图片，峰值表还可送表格页；
  数值一律先过 `DASignalAmplitude`，PSD 是功率量（10·log10），不随幅值模式变化。

### 4.7 内置「绘图操作」上下文

- 页面 dock 激活时，除插件自己的上下文标签外还要显示内置「绘图操作」标签，网格/缩放/平移/拾取/图例/
  主题/复制/导出/选区/标记等动作作用到当前信号 figure。宿主契约是
  `DASettingContextProvider::chartContextFigure()`，本插件由 `DASignalAnalysisSettingContext` 返回当前
  视图 figure。**插件不要自己 show/hide 内置 Chart 上下文。**
- 切信号、切 2D/3D 子图后 `refreshTarget()` 末尾调
  `DADockingAreaInterface::notifySettingContextChanged()`；失活还原后也要 notify。
- 交互工具由内置标签提供，插件不再自建缩放/平移/十字线/数据拾取动作；需要联动时监听当前
  `DAChartWidget::chartPropertiesChanged`（先断后连、`QPointer` 防悬空），`finalize()` 开头解绑。
- 结构性动作（新建绘图窗口/新建坐标系/调整子图大小/数据探针）由宿主在插件上下文中置灰；
  3D 子图保持标签显示，2D 类动作置灰。

### 4.8 工程存档

- 一个任务写三部分：`signal-analysis/signals.json`（来源引用 + 参数 + 视图状态）、
  `signal-analysis/figures.xml`（网格 + 子图样式 + 非 transient 图元 key）、
  `signal-analysis/chart-data`（图元二进制）。后两者只在有 figure 内容时写，旧工程可正常加载。
- 保存流程在主线程收集 figure XML 与图元快照，后台线程只写文件；恢复在主线程 `loadedCallBack` 里
  重建界面并应用 XML。figure 元素同时写 `signal-id`（精确匹配）与 `signal-name`（旧档回退）；
  一个 figure 元素只能被一个视图消费、一个 chart-data key 只能 attach 到一个子图（重复命中跳过并
  `qWarning`）。
- 宿主 `DAProjectInterface::projectIsCleaned`（新建工程 / 加载前 / 快照回滚）必须触发幂等清理：停
  worker/播放器、清信号与峰值缓存、清临时文件，否则「工程包里没有 signals.json」会把上一工程的
  信号带进新工程。
- 缺失来源：文件不存在时弹「重新指定」，恢复后保留采样率与参数；数据帧缺失保留「来源缺失」占位
  信号（`isMissing()`，不参与计算），丝带「重新指定数据帧来源」重选后 `replaceSignalAt()` 原地替换
  并沿用参数与视图状态。

### 4.9 设置

- 两个入口、两份内容：插件全局设置页（`createSettingPage()`，主页 → 设置 → 信号分析）只放与数据
  无关的全局项（FFmpeg 路径）；标签页「分析设置」对话框只放按信号生效的分析参数。
- 宿主 `AppMainWindow::showSettingDialog()` **每次打开都重建设置对话框**，目的是让插件设置页不会活过
  提供它的插件（插件可被停用并卸载 DLL，常驻页面析构会跳进已卸载代码段）。因此插件**不得**把设置页
  指针缓存为长期成员，`finalize()` 也不需要清理设置页。
- 全局设置 `apply()` 后调 `DASignalAnalysisUI::onGlobalSettingsApplied()` 刷新 FFmpeg 可用性；路径无效
  时不落盘、表单回退到已保存值。所有配置共用 `signal-analysis-config.json`
  （`ffmpegPath`、`colormap`、`peakDockVisible`、`colorBarVisible`），由 `DASignalImporter` 读写。

### 4.10 Python 包契约

- 每个编排模块提供 `compute_xxx_sync()`（测试直用）与 `compute_xxx_async()`（返回 task_id，守护线程执行），
  外加 `take_result(task_id)`、`cancel_task(task_id)`、`is_cancelled(task_id)`；进度经
  `thread_status_manager` 上报，C++ `poll()` 轮询。
- 取数优先用 `source["read"]`（流式）；没有 reader 时保留整段读取路径，供 pytest 与旧口径使用。
- `spectrum.py` 出整段频谱/PSD；`stft.py` 分块（256 帧）池化进显示栅格后丢弃块矩阵，并提供单帧
  幅值谱；`peaks.py` 单趟扫描每帧前 N；`envelope.py` 按区段降采样；`exporter.py` 只负责 CSV 与
  送表格页的数据准备。

## 五、常见陷阱

| 症状 | 根因 | 正确做法 |
|---|---|---|
| 大信号分析几十分钟不回来，空闲时后台 Python 线程完全不动 | 轮询槽只查状态、没让出 GIL | `gil_scoped_release` + `QThread::msleep(12)`（§铁律 1） |
| 同一份分析被跑好几遍，出图成倍变慢 | 视图多入口重复提交 | 按 `dedupeKey` 去重，新增任务必须给键 |
| 导入信号后三张图坐标轴固定在 0..800 / 0..500 | `createChart()` 显式设了占位刻度并关掉 autoscale | 建图时对 2D 子图打开 autoscale，`rebuild*` 不重开 |
| 色标刻度与颜色对不上、色带贴着绘图区 | yRight 轴仍是 Qwt 默认 0..1000 | 统一走 `applyHeatmapColors()` 设 `[dbFloor, 0]` |
| 峰值标记停在曲线旁边或下降沿 | 峰值表网格 ≠ 已绘制曲线网格；跟随用了热图栅格列 | 吸附到已绘制曲线快照；跟随改用帧谱（§4.6） |
| 峰值曲线窗口像「笼子」 | 相邻帧同名次不是同一个峰，却被直接连线 | 点全画、折线按 `8×Δf` 跳变断开 |
| 数据帧时间列明明选上了，却提示无时间列/采样率 0 | 接了 `currentChanged`，此时选中尚未生效 | 只接 `selectionChanged` |
| 波形矩形内单击无法定位（被拖动吞掉） | 没有拖动阈值，按下即进入拖动 | 超过 `startDragDistance()` 才拖，未拖动释放交回 picker |
| 关闭程序或卸载插件时崩溃 | `CDockWidget`/页面已被宿主销毁，插件仍用裸指针 | 用 `QPointer`，`finalize()` 判空 |
| 设置页析构跳进已卸载插件代码 | 插件缓存了设置页/设置对话框常驻 | 不缓存，依赖宿主每次重建（§4.9） |
| 工程加载后仍显示上一个工程的信号 | 只靠 `restoreState()` 的防御分支 | 连 `projectIsCleaned` 做幂等清理（§4.8） |
| 新加任务后 Python 侧静默失败（KeyError、参数丢失） | 忘了 include `DAPybind11QtCaster.hpp` | 每个做 Qt↔Python 转换的 `.cpp` 都要 include（§铁律 12） |
| 遍历被修改的 Qt 容器时出现莫名深拷贝/性能抖动 | 非 const 容器范围迭代触发 COW | `const` 声明或 `std::as_const()` |

## 六、构建与测试

```powershell
.\scripts\build.ps1 -Target DASignalAnalysis        # 构建插件
.\scripts\build.ps1 -Target DASignalAnalysisTest    # 构建 C++ 测试（SUITE data）
.\scripts\test.ps1 -Suite data                      # 跑 data 套件（含本测试）
.\scripts\test.ps1 -Changed                         # 按改动自动选 data 套件，并先构建插件目标

# 本插件的 Python 测试（DSP / 导入 / 峰值 / 取消）。要逐个列文件：整目录收集会撞上别的模块的采集错误
python -I -m pytest tests/test_signal_dsp.py tests/test_signal_import.py `
    tests/test_signal_peaks.py tests/test_signal_cancel.py -q
```

`tests/test_signal_manager.py` 测的是工作流信号管理器（`DAWorkFlowPy.signal_manager`），不属于本插件。

- `test.ps1 -Changed` 对 `plugins/DASignalAnalysis/` 的改动会选中 `data` 套件，并额外构建
  `DASignalAnalysis` 插件目标：UI/Page/Dialogs/Playback 源码只被插件 DLL 编译，测试目标只直接编译
  部分 `.cpp`，只构建测试目标会漏掉这些文件的编译错误。
- 新增源文件放进上表子目录后**无需手动 reconfigure**：各目录 `file(GLOB ... CONFIGURE_DEPENDS)`。
  但 `src/tst/DASignalAnalysisTest/CMakeLists.txt` 的 SOURCES 是显式列表，新增**参与测试**的纯逻辑
  文件（如 `Page/DAPeakMarkerSamples.cpp`）要手动补。
- C++ 测试覆盖纯逻辑：参数与幅值换算、内存估算、峰值曲线采样、峰值标记吸附、存档、导出。
  Python 测试覆盖 DSP（与 scipy 对拍）、provider 取数契约、导入与取消。
- Qt 运行时：`Multimedia` 不在主程序 exe 依赖链里。插件 CMake 构建后对插件 DLL 跑 `windeployqt`，
  安装期由 `dafun_install_deploy_qt_runtime(... REQUIRED_QT_MODULES Multimedia)` 校验，缺 DLL 直接
  安装失败。

## 七、相关文档

- [内嵌 Python 的 GIL 架构约束](../../docs/zh/dev-guide/python-binding/embedded-python-gil.md)——并发与轮询的架构依据
- [设置上下文提供者](../../docs/zh/dev-guide/ui/setting-context-provider.md)——§4.7 的宿主侧机制
- [图表格布局](../../docs/zh/dev-guide/graphics/figure-grid-layout.md)——2×2 网格与子图 API
- [测试体系](../../docs/zh/dev-guide/general/testing.md)——套件分组与 `test.ps1` 用法
- [用户指南：信号分析](../../docs/zh/use-guide/analysis/signal-analysis.md)——面向用户的功能说明
- [DESIGN.md](../../DESIGN.md)——改界面或 SVG 图标前必读
- [根 AGENTS.md](../../AGENTS.md)——模块依赖、i18n、日志、构建与提交规范
- `tmp/信号分析插件开发方案.md`——D1–D31 设计决策的历史记录（只作背景，不作规范）

## 八、维护约定

- 本文只记录**当前有效**的约束、机制与陷阱，不按版本号累积「修正要点」；行为变化时直接改对应章节，
  历史看 `git log` 与方案文档。
- 新增分析任务时必须同时补齐：`dedupeKey`、内存估算、取消路径、上下文版本校验、Python 侧流式
  provider 实现与 pytest 用例。
- 新增纯逻辑（不依赖 pybind11/GUI）优先做成可单测的纯函数（参考 `DASignalMemoryEstimate.h`、
  `DAPeakCurvePlotSamples.*`），并加入 `DASignalAnalysisTest` 的显式 SOURCES。