# DASignalAnalysis 插件开发指南

通用时序信号分析插件：主页「创建」面板有一个「信号分析」入口（惰性创建中央页面，与绘图/表格/工作流并列），
页面内每个信号一个标签页；丝带不再占用常驻顶级标签页，只在信号分析页获得焦点时出现
「分析设置」「信号显示」两个上下文标签页。信号只认「采样率 + 通道 + 时间原点」，
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
├── DASignalAnalysisUI.h          # 界面层单类 DASignalAnalysisUI（动作/面板/视图/缓存等全部状态都在这里）
├── DASignalAnalysisUI.cpp        # 生命周期与装配：initialize/finalize、页面惰性创建、上下文标签显隐、错误上报
├── DASignalAnalysisUIRibbon.cpp  # 丝带构建：主页入口 +「分析设置」「信号显示」两页 + 播放工具栏 + 峰值窗口
├── DASignalAnalysisUIDisplay.cpp # 显示页同步与显示类槽：sync*、色带、dB、时间格式、布局预设、测量 ▾
├── DASignalAnalysisUIImport.cpp  # 导入与信号标签：三入口、加入页面、视图连接、重新指定来源
├── DASignalAnalysisUIAnalysis.cpp# 参数与计算：参数读写、分级重算、内存预算、后台结果回填、状态栏进度
├── DASignalAnalysisUIPeaks.cpp   # 峰值：峰值表/曲线回填、标记开关与文本模板、峰值窗口显隐
├── DASignalAnalysisUIExport.cpp  # 导出：峰值/频谱/PSD/热图 CSV、整页与子图图片、剪贴板、送表格页
├── DASignalAnalysisUIArchive.cpp # 工程存档：signals.json 序列化与恢复、figures.xml、工程清理
├── DASignalAnalysisUIPlayback.cpp# 播放：播放源同步、播放/暂停/停止、时间轴联动、WAV 导出回填
├── DASignalAnalysisUIRetranslate.cpp # 重译：retranslateUi（纯文本）
├── DASignalAnalysisUIHelpers.h   # 上述实现文件共享的 objectName/预算/防抖常量与无状态 helper
├── DASignalAnalysisGlobal.h      # 导出宏
├── DASignalAnalysisResource.qrc  # 图标资源（icon/）
├── Page/
│   ├── DASignalAnalysisPage.*    # 页面：空状态 + QTabBar + QStackedWidget（每信号一个视图）
│   ├── DASignalAnalysisView.*    # 单信号视图：DAFigureWidget 2×2（波形 / 频谱 / 热图或三维瀑布）
│   ├── DASignalAnalysisSettingContext.*  # 设置上下文 provider：停靠页激活时接管设置面板/图元树/上下文标签/播放器控制条
│   ├── DASignalChartExport.*     # 导出助手：整页 PNG（含 3D 合帧）、当前子图 PNG/SVG/PDF、剪贴板
│   ├── DASignalTimeText.*        # 播放器控制条时间文本（mm:ss.s 与「当前 / 总长」占位，纯函数）
│   ├── DAPeakDockWidget.*        # 峰值停靠窗口：上 2/3 峰值表、下 1/3 每帧前 N 曲线
│   ├── DAPeakCurvePlotSamples.*  # 峰值曲线采样口径：点全画、折线按跳变断开（纯函数）
│   ├── DAPeakMarkerLabel.*       # 峰值标记文本模板：{index}/{fre}/{amp} 占位符渲染（纯函数）
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
│   ├── DAStftRasterData.*        # QwtMatrixRasterData 子类（显式 Z 色标范围；默认最近邻插值）
│   ├── DAWaterfallBuilder.*      # 三维瀑布抽帧抽频点 + 迹线时间/dB 换算纯函数
│   ├── DAWaterfallColor.*        # Qwt3DColorMapColor 子类：当前帧迹线高亮
│   └── DAColormapMatch.*         # 从色图反查预设名（色带回读，纯函数）
├── Page/…                        # 另含 DATimeScaleDraw.*（时间轴格式，在 Render/）与
│                                 # DADualCursorReadout.*（双光标读数，在 Page/）
├── Playback/DASignalPlayer.*     # QMediaPlayer 封装（Qt5/Qt6 分支；数据帧来源先后台导出临时 WAV）
├── Dialogs/
│   ├── DASignalImportDialog.*    # 导入对话框（音频 / 视频 / 数据帧；采样率必须用户确认）
│   ├── DASignalAnalysisSettingDialog.*  # 按信号生效的高级分析参数（常用项已搬到丝带；删了 dB 参考）
│   ├── DAPeakMarkerDisplayDialog.*      # 峰值标记文本模板编辑（占位符按钮 + 编辑框 + 渲染预览）
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

