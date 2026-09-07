# 模块依赖关系说明

data-workbench采用分层模块化架构，各模块之间有明确的依赖关系。本文档详细说明模块的层次结构、职责边界和依赖配置。

---

## 主要模块概览

| 模块 | 层 | 类型 | 简述 | 源文件数 |
|------|---|------|------|:------:|
| **DAShared** | L1 基础层 | 纯头文件 | 基础数据结构、枚举↔字符串映射、Qt5/Qt6兼容、并发容器 | 19 |
| **DAUtils** | L1 基础层 | 共享库 | XML序列化、字符串转换、CSV读写、目录管理、颜色主题、翻译管理 | 48 |
| **DAMessageHandler** | L1 基础层 | 共享库 | spdlog初始化、Qt消息路由、线程安全日志队列 | 12 |
| **DAPyBindQt** | L1 基础层 | 共享库 | pybind11类型转换器、Python解释器生命周期、numpy/pandas绑定 | 30 |
| **DAPyScripts** | L2 功能层 | 共享库 | Python脚本I/O、DataFrame操作、信号处理函数C++包装 | 13 |
| **DAPyCommonWidgets** | L2 功能层 | 共享库 | DataFrame列选择器、dtype下拉框 | 5 |
| **DAPyWorkFlow** | L2 功能层 | 共享库 | Python工作流节点代理、工厂、场景、执行引擎、图形项 | 52 |
| **DAData** | L2 功能层 | 共享库 | 抽象数据基类、DAData包装器、数据管理器、Python数据封装、撤销命令 | 21 |
| **DAGraphicsView** | L2 功能层 | 共享库 | QGraphicsView框架（场景、视图、图元、连线、动作、撤销命令） | 51 |
| **DAFigure** | L2 功能层 | 共享库 | Qwt图表容器、图表编辑器、数据探针、序列化 | 85 |
| **DAGui** | L3 界面层 | 共享库 | 工作流UI、图表设置面板、数据管理UI、Model/View、对话框 | 405 |
| **DAInterface** | L4 接口层 | 共享库 | 抽象接口定义（Core/UI/Docking/Ribbon/Actions/Command/DataManager/Project） | 29 |
| **DAPluginSupport** | L4 接口层 | 共享库 | 插件框架（DAAbstractPlugin/DAPluginManager/DAAbstractNodePlugin） | 9 |
| **DAAgent** | L4 接口层 | 共享库 | 纯 agent 框架库（LLM 聊天/工具注册/信号链/会话持久化），不依赖 GUI 模块 | 17 |
| **APP** | L5 应用层 | 可执行程序 | 主程序、接口具体实现、项目文件管理、插件管理 | 92 |
| **DAAxOfficeWrapper** | L1 基础层 | 共享库 | Windows Office自动化封装（仅Win） | 9 |

!!! note "文件计数口径"
    上表“源文件数”统计各模块目录下的 `.h` / `.hpp` / `.cpp` / `.ui` 文件数（不含资源、构建产物与子模块），由 `find src/<模块> -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.ui' \) | wc -l` 得到。历史 `DACommonWidgets` 模块已并入 `DAGui`（其控件现位于 `DAGui/` 根级），不再作为独立模块列出。

---

## 五层架构图

```
┌──────────────────────────────────────────────────────────┐
│ Layer 5: 应用层          │ APP                           │
├──────────────────────────────────────────────────────────┤
│ Layer 4: 接口层          │ DAInterface, DAPluginSupport, │
│                          │ DAAgent                       │
├──────────────────────────────────────────────────────────┤
│ Layer 3: 界面层          │ DAGui                         │
├──────────────────────────────────────────────────────────┤
│ Layer 2: 功能层          │ DAData, DAFigure, DAPyWorkFlow│
│                          │ DAGraphicsView, DAPyScripts,  │
│                          │ DAPyCommonWidgets             │
├──────────────────────────────────────────────────────────┤
│ Layer 1: 基础层          │ DAShared, DAUtils,            │
│                          │ DAMessageHandler, DAPyBindQt  │
└──────────────────────────────────────────────────────────┘
```

---

## 模块构建依赖顺序

