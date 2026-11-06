# DASignalAnalysis 插件开发指南

通用时序信号分析插件。主工作区新增「信号分析」页面（与绘图/表格/工作流并列），
页面内每个信号一个标签页；丝带新增「信号分析」标签页。开发方案见
`tmp/信号分析插件开发方案.md`（决策记录 D1–D31 为定稿口径，不得擅自更改）。

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
├── Dialogs/                      # 导入对话框 + 参数设置对话框
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
    对话框以插件页面为父（`mPage`），避免卸载后残留。
11. **信号来源干净**：文件来源读块、数据帧来源引用式（按块转换）；临时 WAV 随信号/控制器生命周期清理。
12. **工程只存来源引用、参数与视图状态**（`Archive/DASignalArchiveTask`），不存分析结果/STFT 矩阵/峰值数据；
    任务在后台线程只做文件读写，界面重建走 `loadedCallBack`。

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
  STFT 窗长由 `stftWindowMs` 决定（评审 C3/C4）。
- **卸载清理**：`finalize()` 必须清理峰值停靠窗口、页面停靠窗口、播放器、worker、丝带标签页与动作；
  `mUI`/`mCategory` 为空时不能提前 return（评审 B5）。
- **测试选择**：`plugins/DASignalAnalysis/` 的改动在 `scripts/test.ps1 -Changed` 下会选中 `data` 套件。