10. **全局设置与按信号参数分开，常用参数在丝带、高级项在对话框。** 与数据无关的全局项（FFmpeg
    可执行文件路径、峰值标记文本模板）放插件级配置（FFmpeg 路径在 `createSettingPage()` →
    `Dialogs/DASignalAnalysisSettingPage`，入口「设置 → 信号分析」；模板在「分析设置 → 峰值」的
    「标记显示」对话框）。按信号生效的**常用参数**（通道 / 预处理 / 窗函数 / 幅值口径 / 频率分辨率 /
    STFT 窗长与跳步 / 峰值模式、标记数量、相对门限）只在「分析设置」上下文页出现；「更多 → 高级设置…」
    对话框只保留零填充、功率谱、峰值高级三项、标定，不要再把常用项加回对话框（两处入口会分叉，
    且 `params()` 必须从打开时基线回填，否则点确定会重置丝带选择）。dB 参考不再有独立入口，由幅值
    口径派生（`dbRef1` → 相对 1.0，其余 → 满量程），字段只保留以兼容旧档，写盘/送 Python/读档统一经
    `DASignalAnalysisParams::normalizeDbReference()` 规范化。界面文案引用入口时统一写「设置 → 信号分析」。

11. **插件必须能热卸载干净。** 动作经宿主 `DAActionsInterface` 创建/销毁；`CDockWidget*` 与页面指针
    用 `QPointer`（宿主可能先销毁 dockManager）；对话框以页面为父；`finalize()` 清理峰值窗口、页面
    dock、播放器、worker、上下文标签与动作，并摘掉主页「创建」panel 里注入的按钮（只摘按钮、不删宿主
    panel）；且 `mUI`/`mContextCategory` 为空时**不能提前 return**（后续动作清理必须执行）。
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
    变速、深色主题、智能体工具、工作流节点都不做，只留结构口。矢量导出是既有例外：2D 子图
    PNG/SVG/PDF，3D 固定 PNG。**3D 矢量导出已在 2026-11 于目标机实测不可用**：`Qwt3DPlot::saveVector`
    能出文件，但 gl2ps 在当前 3D 渲染器（VBO+GLSL、CPU 矩阵、无固定管线矩阵栈）下 SVG 页面盒错位
    （900×700 内容写进 700×900 页面，右侧被裁）、**文本/色标图例全部丢失**，PDF 基本空白——不要
    据此扩 3D 导出过滤器，口径见 `tmp/signal-analysis-chart-ops/03-3d-items-and-vector-export.md` 附录。
    对数频率轴已**整体移除**（D2：动作/视图状态/存档键/图标全删，旧档 `logFrequency` 静默忽略），
    不要重新引入。

## 四、关键机制

### 4.1 页面惰性加载与丝带入口

- `initialize()` 只建主页「创建」panel 的「信号分析」入口（`installHomeEntry()`）、上下文标签两页、动作与
  后台 worker，**不建**中央页面和峰值窗口——多数用户用不到信号分析，程序启动不该多出一个空标签页。
- 插件**不再占用顶级丝带标签页**：入口是宿主主页「创建」panel 里注入的一个 Large 按钮（动作
  `actionSignalOpenPage`，objectName 沿用旧值），卸载时只摘按钮、不删宿主 panel。宿主 panel 按
  `DA::UiNames::Ribbon::MainCreatePanel` 查找，找不到只 `qWarning`，不影响工程恢复等其它建页路径。