```
基础层: DAShared → DAUtils
       ├──→ DAAxOfficeWrapper (仅 Win)
       └──→ DAMessageHandler
Python层: DAPyBindQt → DAPyScripts → DAPyCommonWidgets → DAPyWorkFlow
功能层: DAData (→DAUtils, DAPyBindQt, DAPyScripts)
       DAGraphicsView (→DAUtils)
       DAFigure (→DAUtils + Qwt)
界面层: DAGui (→ 上述所有模块 + SARibbon/ADS/qwt/DALiteCtk/quazip；含已并入的 DACommonWidgets 控件)
接口层: DAInterface (→DAGui)
       DAPluginSupport (→DAInterface + DAPyWorkFlow)
       DAAgent (→DAInterface/DAData/DAPyBindQt/DAPyScripts；纯 agent 框架库，不依赖 GUI)
应用层: APP (→DAPluginSupport + DAPyWorkFlow)
```

---

## 模块依赖矩阵

下表列出每个模块的**直接**依赖（CMake `target_link_libraries`）：

| 模块 | PUBLIC 直接依赖 | PRIVATE 直接依赖 | 外部关键依赖 |
|------|----------------|-----------------|-------------|
| **DAShared** | — | — | Qt::Core |
| **DAUtils** | Qt::Core/Gui/Widgets/Xml | — | — |
| **DAAxOfficeWrapper** | DAUtils, Qt::Core/Gui/Widgets/AxContainer | — | Windows only |
| **DAMessageHandler** | Qt::Core/Gui/Widgets/Xml | DAUtils | spdlog |
| **DAPyBindQt** | Qt::Core, pybind11::headers | DAUtils, Python3 | numpy, pandas |
| **DAPyScripts** | Qt::Core, DAPyBindQt, pybind11 | — | Python3 |
| **DAPyCommonWidgets** | Qt::Core/Gui/Widgets, DAPyBindQt, pybind11 | — | Python3 |
| **DAPyWorkFlow** | DAUtils, DAGraphicsView, DAPyBindQt | Qt::Core/Gui/Widgets | Python3, pybind11 |
| **DAData** | Qt::Core/Gui/Widgets, DAPyBindQt, DAPyScripts | DAUtils | Python3, pybind11 |
| **DAGraphicsView** | Qt::Core/Gui/Widgets/Xml/Svg | DAUtils | — |
| **DAFigure** | Qt::Core/Gui/Widgets/PrintSupport/Concurrent/OpenGL, Qwt, DAWidgets | DAUtils | — |
| **DAGui** | DAUtils, DAMessageHandler, DAData, DAPyWorkFlow, DAFigure, DAPyBindQt, DAPyScripts, DAPyCommonWidgets, Qt, SARibbon, QtAdvancedDocking, qwt, DALiteCtk, quazip | Qt6::Core5Compat (if Qt6) | Python3, pybind11 |
| **DAInterface** | **DAGui** (PUBLIC → 传递至所有消费者) | Qt, SARibbon, QtAdvancedDocking, qwt, DALiteCtk | Python3, pybind11 |
| **DAPluginSupport** | **DAInterface** (PUBLIC), **DAPyWorkFlow** (PUBLIC), Qt | QtAdvancedDocking | Python3, pybind11 |
| **DAAgent** | **DAInterface**, **DAData**, **DAPyBindQt**, **DAPyScripts**, Qt::Core/Gui/Widgets | Qt::PrintSupport/Svg, DAAxOfficeWrapper (Win), Crypt32 (Win, DPAPI 加密 api_key) | Python3, pybind11 |
| **APP** | **DAPluginSupport**, DAPyWorkFlow, Qt, DALiteCtk, SARibbon, QtAdvancedDocking, qwt | — | Dbghelp (Win) |

---

## 依赖方向规则（铁律）

### 规则

1. **上层可以依赖下层，下层绝不能依赖上层。**
   - ✅ DAGui → DAUtils（界面层依赖基础层）
   - ❌ DAUtils → DAGui（基础层绝不能依赖界面层）
2. **同层模块尽量减少直接依赖**，通过上层整合模块（DAGui）协调。
3. **Python 相关模块**（DAPyBindQt/DAPyScripts/DAPyCommonWidgets/DAPyWorkFlow）为强制依赖，始终参与编译。
4. **DAShared 是纯头文件库**，所有模块可通过 include path 直接使用其头文件，无需显式 CMake 链接。

### 模块依赖关系图