- 页面统一走 `ensurePageCreated()`：建页面 + 连接导入/标签事件 + 建峰值窗口 + 播放器控制条 + 读色带偏好。
  三个导入槽、`addSignalAndActivate`、`restoreState`、主页「信号分析」入口都先调它。
- `openSignalAnalysisPage()`：首次调用创建页面；页面已关闭时恢复显示并 raise；打开后丝带默认停在
  「分析设置」页（上下文标签已显示时也会切回页 1）。
- `serializeState()` / `finalize()` / `retranslateUi()` / 各后台结果回填都必须容忍 `mPage == nullptr`
  （页面从未打开过的会话）。页面 dock 图标为 `icon/showSignalAnalysis.svg`。
- 页面 dock 激活时由 `DASignalAnalysisSettingContext` 接管设置面板、图元树、上下文标签与播放器控制条；
  失活时还原宿主绘图区目标（**2D 与 3D 两页 target 都复位**：2D 页指向宿主当前子图，
  3D 页 `setPlot3D(nullptr)`），并停止播放（D6：停止只挂在 provider 失活，不挂在
  `setSignalToolbarVisible(false)`——该函数在激活流程里也会被调用）。
- 设置面板目标页由纯函数 `Page/DASignalSettingTarget`（`daSignalSettingPage()`）判定：
  无视图 → None（两页 target 都置空）、3D 子图 → Chart3D、其余 → Chart2D。
- 图元树只在 figure 列表**真的变化**时 `setFigures`（列表同构判定 `daSameFigureList`），
  否则只 `setCurrentFigure`：`setFigures` 会清空重建 combo/树视图，每次点一下子图都重建
  会丢展开/选中状态。provider 失活时缓存作废（图元树已切回宿主绘图区）。

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
  `DASignalAnalysisWorker` 提交，进度写状态栏进度条；`cancelAll()` 与状态栏进度条右侧的「停止计算」
  按钮（`DAStatusBarInterface::setProgressCancelAction` 绑定 `actionCancelAnalysis`，动作不再放进
  丝带「分析」panel），enabled 绑 `runningTaskCount() > 0`；取消只走 `canceled(taskId)` 清进度，
  不弹失败。状态栏进度显示有 500 ms 防抖（`cProgressIndicatorDelayMs`）：短任务在窗口内结束就完全
  不显示进度条与停止按钮；多任务并发时 worker 上报平均进度 + 任务数，状态栏文本显示任务数。
- 失败经 `failed(error)`（英文细节）→ 界面 `reportOperationFailure()` 只显示翻译后的提示，并
  `clearProgressIfIdle()`；上下文过期丢弃结果的分支同样要清进度。
- 视图侧只发请求信号（`spectrumRequested`/`envelopeRequested`/`heatmapRequested`/`followFrameSpectrumRequested`），
  不自己算、也不缓存跨信号结果。

### 4.4 参数、幅值与内存预算

- 参数一个信号标签页记一份，不进全局撤销栈。常用项在「分析设置」上下文页（通道；预处理：去直流/
  去趋势；频谱：窗函数/幅值口径/频率分辨率；时频：窗长/跳步重叠；峰值：标记数量/模式/相对门限），
  高级项在「更多 → 高级设置…」对话框（零填充/功率谱/峰值高级三项/标定）。通道下拉在「频谱」参数
  网格首行（D8：通道是分析输入，显示页不放通道）。
- **统一写重回填入口**：控件 → `applyParamsPatch(sig, patchFn, ParamScope)`（拷贝参数 → 改字段 →
  `mParamsPerSignal` → `bumpParamsVersion()` → 按 scope 分级重算）；参数 → 控件 `syncParamsRibbonControls()`
  由 `syncSignalContextControls()` 调用，全部 `QSignalBlocker` 防回环，无当前信号时置灰。
- **分级重算**：`ParamScope::Preprocess`/`Spectrum` → 频谱+热图+峰值；`TimeFreq`（跳步）→ 热图+峰值曲线；
  `Peaks` → 只重算峰值表与曲线；`DisplayOnly`（幅值口径）→ 只重绘。不要所有改动都走
  `recomputeCurrentSignal()` 全量（大信号改跳步重算整段频谱会明显变慢）。