```mermaid
graph BT
    subgraph "Layer 1 - 基础层"
        DAShared["DA<b>Shared</b><br/>基础数据结构<br/>纯头文件"]
        DAUtils["DA<b>Utils</b><br/>通用工具类"]
        DAMsg["DA<b>Message</b><br/>Handler<br/>日志基础设施"]
        DAPyBind["DA<b>PyBind</b>Qt<br/>Python↔Qt胶水层"]
    end

    subgraph "Layer 2 - 功能层"
        DAPyScripts["DA<b>PyScripts</b><br/>Python脚本包装"]
        DAPyCW["DA<b>PyCommon</b><br/>Widgets<br/>Python控件"]
        DAPyWF["DA<b>PyWorkFlow</b><br/>Python工作流"]
        DAData["DA<b>Data</b><br/>数据管理"]
        DAGV["DA<b>Graphics</b><br/>View<br/>图形视图框架"]
        DAFigure["DA<b>Figure</b><br/>图表容器"]
        DAOffice["DA<b>AxOffice</b><br/>Wrapper<br/>Win Office"]
    end

    subgraph "Layer 3 - 界面层"
        DAGui["DA<b>Gui</b><br/>GUI整合层<br/>405文件"]
    end

    subgraph "Layer 4 - 接口层"
        DAIF["DA<b>Interface</b><br/>抽象接口定义"]
        DAPS["DA<b>Plugin</b><br/>Support<br/>插件框架"]
        DAAgent["DA<b>Agent</b><br/>纯 agent 框架库"]
    end

    subgraph "Layer 5 - 应用层"
        APP["APP<br/>可执行程序"]
    end

    %% Layer 1 internal
    DAUtils --> DAShared
    DAMsg --> DAUtils
    DAPyBind --> DAUtils
    DAOffice --> DAUtils

    %% Layer 2 → Layer 1
    DAPyScripts --> DAPyBind
    DAPyCW --> DAPyBind
    DAPyWF --> DAUtils
    DAPyWF --> DAGV
    DAPyWF --> DAPyBind
    DAData --> DAUtils
    DAData --> DAPyBind
    DAData --> DAPyScripts
    DAGV --> DAUtils
    DAFigure --> DAUtils

    %% Layer 3 → Layer 2
    DAGui --> DAUtils
    DAGui --> DAMsg
    DAGui --> DAData
    DAGui --> DAPyWF
    DAGui --> DAFigure
    DAGui --> DAPyBind
    DAGui --> DAPyScripts
    DAGui --> DAPyCW
    DAGui --> DAGV

    %% Layer 4 → Layer 3
    DAIF --> DAGui
    DAPS --> DAIF
    DAPS --> DAPyWF

    %% DAAgent → L1/L2/L4（纯 agent 框架库，不依赖 GUI 模块）
    DAAgent --> DAIF
    DAAgent --> DAData
    DAAgent --> DAPyBind
    DAAgent --> DAPyScripts

    %% Layer 5 → Layer 4
    APP --> DAPS
    APP --> DAPyWF

    %% Styling
    style DAShared fill:#e8f5e9
    style DAUtils fill:#e8f5e9
    style DAMsg fill:#e8f5e9
    style DAPyBind fill:#e8f5e9
    style DAOffice fill:#e8f5e9
    style DAPyScripts fill:#e3f2fd
    style DAPyCW fill:#e3f2fd
    style DAPyWF fill:#ffebee
    style DAData fill:#e3f2fd
    style DAGV fill:#e3f2fd
    style DAFigure fill:#e3f2fd
    style DAGui fill:#fff3e0
    style DAIF fill:#f3e5f5
    style DAPS fill:#f3e5f5
    style DAAgent fill:#f3e5f5
    style APP fill:#fff9c4
```

---

## 各模块职责详解

### 基础层 (L1)

#### DAShared — 纯头文件模板库

职责：提供全项目通用的基础数据结构和工具模板，**纯头文件，无 .cpp 编译单元**。

提供内容：
- 数据结构：`DAVector`（命名向量）、`DAAutoincrementSeries`（等差数列）、6种二维表格（固定/稀疏/稠密/列式/行式）、并发容器
- 工具宏/模板：`DAEnumStringUtils`（枚举↔字符串映射）、`da_qt5qt6_compat`（Qt5/Qt6兼容层）、`da_qstring_cast`（类型转换）
- 其他：`DASignalBlockers`（批量信号阻塞）、`DAQtContainerUtil`（QSet↔QList转换）、`DAGenericIndexedContainer`（环形索引容器）

外部依赖：仅 `Qt::Core`

#### DAUtils — 通用工具模块

职责：为所有上层模块提供通用工具类。

提供内容：
- XML序列化：`DAXMLFileInterface`（序列化基类）、`DAXMLConfig`、`DAXMLProtocol`
- 字符串/类型：`DAStringUtil`、`DAQtEnumTypeStringUtils`（Qt枚举↔字符串映射）
- 文件/IO：`DACsvStream`、`DATextReadWriter`、`DADir`（应用目录管理）
- 其他：`DAColorTheme`（30+调色板）、`DATranslatorManeger`（国际化）、`DAProperties`（键值属性）、`DATree/DATreeItem`、`DAUniqueIDGenerater`、`DAProcess`

外部依赖：Qt::Core/Gui/Widgets/Xml

#### DAMessageHandler — 日志基础设施

职责：基于 spdlog 的应用级日志系统，Qt 消息路由。

提供内容：
- `daRegisterRotatingMessageHandler()` / `daRegisterConsolMessageHandler()` — 安装 Qt→spdlog 消息处理
- `DAMessageQueueProxy` — 线程安全消息队列（QObject，有信号）
- `DAMessageLogItem` — 单条日志消息数据结构

外部依赖：Qt::Core/Gui/Widgets/Xml, DAUtils (PRIVATE), spdlog

消费者：DAGui（`DAMessageLogsModel` 用于日志面板）、APP（main.cpp 注册日志处理器）

#### DAPyBindQt — Python↔Qt 胶水层

职责：pybind11 类型转换器、Python 解释器管理、numpy/pandas 绑定。**所有 Python 相关代码的基础**。

提供内容：
- 类型转换器（`DAPybind11QtCaster.hpp`）：QString↔str、QDateTime↔datetime、QVariant↔Any、QList/Vector/Set/Map/Hash↔list/dict/set 双向转换
- Python 环境：`DAPyInterpreter`（解释器生命周期）
- py::object 包装：`DAPyObjectWrapper`、`DAPyModule`（模块管理）
- JSON 转换：`DAPyJsonCast`（QJsonObject↔py::dict 双向）
- 线程安全：`DAPythonSignalHandler`（Python线程→Qt主线程）
- NumPy：`DAPyModuleNumpy`、`DAPyDType`（dtype 包装+类型检查）
- Pandas：`DAPyModulePandas`、`DAPyDataFrame`、`DAPySeries`、`DAPyIndex`

外部依赖：Qt::Core, DAUtils (PRIVATE), Python3, pybind11::headers (PUBLIC)

---

### 功能层 (L2)

#### DAPyScripts — Python 脚本包装

职责：将 `DAWorkbench` Python 模块的 I/O/DataFrame/信号处理 函数暴露为 C++ API。

提供内容：
- `DAPyScripts`（门面单例）→ `DAPyWorkBench`（包装 Python `DAWorkbench` 模块）
  - `DAPyScriptsIO` — 文件读写（CSV/TXT/PKL）
  - `DAPyScriptsDataFrame` — DataFrame 操作（插入/删除/排序/查询/透视表/异常值检测，35+方法）
  - `DAPyScriptsDataProcess` — 信号处理（频谱/滤波/峰值/STFT/小波）

外部依赖：DAPyBindQt (PUBLIC), Python3

消费者：DAData, DAGui, APP, plugins

#### DAPyCommonWidgets — Python 通用控件

职责：与 pandas DataFrame/Series 相关的基础 UI 控件。

提供内容：
- `DAPyDataframeColumnsListWidget` — DataFrame 列名列表（带 dtype 图标）
- `DAPyDTypeComboBox` — numpy/pandas dtype 选择下拉框（含 nullable 扩展类型）

外部依赖：DAPyBindQt (PUBLIC), Python3, pybind11

消费者：DAGui (PUBLIC link), APP (传递依赖)

#### DAPyWorkFlow — Python 工作流节点 ⚠️

职责：Python 工作流核心 — 节点代理、工厂、场景、执行引擎、图形项、序列化。

**⚠️ 注意：此模块由 AI 编写，存在类放置错误（见下方§反模式警告）。**