- `resolutionHz` 与 `stftWindowMs` 双向联动（Δf = 1/窗长），换算函数
  `signalStftWindowMsFromResolution` / `signalResolutionFromStftWindowMs`；改一个时用阻断信号回填另一个
  控件，`applyParamsPatch` 里同时写两个字段，scope 用 `Spectrum`（联动时含频谱）。峰值最小间距 `<=0`
  是自动哨兵，按 5×实际分辨率派生。
- 幅值口径（peak/rms/dbFS/dbRef1）只影响显示与导出；dB 参考由幅值口径派生（D7），改口径不递增参数
  版本、不提交计算，只 `syncAmplitudeUiFromParams()` 同步全局 `mAmplitudeMode/mDbReference`、视图与
  峰值表重绘。
- 对话框确定后必须 `syncParamsRibbonControls()` 全量回填丝带，否则峰值表、导出与存档口径会和视图分叉。
- 丝带参数行统一用 SARibbon 内置的 `SARibbonParamGrid`（以 Large 占比加入面板）：
  一组参数放进同一个网格，标签列与控件列共享列宽；网格会跟随面板行模式（3/2/1 行）自动横向分列，
  4 行参数在 3 行样式下摊成 2+2、在 1 行样式下摊成 4 列，不会被压扁或裁切。
  标签经 `leadingLabel(id)` 取回用于重译，换语言后必须让网格重算列宽（`retranslateUi()` 末尾统一
  对 6 个参数面板调 `recalcColumnWidths()`）；**不要**再用 `addSmallWidget` 逐行拼「标签 + 控件」，
  也不要在插件里自建 QGridLayout 网格——那两行写法都会让控件列参差不齐且没有行预算。
- spin 一律 `setKeyboardTracking(false)`；单位用 suffix（` Hz` / ` ms` / ` %`）。
- 预算：频谱/热图/包络流式或按区段，峰值表/曲线含整段 float32 副本并按并发系数放大；
  详见 `DASignalMemoryEstimate.h` 的公式注释。

### 4.5 视图与三图联动

- 布局：`DAFigureWidget` 2×2 保留式网格——波形 (0,0,1,2)、频谱 (1,0)、热图/三维瀑布 (1,1)；
  瀑布默认二维（`setWaterfallMode` 0=二维 / 1=三维 / 2=隐藏）；播放头、STFT 窗口矩形、峰值标记是
  画布图元。视图内不放 inline 控件：分析参数在「分析设置」上下文页、显示类在「信号显示」页、
  播放类在窗口底部播放器控制条、外观类在右侧设置面板。**视图的 setter/getter 是这些入口共同的
  状态源**：每个显示开关都必须成对（例：`setPeakMarkersVisible` / `peakMarkersVisible`），
  丝带勾选状态从视图回填，**不要**在 `DASignalAnalysisUI` 里另记一份全局值当真相
  （UI 层同名成员只作为「新建视图的默认值」）；峰值标记显隐随信号存档（§4.8）。
- **窗口底部播放器控制条**（`buildSignalToolbar`，只在信号分析页激活时显示）：跟随播放 / ▶⏸ 单切换 /
  ■ / 时间滑块 / 时间文本（`Page/DASignalTimeText` 纯函数，`mm:ss.s` 与「当前 / 总长」占位）/
  自动滚动。**只放播放相关控件**：dB 范围已归「信号显示 → 云图设置」（D5）。`actionPlay` 是可勾选动作
  （勾选 = 正在播放），`DASignalPlayer::stateChanged(bool)` 驱动图标与勾选；`actionPause` 已删除。
  失活停止播放挂在 provider 失活，不挂在 `setSignalToolbarVisible(false)`。
  **分工澄清**：插件**没有**丝带上的播放面板，播放类控件只在底部控制条（丝带「分析设置」页只放
  分析参数与峰值/导出入口）；控制条随页面激活显隐，不占常驻丝带空间。