提供内容：
- 节点核心：`DAPyNode`（C++↔Python节点代理）、`DAPyNodeFactory`（Python节点发现）、`DAPyNodeParameter`（参数代理）、`DAPyNodeConnection`（连接代理）
- 工作流容器：`DAPyWorkFlow`（DAG 操作代理）
- 可视化：`DAPyNodeGraphicsItem`、`DAPyLinkGraphicsItem`、`DAPyLinkPoint`
- 场景/执行：`DAPyWorkFlowScene`、`DAPyWorkFlowExecutor`（拓扑排序执行）、`DAPyWorkFlowManager`（中央调度器）
- 序列化：`DAPyWorkFlowSceneSerializer`（场景布局）、`DAPyWorkFlowSerializer`（工作流逻辑）、撤销命令工厂
- 样式系统：`DAPyNodeStyle`、`DAPyLinkPointStyle`、`DAPyNodePalette`
- 信号管理：`DAPySignalManager`（信号驱动执行）

外部依赖：DAUtils, DAGraphicsView, DAPyBindQt (all PUBLIC), Python3, pybind11

消费者：DAGui (PUBLIC link), DAPluginSupport (PUBLIC link), APP (传递)

#### DAData — 数据管理

职责：数据抽象层，管理数据对象和操作。

提供内容：
- 抽象数据基类：`DAAbstractData`（6种 DataType 枚举 + `tableSource/isReferenceData/supportsUndoSnapshot/typeIdentifier` 能力位虚函数）
- 表格数据源抽象：`DATableDataSource`（schema+块级取数 mixin 接口，头文件不依赖 Python）、`DATableDataBlock`（块数据值类型）、`DADataFactory`（类型标识→创建函数注册表，引用式数据工程重建），详见[表格数据源抽象层](table-data-source.md)
- 数据包装器：`DAData`（轻量值类型，引用语义），`DADataPyObject/DataFrame/Series`（Python数据封装，DataFrame/Series 同时实现 `DATableDataSource`）
- 数据管理器：`DADataManager`（QObject，含 QUndoStack）
- 撤销命令：`DACommandDataManagerAdd/Remove/Rename`、`DADataObjectSwapUndoCommand/PersistUndoCommand`

外部依赖：DAUtils (PRIVATE), DAPyBindQt + DAPyScripts (PUBLIC, Python时)

消费者：DAGui (PUBLIC link → 40+文件)，DAInterface (传递)，APP (传递)

#### DAGraphicsView — 图形视图框架

职责：通用 QGraphicsView/QGraphicsScene 框架，提供基类但不含业务图元。

提供内容：
- 核心：`DAGraphicsView`（含缩放/平移/标记）、`DAGraphicsScene`（含网格/链接模式/撤销栈）
- 图元：`DAGraphicsItem`（基类）→ `DAGraphicsResizeableItem`（可缩放）→ `DAGraphicsRectItem/TextItem/PixmapItem`
  - `DAGraphicsLinkItem`（连线图元，贝塞尔/直线/折线）
  - `DAGraphicsItemGroup`、`DAGraphicsStandardTextItem`、`DAGraphicsMarkItem` 等
- 动作：`DAAbstractGraphicsSceneAction`（场景动作）、`DAAbstractGraphicsViewAction`
- 撤销命令：14个 QUndoCommand 子类（增删移动缩放旋转分组等）
- 工厂：`DAGraphicsItemFactory`（反射工厂）

外部依赖：DAUtils (PRIVATE), Qt::Core/Gui/Widgets/Xml/Svg

消费者：DAPyWorkFlow (PUBLIC link → 通过继承实现业务图元)，DAGui (传递)

#### DAFigure — 图表容器

职责：基于 Qwt 的纯 C++ 图表系统（无 Python 依赖）。

提供内容：
- 容器：`DAFigureWidget`（QScrollArea，QwtFigure布局）、`DAChartWidget`（QwtPlot + 3个接口Mixin）
- 编辑器：`DAAbstractChartEditor`→`DAAbstractTwoPointEditor`（→`DAChartArrowEditor`）、`DAAbstractRegionSelectEditor`（→三个区域选择子类）
- 探针：`DADataProbeMarker`（数据点探针）
- 序列化：`DAChartSerialize`（15+ QwtPlotItem 类型二进制序列化）
- 树视图：`DAFigureTreeView` + Models（图/子图/图元/轴 层级导航）
- 撤销命令：`DAFigureWidgetCommands`（创建/删除/调整大小/附加图元）

外部依赖：DAUtils (PRIVATE), Qwt (PUBLIC), DAWidgets (PRIVATE, DAFontEditPannelWidget/DAColorPickerButton 等基础 UI 控件，用于文本标注富文本编辑弹窗), Qt OpenGL/PrintSupport/Concurrent

消费者：DAGui (PUBLIC link → 图表设置面板)，APP (传递 → 主窗口集成)

---

### 界面层 (L3)

!!! info "DACommonWidgets 已并入 DAGui"
    历史 `DACommonWidgets` 通用控件模块已整体并入 `DAGui/`（控件现位于 `DAGui/` 根级，如 `DAPropertyPanelWidget`、`DASettingDialog`、`DAAbstractSettingPage`、`DAFormEditorRegistry` 等；见 `src/DAGui/CMakeLists.txt`“迁移自 DACommonWidgets”注释）。`src/` 下不再有独立的 `DACommonWidgets/` 目录，也不再作为独立 CMake target 存在。原模块承载的控件清单见下方 DAGui 根级说明：

    - 属性面板：`DAPropertyPanelWidget/ContainerWidget/ItemWidget`、`DACollapsiblePanel/GroupBox`
    - 样式编辑器：`DAColorPickerButton`、`DABrushEditWidget/StyleComboBox`、`DAPenEditWidget/StyleComboBox`、`DAFontEditPannelWidget`、`DAShapeEditPannelWidget`
    - 对齐/位置：`DAAligmentEditWidget`、`DAAligmentPositionEditWidget`（9宫格定位）
    - 文件/路径：`DAFilePathEditWidget`、`DAPathLineEdit`
    - 设置对话框：`DASettingDialog/Widget`、`DAAbstractSettingPage`、`DAPropertyFormDialog`/`DAPropertyFormWidget`（统一表单）
    - 工具：`DAWaitCursorScoped`、`DACursorScoped`、`DAScrollArea`

#### DAGui — GUI 整合层（最大模块，405 文件）

职责：整合所有模块的 GUI 组件，提供完整用户界面。

子目录（源文件数 .h/.hpp/.cpp/.ui）：
- `Chart/` (12) — 图表管理容器：`DAChartOperateWidget`(图表操作总控)/`DAChartManageWidget`(对象树)/`DAChartSettingWidget`(设置宿主) + `DAChartItemsManager`(序列化 key↔item 映射)；**非 Python 门控**
- `ChartAddItem/` (89) — 图表添加向导面板体系：2D/3D/Stats 三组抽象基类 + 27 个 `DAChartAdd*` 派生 Widget + `DAChartSeriesPickerWidget`/`DAChartSeriesSelectWidget`。**不可整目录 GLOB**，应显式列出源文件，否则会改变构建行为
- `ChartSetting/` (64) — 每种图表类型的属性设置面板 + 工厂
- `Chart3DSetting/` (26) — 3D 图表属性设置面板
- `NodeSetting/` (8，Python only) — 通用节点参数面板系统（参数类型注册+SceneB模式）
- `Commands/` (10) — 工作流/DataFrame 撤销命令
- `Dialog/` (31) — 对话框（图表向导、导入、列类型转换、Python参数等）
- `MimeData/` (3) — 拖放 MIME 数据类型
- `Models/` (22) — Qt Model/View 模型（数据树、表格、消息日志、Python数据）
- `Agent/` (4，Python only) — Agent 聊天 UI（`DAAgentDockWidget`）
- `MarkdownView/` (2，Python only) — Markdown 预览视图

根级 (134)：
- 工作流 UI：`DAPyWorkFlowEditWidget/GraphicsView/Scene` 等
- 数据 UI：`DADataManageWidget/OperateWidget`、`DAPyDataFrameTableView` 等
- 通用基类：`DAAbstractOperateWidget`（被 Chart/Data/PyWorkFlow 三家继承，留顶层）
- 工具：`DASplashScreen`、`DAZipArchive`、`DARecentFilesManager`

外部依赖：**所有下层模块** + SARibbon/ADS/qwt/DALiteCtk/quazip

消费者：DAInterface (PUBLIC link → 传递至所有消费者)