- **切通道清屏**：`setCurrentChannel` 调 `clearRealtimePlots()` 一次性清掉上一个通道的频谱曲线、
  峰值标记、热图栅格与三维瀑布迹线（热图是**隐藏谱图图元**，只 replot 会让旧栅格留在屏上）。
  只在切通道时清一次，播放/重绘路径不动（避免闪烁）；`rebuildHeatmap` 拿到新栅格后恢复可见。
- **「信号显示」页按对象分组**（D1）：时频图设置（2D/3D/隐藏、当前帧标记、测量 ▾、视图 ▾）/
  云图设置（色带、色标、dB、显示 ▾）/ 瀑布图设置（瀑布设置…、色带、dB、当前迹线高亮）。
  该页单独开启面板标题（D9）；`SARibbonBar::setRibbonStyle()` 会按样式统一改写面板标题，
  必须监听 `ribbonStyleChanged` 重新下发（`applyDisplayPagePanelTitles()`），`finalize()` 断开。
  另外监听 `currentRibbonTabChanged(int)`，切回本插件两页时刷新一次上下文控件（宿主设置面板改色/
  改显示模式后需要刷新点）。
- **当前帧标记**（默认开）：二维云图上一条半透明橙红虚线 + STFT 窗宽带，三维瀑布里对应迹线整条高亮；
  都按像素列节流（同一屏幕列不重复 replot），轴范围/尺寸/通道变化后缓存作废。
- **联动缩放**（测量 ▾，默认关）：波形 x ↔ 云图 x、频谱 x ↔ 云图 y。zoomer/panner 是懒创建的，
  只能挂接已存在的工具（不替用户开工具），并在 `chartPropertiesChanged(Zoom/PanStateChanged)` 时重试；
  同步用 `setAxisScale` + `mSyncingAxes` 防重入，关闭只断开连接、不恢复 autoscale。
- **双光标测量**（测量 ▾，默认关）：在当前 2D 子图上点击依次放 A/B/再放 A；读数全走纯函数
  `Page/DADualCursorReadout`；开启时停用「点击波形即定位」（UI 的 seek 门控把双光标也算进去），
  切子图/切通道/改参数/关闭开关都清理游标。
- **峰值保持**（测量 ▾，默认关）：仅在跟随播放时可用；按频率网格逐点取最大值画灰虚线（transient、
  不参与导出），网格变化重建缓冲；关闭跟随时自动取消勾选并清空。
- **时间格式 / 布局比例 / 重置显示**（视图 ▾）：时间格式作用到波形 x 与云图 x（`Render/DATimeScaleDraw`，
  `mm:ss.s` 以信号时间原点为 0 点）；布局比例是「套用一次」的网格比例预设；重置显示把显示类状态
  一键恢复默认（不动分析参数）。
- **坐标轴**：`DAFigureWidget::createChart()` 会给新子图显式设 X 0..800 / Y 0..500 占位刻度（同时关掉
  autoscale）。`buildFigure()` 只在建图时对三张 2D 子图调用 `setAxisAutoScale(..., true)`；
  `rebuild*` 不无条件重开，否则用户缩放会被覆盖。工程恢复走存档轴状态。
- **色标**：`applyHeatmapColors()` 统一设 yRight 轴范围 `[mDbFloor, 0]`、色标宽 18、间距 4；隐藏时
  `setAxisVisible(yRight, false)` 不占横向空间。`setDbFloor`/`rebuildHeatmap`/`refreshAfterFigureRestore`
  都走这一入口，避免刻度与颜色对不上。显隐状态按信号存 `view{}.colorBarVisible`（同时保留插件级偏好
  `colorBarVisible` 作为新视图默认值）。
- **色带单一真相 = 谱图图元的 colorMap**（D10）：`applyHeatmapColors()` **不再**无条件重设 item 的
  色带（否则宿主热图面板改的起止色会被 dB 变化/重算冲掉）；丝带「色带」是「写预设 + 回读勾选」，
  回读走 `daMatchColormapPreset()`（不匹配任何预设时菜单不勾选任何项）。色标轴取 item 当前色带的拷贝，
  因此宿主面板改色后色标在下一次同步/重建时才更新（弱同步，已写进用户指南）。