---

### 接口层 (L4)

#### DAInterface — 抽象接口定义

职责：定义抽象接口契约，解耦插件和 APP 具体实现。

提供内容：
- `DACoreInterface` — 核心单例入口
- `DAUIInterface` — UI 聚合接口
  - `DAActionsInterface` — QAction 注册/查找
  - `DACommandInterface` — QUndoGroup 管理
  - `DADockingAreaInterface` — 停靠区域管理（8个dock区域）
  - `DARibbonAreaInterface` — Ribbon 栏管理
  - `DAStatusBarInterface` — 状态栏管理
  - `DADataManagerInterface` — 数据管理器包装
- `DAProjectInterface` — 项目文件读写
- `DAInterfaceHelper` — 所有接口的一站式便捷访问

外部依赖：**DAGui** (PUBLIC → 传递所有 DAGui 依赖给消费者)

消费者：DAPluginSupport (PUBLIC link), APP (传递)

#### DAPluginSupport — 插件框架

职责：插件加载/卸载管理和插件基类定义。

提供内容：
- `DAAbstractPlugin` — 插件基类（纯虚接口）
- `DAAbstractNodePlugin` — 工作流节点插件基类
- `DAPluginManager` — 插件发现/加载/卸载管理器
- `DAPluginOption` — 插件元数据

外部依赖：DAInterface + DAPyWorkFlow (both PUBLIC)

消费者：APP (PUBLIC link), plugins/ 目录下所有插件

#### DAAgent — AI Agent 框架库

职责：纯 agent 框架库，提供工具注册机制、agent 生命周期信号、LLM 配置接口、QProcess 子进程通信管理。**不依赖任何 GUI 模块**（无 DAGui/DAFigure/qwt/ADS）——工具注册、聊天 UI 接线、LLM 设置页持久化全部由插件 / APP 层决定，DAGui 与 DAAgent 为兄弟模块互不依赖。

提供内容：
- `DAAgentInterface` — 公共接口：14 个 agent 生命周期/会话信号 + `registerTool`/`registerSystemPrompt` + `sendUserAnswer`/`newSession` + `get/setLLMConfig` + 会话管理纯虚方法
- `DAAgentModule` — 接口实现：工具注册表、系统提示词组装、懒启动、Bridge 信号转发（**不持有 Dock**）、`DAAgentConfig`（agent-config.json）持久化（api_key 在序列化边界 DPAPI 加解密）
- `DAAgentBridge` — QProcess 子进程管理 + JSON Lines 协议解析
- `DAAgentSessionStore` — 会话持久化层（JSONL 读写/索引/清理）
- `DAAgentToolBase` — 瘦工具基类（`DAAgent_API` 导出，供插件跨 DLL 继承）

外部依赖：DAInterface, DAData, DAPyBindQt, DAPyScripts (PUBLIC) + Crypt32 (Win, PRIVATE, DPAPI 加密 api_key) + DAAxOfficeWrapper (Win, PRIVATE)

消费者：APP（`DAAppController` connect Dock↔接口信号链，决策 D3b），`plugins/DAAgentTools/`（注册 16 个内置工具）

> 工具注册、聊天 UI、LLM 设置页均不在本模块：16 个工具由 `plugins/DAAgentTools/` 提供；聊天 UI（`DAAgentDockWidget`）在 DAGui/Agent；LLM 设置页（`DAAgentSettingsWidget`）在 `src/APP/SettingPages/`。

---

### 应用层 (L5)

#### APP — 主程序

职责：可执行程序，所有抽象接口的具体实现，整合所有模块。

提供内容：
- `DAAppCore` — 核心单例，初始化所有子系统
- `AppMainWindow` — 主窗口（SARibbonMainWindow 子类）
- `DAAppUI` — Ribbon/Docking 布局管理
- `DAAppController` — MVC 控制器
- `DAAppPluginManager` — 插件管理器具体实现
- `DAAppProject` — 项目文件序列化
- `DAAppDataManager/DockingArea/RibbonArea/StatusBar/Command/Actions` — 各 Interface 的具体实现
- `SettingPages/` — 设置页面（通用设置、应用配置）
- `Dialog/` — 应用级对话框（关于、PNG导出）
- `PythonBinding/` — pybind11 应用绑定（Python触发UI操作）