- **三维瀑布**（D11/D12）：dB 化用整份栅格峰值归一 + 固定色域 `[floor, 0]` + 基线 = floor；**复用图元**
  （`setTraceCapacity`/`clear`/`pushTrace`/`setBinValues`）而不是 delete+重建，否则右侧 3D 面板会反复
  跳页、用户改的样式被重算覆盖；当前迹线高亮用 `Render/DAWaterfallColor`（`Qwt3DColorMapColor` 子类，
  零改动 qwt）。瀑布样式不在 `rebuild*` 里写死，由 3D 面板维护，保存时从图元读回。
- **跟随播放**：频谱必须画「当前帧真实幅值谱」，不能拿热图显示栅格的当前列——栅格在频率方向做过
  max 池化（Δf_row 远大于峰值网格），两条曲线口径不同，峰值标记会落在下降沿。当前实现走
  `stft.frame_spectrum_sync()` + 帧号换算，与 `peaks.py` 单帧同口径、只读一帧（O(nperseg)）。
  峰值保持也只在这一口径上累积。
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
  极大，邻域半径 2、峰值数量由标记数量（10/20/50）决定；峰频超出曲线范围时跳过，无快照时回退旧口径。
  吸附是显示层口径，不改峰值表数值，也不要去改 Python 侧峰值算法的频率网格。
- 标记形状与文本：符号是**倒三角**（`QwtSymbol::Path` + `pinPoint`，尖端严格落在峰值点；不要用内置
  `DTriangle`，它以符号中心为锚点、尖端会落到峰顶下方），统一一种颜色；文本由 `DAPeakMarkerLabel`
  按模板渲染（`{index}` / `{fre}` / `{amp}`，默认 `{index}-{fre} Hz` → `1-32.5 Hz`），幅值取标记自身
  的显示坐标（与 `DASignalAmplitude` 同口径，dB 模式自动带单位），空模板表示不显示文本。
  标记非 transient、随工程存档；恢复时位置以存档为准，文本按当前模板重写（`applyPeakMarkerLabels()`）。
- 丝带入口（**全项目唯一一处**，D13）：**分析设置 → 峰值**依次是 峰值标记（大按钮，勾选）/
  标记数量（大按钮 + 10/20/50 菜单）/ 模式与相对门限（同一个 `SARibbonParamGrid` 参数网格）/
  标记显示（半高按钮，弹出模板编辑对话框）/ 峰值窗口（大按钮，打开峰值表停靠窗口的唯一入口）。
  「信号显示」页**没有**峰值面板，不要再把入口拆到两页。模式/相对门限/标记数量改后只重算
  峰值表与曲线（`ParamScope::Peaks`），不重算频谱；模板改动立即写配置并广播到全部已创建视图。
- 存档优先：加载后到第一次重算之间，`mPeakMarkersRestored` 保证不删除重建用户调整过的标记；
  `setPeaks()` 收到新计算结果时复位该窗口。
- **标记重建时机**（性能 + 不闪烁）：只有「峰值数据」（`setPeaks`）、「显隐切换」、「模板/参数变化」
  三条路径调 `rebuildPeakMarkers()`；曲线重绘/播放路径（`rebuildSpectrumPlot`）只调
  `updatePeakMarkerPositions()` **就地改坐标**，不新建/不删除图元。两处都**复用现成图元**
  （数量一致时不 delete+new），连续回填时标记指针稳定。不要在重绘路径重新调 `rebuildPeakMarkers`。
- 窗口显隐偏好 `peakDockVisible`：`ensurePageCreated()` 读取、`finalize()` 写回；首次导入只按偏好决定
  是否显示峰值窗口，**不得**覆盖用户关闭过的选择。