外部依赖：DAPluginSupport + DAPyWorkFlow (PUBLIC), Qt全组件, 所有第三方库

---

## ⚠️ 历史重构记录：DAPyWorkFlow 的类迁移

`src/DAPyWorkFlow` 模块最初由 AI 编写，曾有多个类放错了位置。以下类已在之前的重构中被迁移到正确位置：

| 已迁移类 | 原位置 | 现位置 | 说明 |
|---------|--------|--------|------|
| `DAPyGILGuard` / `DAPyGILRelease` / `DAPySafePyObjectHolder` | `DAPyWorkFlow/DAPyGILGuard.*` | `DAPyBindQt/DAPyGILGuard.h` | 通用 pybind11 GIL RAII 工具，已迁移至基础层 |
| `DAParameterDescriptor` | `DAPyWorkFlow/DAParameterDescriptor.h` | `DAGui/NodeSetting/ParameterDescriptor.h` | 参数描述符已重构至 DAGui/NodeSetting/ 下 |
| `DAPortDescriptor` | `DAPyWorkFlow/DAPortDescriptor.h` | 已移除 | 端口描述符已不再需要，端口信息通过 `DAPyLinkPoint` 管理 |

仍待评估的类：

| 待评估类 | 当前位置 | 可考虑迁移至 | 原因 |
|---------|---------|------------|------|
| `DAPyPainterProxy` | `DAPyWorkFlow/DAPyPainterProxy.*` | **`DAPyBindQt/`** | QPainter↔Python 桥接，无工作流逻辑。但仅用于节点绘制回调，暂保留。 |

---

## AI 开发检查清单

在创建新类/文件时，**必须**依次回答以下问题：

1. **这个类的功能属于哪个模块的职责范围？** → 对照§各模块职责详解
2. **放在这个模块是否会引入违反依赖方向的依赖？** → 下层不能依赖上层
3. **这个类能否被其他不依赖当前模块的模块复用？** → 如果是，应该下沉到更低层模块
4. **这个类是通用工具/基础类型吗？** → DAShared（纯头文件）或 DAUtils
5. **这个类涉及 Python 绑定吗？** → DAPyBindQt（基础绑定）还是 DAPyWorkFlow（工作流特定）

### 典型放置指南

| 如果你要创建一个... | 放在... | 原因 |
|-------------------|---------|------|
| 通用模板、算法、数据结构 | DAShared | 纯头文件，零编译依赖 |
| 通用工具类（字符串、文件、进程） | DAUtils | 全项目通用 |
| Python↔Qt 类型转换、py::object 包装 | DAPyBindQt | Python 基础设施 |
| 通用 UI 控件（颜色选择器、属性面板） | DAGui（根级，原 DACommonWidgets 已并入） | 界面层通用组件 |
| 工作流节点可视化（图形项、场景） | DAPyWorkFlow | 工作流专用 |
| 图表系统组件（编辑器、序列化） | DAFigure | 图表专用 |
| 数据管理（DAData包装、管理器） | DAData | 数据抽象层 |

---

## 注意事项

!!! warning "依赖顺序"
    模块链接顺序很重要，上层模块必须在下层模块之后链接。错误的链接顺序会导致编译失败。

!!! warning "模块隔离"
    功能层模块之间尽量减少直接依赖，通过接口层（DAInterface）进行通信。

!!! note "DAPyWorkFlow 重构记录"
    DAPyWorkFlow 模块历史上存在类放置错误，大部分已在之前的重构中迁移（见上方§历史重构记录）。创建新类前仍需判断是否属于 DAPyBindQt 或 DAShared。

!!! note "Python模块"
    Python相关模块需要Python环境和pybind11支持。Python 为强制依赖，DAPyBindQt/DAPyScripts/DAPyCommonWidgets/DAPyWorkFlow 始终参与构建，DAData/DAGui/DAInterface/DAPluginSupport/APP 中的 Python 相关代码也始终参与编译。

!!! note "平台差异"
    DAAxOfficeWrapper 仅在 Windows 平台构建，使用 Qt::AxContainer 组件。

---

## 参考资料

- [项目构建指南](../../build/build-instructions.md)
- [插件模块DAPluginSupport](../../plugin/plugin-module.md)
- [绘图模块概述](../graphics/figure-abstract.md)
- [枚举/字符串转换工具](../general/da-enum-string-utils.md)