- 导出：峰值表 / 频谱 / 功率谱密度 / 热图矩阵 CSV，整页或当前子图图片，峰值表还可送表格页；
  数值一律先过 `DASignalAmplitude`，PSD 是功率量（10·log10），不随幅值模式变化。
  当前子图的格式由「所选过滤器修正后的后缀」决定（`DAFigureWidget::exportFormatFromFilePath`），
  实际写入格式再过 `DASignalChartExport::effectiveFormat`：3D 子图只有 PNG 位图，请求 SVG/PDF
  一律降级为 PNG（UI 给用户可见提示），不要给 3D 放开矢量过滤器（见铁律 14）。

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
  `signal-analysis/chart-data`（图元二进制）。后两者只在**有 figure 内容**时写：`saveFigures` 在
  没有可写视图时返回**空字节**、`buildFiguresXml` 在页面无信号时提前返回，存档任务按 `!isEmpty`
  判据跳过——空工程不会留下只有根节点的 `figures.xml` 与空 `chart-items.xml`。旧工程可正常加载。
- 三维瀑布通过 `DAChart3DWidget::setItemTransient()` 标 transient：工程存档的 `chart3d-data/`
  （`DAChart3DItemArchive` 写 `<chart3d><items>` 索引）不会记录它；本插件的 figure XML 里
  `<chart3d><items>` 因此为空，恢复时也不消费（3D 图元由 `rebuildWaterfall` 重建）。
- `signals.json` 的 `view{}` 键（缺键取默认，旧档兼容）：`waterfallMode` / `colormap` / `dbFloor` /
  `colorBarVisible` / `autoScroll` / `peakMarkersVisible` / `timeCursor` / `linkedZoom` / `dualCursor` /
  `peakHold` / `timeFormat` / `waterfallColormap` / `waterfallDbFloor` / `waterfallHighlight` /
  `heatmapDisplayMode` / `heatmapResample` / `waterfall`（对象：style/colorMode/opacity/depthOrder/
  outlineColor/outlineWidth/solidColor，从图元读回）/ `gridRows` / `gridColumns`。
  二维与三维的 dB 色域相互独立（D12）。谱图/瀑布是 transient，恢复时先把显示设置记成「待应用值」，
  图元重建后再套用；`logFrequency` 已删除，读取时静默忽略。
  峰值保持只在跟随播放时可用（跟随开关不存档），恢复时一律关闭。
- 保存流程在主线程收集 figure XML 与图元快照，后台线程只写文件；恢复在主线程 `loadedCallBack` 里
  重建界面并应用 XML。figure 元素同时写 `signal-id`（精确匹配）与 `signal-name`（旧档回退）；
  元素分派走 `DASignalFigureArchive::assignFigureElements`（纯逻辑、可单测）：一个 figure 元素只能被
  一个视图消费、一个 chart-data key 只能 attach 到一个子图（重复命中跳过并 `qWarning`）。
- `view{}.peakMarkersVisible` **按信号**存档：序列化取该视图的 `peakMarkersVisible()`，读档按信号
  回填视图；UI 层的 `mPeakMarkersVisible` 只作为「新建视图的默认值」，不要再用一个全局值覆盖全部
  信号标签页。
- 恢复子图的匹配规则：`role` → `grid-row/grid-col`。**两者都认不出来时**：整档都没有 `role`
  （旧存档）才按 XML 顺序回退到 wave/spectrum/heatmap；新存档直接跳过该 `<chart>` 并 `qWarning`，
  绝不把认不出的子图套到下一个空子图（否则串图比丢样式更难排查）。
- 图元管理者**不持有所有权**（`DAChartItemsManager` 析构为空）：加载残档后未被任何子图消费的图元
  必须由 `DASignalFigureArchive::releaseUnattachedItems()` 统一释放（`applyFiguresXml` 收尾调用，
  判据是 `plot() == nullptr`），并给用户可见提示「部分图元未能恢复，已丢弃」。已 attach 的图元归
  子图所有，不要在那里删。
- 宿主 `DAProjectInterface::projectIsCleaned`（新建工程 / 加载前 / 快照回滚）必须触发幂等清理：停
  worker/播放器、清信号与峰值缓存、清临时文件，否则「工程包里没有 signals.json」会把上一工程的
  信号带进新工程。
- 缺失来源：文件不存在时弹「重新指定」，恢复后保留采样率与参数；数据帧缺失保留「来源缺失」占位
  信号（`isMissing()`，不参与计算），丝带「重新指定数据帧来源」重选后 `replaceSignalAt()` 原地替换
  并沿用参数与视图状态。

### 4.9 设置

- 两个入口、两份内容：插件全局设置页（`createSettingPage()`，主页 → 设置 → 信号分析）只放与数据
  无关的全局项（FFmpeg 路径）；「更多 → 高级设置…」（`DASignalAnalysisSettingDialog`）只放按信号生效
  的高级参数（零填充/功率谱/峰值高级三项/标定），常用参数在上下文页。
- 对话框 `params()` 必须从打开时基线参数回填丝带字段（removeDc / detrend / window / amplitudeMode /
  resolutionHz / stftWindowMs / stftOverlap / peakMode / peakTopN / peakRelDb），否则点确定会把这些
  选择重置成默认值；返回前再 `normalizeDbReference()`（D7）。
- 宿主 `AppMainWindow::showSettingDialog()` **每次打开都重建设置对话框**，目的是让插件设置页不会活过
  提供它的插件（插件可被停用并卸载 DLL，常驻页面析构会跳进已卸载代码段）。因此插件**不得**把设置页
  指针缓存为长期成员，`finalize()` 也不需要清理设置页。
- 全局设置 `apply()` 后调 `DASignalAnalysisUI::onGlobalSettingsApplied()` 刷新 FFmpeg 可用性；路径无效
  时不落盘、表单回退到已保存值。所有配置共用 `signal-analysis-config.json`
  （`ffmpegPath`、`colormap`、`peakDockVisible`、`colorBarVisible`、`peakMarkerText`），
  由 `DASignalImporter` 读写。峰值标记文本模板在「标记显示」对话框确定时立即「读-改-写」落盘。

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
| 宿主面板改了起止色，一次 dB 拖动就被冲回预设 | `applyHeatmapColors()` 无条件重设 item 的 colorMap | 色带单一真相 = item 的 colorMap，只在「还没有色带」时用预设初始化（§4.5） |
| 改参数/切色带后右侧 3D 面板又跳回 Waterfall 页 | 每次 rebuild 都 delete+重建瀑布图元 | 复用图元（`setTraceCapacity`/`clear`/`pushTrace`），只有外部销毁后才重建（§4.5） |
| 面板标题在切换 Ribbon 样式后消失 | `setRibbonStyle()` 会统一改写面板标题 | 监听 `ribbonStyleChanged` 重新下发（`applyDisplayPagePanelTitles()`） |
| 开启双光标后点击波形不再定位 | 双光标接管左键释放 | 预期行为：seek 门控把双光标算作互斥工具（§4.5） |
| 联动缩放关掉后两图轴范围仍相同 | 关闭只断开连接，不恢复 autoscale | 预期行为（写进用户指南），需要重置范围用「重置显示」 |

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
- **界面层是一个类、多个 translation unit**：`DASignalAnalysisUI` 的实现按域拆成 `DASignalAnalysisUI*.cpp`
  （仿 `src/APP/Ribbon/DAAppRibbonArea*`）。新增成员函数按域放进对应文件；跨文件共享的常量与无状态
  helper 放 `DASignalAnalysisUIHelpers.h`，**不要**在某个 `.cpp` 的匿名 namespace 里定义后又被别的
  `.cpp` 使用（各 TU 会各有一份副本）。拆分只是文件级组织，类定义（`DASignalAnalysisUI.h`）与
  信号槽/ABI 不变。
- 新增分析任务时必须同时补齐：`dedupeKey`、内存估算、取消路径、上下文版本校验、Python 侧流式
  provider 实现与 pytest 用例。
- 新增纯逻辑（不依赖 pybind11/GUI）优先做成可单测的纯函数（参考 `DASignalMemoryEstimate.h`、
  `DAPeakCurvePlotSamples.*`），并加入 `DASignalAnalysisTest` 的显式 SOURCES。