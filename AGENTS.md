# data-workbench 项目指南

## OVERVIEW

AI Agent 驱动的数据分析工作平台 — C++17/Qt 有向图工作流引擎 + 内嵌 Python (pandas/numpy) + 交互式图表。核心栈: Qt 5.14+/6, pybind11, SARibbon, qwt, Qt-Advanced-Docking-System。

**核心价值**：

- 工作流驱动自动化处理重复性数据
- GUI 封装 pandas 核心功能，无需编程即可操作
- 交互式图表编辑，生成论文级别矢量图
- 插件化架构，易于扩展自定义功能
- 集成agent，实现ai分析

## STRUCTURE

```
data-workbench/
├── .github/workflows/  # CI (build.yml, page.yml)
├── cmake/              # CMake 工具链 (daworkbench_utils.cmake, 3rdparty.cmake, plugin_utils.cmake)
├── docs/zh/            # 中文技术文档 (dev-guide/, use-guide/, build/)
├── plugins/            # 插件 (DataAnalysis, CrewAIAdapter, DAAgentTools, plugin-template)
├── src/
│   ├── 3rdparty/       # 第三方库 (qwt, SARibbon, qt-advanced-docking 等)
│   ├── DAShared/       # 共享基础类型 (20 files)
│   ├── DAUtils/        # 工具类 (40 files)
│   ├── DAAxOfficeWrapper/ # Windows Office 封装 (10 files, 仅 Win)
│   ├── DAMessageHandler/  # 日志/消息 (9 files)
│   ├── DAPyBindQt/     # Python↔Qt 绑定层 (28 files)
│   ├── DAPyScripts/    # Python 脚本加载 (12 files)
│   ├── DAPyCommonWidgets/ # Python 通用 Widgets (13 files)
│   ├── DAPyWorkFlow/   # Python 工作流节点 (50 files)
│   ├── DAData/         # 数据管理/结构 (22 files)
│   ├── DACommonWidgets/ # 通用 UI 组件 (82 files)
│   ├── DAGraphicsView/ # 图形视图框架 (54 files)
│   ├── DAFigure/       # 图表/Figure 容器 (103 files)
│   ├── DAGui/          # GUI Widgets/面板/对话框 (334 files, 最大模块)
│   │   ├── ChartSetting/  # 图表属性设置面板
│   │   ├── NodeSetting/   # 工作流节点通用设置面板 (DAFormSpec 统一表单)
│   │   ├── Commands/      # QUndoCommand 子类
│   │   ├── Dialog/        # 各种对话框
│   │   ├── MimeData/      # 拖放 MIME 数据
│   │   └── Models/        # Qt Model/View 模型
│   ├── DAInterface/    # 核心接口定义 (28 files)
│   ├── DAPluginSupport/ # 插件框架 (10 files)
│   ├── APP/            # 应用主程序 (190 files)
│   │   ├── Dialog/     # 应用级对话框
│   │   ├── Icon/       # 图标资源
│   │   ├── PythonBinding/ # Python 绑定初始化
│   │   └── SettingPages/  # 设置页面
│   ├── PyScripts/      # 内置 Python 脚本 (23 files)
│   ├── i18n/           # 国际化翻译
│   └── tst/            # 测试 (7 files)
├── CMakeLists.txt      # 顶层 CMake (项目名 DAWorkbench)
└── requirements.txt    # Python 依赖
```

### 模块构建依赖顺序

```
基础层: DAShared → DAUtils
       ├→ DAAxOfficeWrapper (Win only)
       └→ DAMessageHandler
Python层: DAUtils → DAPyBindQt → DAPyScripts → DAPyCommonWidgets → DAPyWorkFlow
功能层: DAData (→ DAUtils, DAPyBindQt, DAPyScripts)
       DACommonWidgets (→ DAUtils)
       DAGraphicsView (→ DAUtils)
       DAFigure (→ DAUtils + Qwt)
界面层: DAGui (→ 所有上述模块 + SARibbon/ADS/qwt)
接口层: DAInterface (→ DAGui)
       DAPluginSupport (→ DAInterface + DAPyWorkFlow)
       DAAgent (→ DAInterface/DAData/DAPyBindQt/DAPyScripts；纯 agent 框架库 + 提示词库管理，不依赖 GUI 模块)
应用层: APP (→ DAPluginSupport)
```

---

## MODULE DEPENDENCY（模块依赖关系）

> ⚠️ **AI 开发必读**：在创建任何新类之前，**必须先判断它属于哪个模块**。将类放在错误的模块会导致模块间产生不必要的依赖、代码复用困难、项目架构混乱。
>
> 📖 **模块职责边界、依赖矩阵、典型放置指南详见** [docs/zh/dev-guide/module-dependency.md](docs/zh/dev-guide/module-dependency.md)

### 五层架构总览

```
┌─────────────────────────────────────────────────┐
│ Layer 5: 应用层    │ APP (可执行程序)            │
├─────────────────────────────────────────────────┤
│ Layer 4: 接口层    │ DAInterface, DAPluginSupport,│
│                    │ DAAgent                      │
├─────────────────────────────────────────────────┤
│ Layer 3: 界面层    │ DAGui, DACommonWidgets      │
├─────────────────────────────────────────────────┤
│ Layer 2: 功能层    │ DAData, DAFigure, DAPyWorkFlow,
│                    │ DAGraphicsView, DAPyScripts,
│                    │ DAPyCommonWidgets            │
├─────────────────────────────────────────────────┤
│ Layer 1: 基础层    │ DAShared, DAUtils,
│                    │ DAMessageHandler, DAPyBindQt │
└─────────────────────────────────────────────────┘
```

### 各模块职责边界

> 📖 **完整的职责边界表、依赖矩阵、典型放置指南详见** [docs/zh/dev-guide/module-dependency.md](docs/zh/dev-guide/module-dependency.md)

| 层 | 模块 |
|----|------|
| **L1 基础层** | DAShared（纯头文件）、DAUtils、DAMessageHandler、DAPyBindQt |
| **L2 功能层** | DAData、DAFigure、DAPyWorkFlow、DAGraphicsView、DAPyScripts、DAPyCommonWidgets |
| **L3 界面层** | DAGui、DACommonWidgets |
| **L4 接口层** | DAInterface、DAPluginSupport、DAAgent |
| **L5 应用层** | APP |

### 依赖方向规则（铁律）

1. **上层可以依赖下层，下层绝不能依赖上层。**（✅ DAGui → DAUtils；❌ DAUtils → DAGui）
2. **同层模块尽量减少直接依赖**，通过上层整合模块（DAGui）协调。
3. **APP层是顶层，用于桥接各个层数据和信号**，不要把过多逻辑在APP层实现而是下沉到各个库，抽象对应功能
4. **DAShared 是纯头文件库**，无需显式 CMake 链接。

### AI 开发检查清单

创建新类/文件时**必须**回答：

1. 这个类的功能是否在这个模块的职责范围内？（对照 [module-dependency.md](docs/zh/dev-guide/module-dependency.md) 的职责边界表）
2. 是否会引入违反依赖方向的依赖？（下层不能依赖上层）
3. 是否可以被其他不依赖当前模块的模块复用？→ 考虑下沉到更低层
4. 是否是通用工具/基础类型？→ DAShared（纯头文件）或 DAUtils
5. 是否涉及 Python 绑定且通用？→ DAPyBindQt（而非 DAPyWorkFlow）

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| 程序入口 / main() | `src/APP/main.cpp` | QApplication, Python 初始化, AppMainWindow |
| 主窗口 | `src/APP/AppMainWindow.h/.cpp` | 继承 SARibbonMainWindow, 管理 Docking/UI/Controller |
| 应用核心(单例) | `src/APP/DAAppCore.h/.cpp` | 初始化所有子系统 |
| 插件管理 | `src/APP/DAAppPluginManager.h/.cpp` | 加载/卸载插件 |
| 项目管理 | `src/APP/DAAppProject.h/.cpp` | 工程文件读写 |
| 数据处理接口 | `src/DAData/` | 数据容器, DataFrame 封装 |
| 工作流节点通用设置 | `src/DAGui/NodeSetting/` | 基于 DAFormSpec/DAPropertyFormWidget 的统一表单参数面板, PIMPL |
| 图表创建/编辑 | `src/DAFigure/`, `src/DAGui/ChartSetting/` | QwtFigure 容器 + 属性面板 |
| 图形视图交互 | `src/DAGraphicsView/` | QGraphicsView 子类, 节点/连线编辑 |
| Python 绑定 | `src/DAPyBindQt/` | pybind11 胶水代码 |
| pybind11↔Qt 类型转换 | `src/DAPyBindQt/DAPybind11QtCaster.hpp` | **重要**：QString/QVariant/QDateTime 等自动双向转换，详见`docs/zh/dev-guide/dapybind11-qt-caster.md` |
| Python 工作流节点 | `src/DAPyWorkFlow/` | Python 脚本节点执行 |
| 命令行参数 | `src/APP/main.cpp` → `initCommandLine()` | --version, --help, import-data, 工程文件 |
| UI 状态持久化 | `src/APP/AppMainWindow` | saveUIState/restoreUIState |
| 翻译/国际化 | `src/i18n/`, `src/APP/App_zh_CN.ts` | DATranslatorManeger |
| 崩溃转储 | `src/APP/DADumpCapture.h` | Windows dump 文件生成 |
| 日志系统 | `src/DAMessageHandler/` | DALogger + daInfo/daWarning/daCritical 便捷宏，详见 `docs/zh/dev-guide/logging.md` |
| PIMPL 宏定义 | `src/DAGlobals.h` | DA_DECLARE_PRIVATE, DA_D, DA_DC 等 |
| Qt5/Qt6 兼容宏 | `src/DAGlobals.h` | Qt5Qt6Compat_* 系列宏 |
| 通用 Widgets | `src/DACommonWidgets/` | 按钮、列表、树等基础组件 |
| 枚举/字符串转换 | `src/DAShared/DAEnumStringUtils.hpp` | 通用枚举↔字符串映射宏，详见`docs/zh/dev-guide/da-enum-string-utils.md` |
| 插件开发参考 | `plugins/DataAnalysis/` | 最完整的插件示例 |
| Agent 工具/设置 | `plugins/DAAgentTools/`（19 内置工具）/ `src/APP/SettingPages/DAAgentSettingsWidget`（LLM 设置页） | 工具由 `DAAgentToolsPlugin` 注册；设置页经 `setAgentInterface` 持久化 `agent-config.ini`（DAAgent 不依赖 DAGui） |
| Agent 提示词库 | `src/DAAgent/DAAgentManager`（提示词引擎，实现 `DAAgentPromptOps`）/ `src/DAAgent/DAAgentPromptOps`（CRUD 回调接口）/ `src/DAAgent/DAAgentPrompt`（提示词数据结构）/ `src/DAUtils/DAMarkdownHighlighter`（通用 Markdown 高亮器）/ `src/APP/Dialog/DAAgentManagerDialog`、`DAAgentEditorDialog`（管理/编辑 UI） | 平台核心能力：`DAAgentInterface` 的 `registerBuiltinAgent`/`runAgent`/`agentPromptOps` 三方法；提示词库类型与引擎均在 DAAgent 模块内，通用高亮器在 DAUtils，对话框在 APP/Dialog（参照 `DAAgentSettingsWidget` 的 L5→L4 依赖先例）；插件经 `registerBuiltinAgent` 注入领域提示词，UI 由 `DAAppRibbonArea::buildRibbonAgentCategory` 构建「AI分析」标签页 |
| Python 工作流节点开发 | `plugins/DASystemNodes/AGENTS.md` | @NodeDef 节点开发规范、**init**/paint() 等核心陷阱 |
| 插件模板 | `plugins/plugin-template/` | 新插件脚手架 |
| 文档源码 | `docs/zh/` | Doxygen Wiki 中文 |

## 日志输出规范

AI 编写代码时，日志宏的选择直接影响日志是否进入 UI 消息队列（用户可见），必须遵循以下分流规则。日志文件位置、读取方法、故障排查场景详见 [logging.md](docs/zh/dev-guide/logging.md) § 日志读取与故障排查。

### 日志宏与分流

- 业务代码用 `daDebug` / `daInfo` / `daWarning` / `daCritical` 宏（`src/DAMessageHandler/DALogCategory.h`），category = `da.user`，既写文件又进 UI 消息队列(**注意：使用da* 的日志是会在界面显示，必须国际化**)
- `qDebug` / `qInfo` / `qWarning` / `qCritical`（Qt 自身、第三方库）只写文件和控制台，不进 UI 队列
- 第三方库日志（SARibbon、qwt、ADS、QtWebEngine 等）在文件里可见，排查时不要误认为是本项目代码——看 `[源文件:行号]` 字段，本项目代码文件路径在 `src/` 下

> 📖 何时用 `da*` vs `q*` 的判断标准详见 § 国际化（i18n）规范；日志系统 API、配置、故障排查详见 [docs/zh/dev-guide/logging.md](docs/zh/dev-guide/logging.md)
> 当用户报告运行异常、崩溃、行为不符合预期时，应主动读取程序运行日志分析根因，不要凭猜测下结论，详见上述文档。

## 文档导航

> 📖 **完整文档站点**：`mkdocs serve` 后访问 http://localhost:8000，或浏览 `docs/zh/`

| 类别 | 关键文档 | 说明 |
|------|---------|------|
| **开发指南** | [module-dependency.md](docs/zh/dev-guide/module-dependency.md) | 模块职责边界、依赖矩阵 |
| | [coding-standard.md](docs/zh/dev-guide/coding-standard.md) | 编码规范、命名约定、注释规范 |
| | [architecture.md](docs/zh/dev-guide/architecture.md) | 架构设计与模块划分 |
| | [developer-guide.md](docs/zh/dev-guide/developer-guide.md) | 开发者入门指引 |
| | [module-breakdown.md](docs/zh/dev-guide/module-breakdown.md) | 各模块业务逻辑详解 |
| | [i18n.md](docs/zh/dev-guide/i18n.md) / [python-i18n.md](docs/zh/dev-guide/python-i18n.md) | 国际化规范 |
| | [workflow-overview.md](docs/zh/dev-guide/workflow-overview.md) | 工作流系统架构 |
| | [workflow-python-node-dev.md](docs/zh/dev-guide/workflow-python-node-dev.md) | Python 节点开发 |
| | [dapybind11-qt-caster.md](docs/zh/dev-guide/dapybind11-qt-caster.md) | pybind11↔Qt 类型转换 |
| | [chart-dock-nesting.md](docs/zh/dev-guide/chart-dock-nesting.md) | 绘图窗口 ADS 嵌套停靠区、FocusHighlighting 焦点陷阱 |
| | [logging.md](docs/zh/dev-guide/logging.md) | 日志系统、故障排查 |
| | [creating-setting-panel.md](docs/zh/dev-guide/creating-setting-panel.md) | 创建设置面板 |
| **构建** | [build-instructions.md](docs/zh/build/build-instructions.md) | 完整构建指南 |
| | [build-options.md](docs/zh/build/build-options.md) | CMake 构建选项参考 |
| | [common-build-errors.md](docs/zh/build/common-build-errors.md) | 常见构建错误 |
| **使用指南** | [use-guide/index.md](docs/zh/use-guide/index.md) | 用户指南入口 |
| | [workflow-usage.md](docs/zh/use-guide/workflow-usage.md) | 工作流使用 |
| | [data-management.md](docs/zh/use-guide/data-management.md) | 数据管理使用 |
| | [chart-usage.md](docs/zh/use-guide/chart-usage.md) | 图表使用 |
| **顶层** | [overview.md](docs/zh/overview.md) | 项目概览 |
| | [plugin-development.md](docs/zh/plugin-development.md) | 插件开发总览 |
| | [api-reference.md](docs/zh/api-reference.md) / [overview.md](docs/zh/api-reference/overview.md) | API 参考 |


## CONVENTIONS

> 📖 **详细编码规范见** [docs/zh/dev-guide/coding-standard.md](docs/zh/dev-guide/coding-standard.md)

### 代码风格

- 严格保持与现有代码一致（命名规范、缩进、头文件组织等）
- 代码文件、类名统一`DA`开头，并放入`DA`命名空间
- 遵循 Qt 开发最佳实践（使用 `Q_PROPERTY`、`Q_SIGNALS`、`Q_SLOT` 等宏，禁止使用 `slot`、`signal` 等小写命名的宏）
- 工具类使用信号和槽进行事件通讯，属性使用 `Q_PROPERTY` 暴露

### Qt 版本兼容性

代码需兼容 Qt5 和 Qt6，差异处理使用宏判断（`Qt5Qt6Compat_*` 系列宏定义在 `src/DAGlobals.h`）：

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 方法
#else
    // Qt6 方法
#endif
```

### Qt 容器范围迭代（避免 COW 深拷贝）

**禁止**对非 const Qt 容器直接使用范围迭代（`for(T& v : container)` 或 `for(const T& v : container)` 都会触发 COW 深拷贝）。必须用 `const` 声明容器或 `std::as_const()` 包裹。

## 国际化（i18n）规范

> 📖 **详细规范见** [docs/zh/dev-guide/i18n.md](docs/zh/dev-guide/i18n.md) | Python 详见 [docs/zh/dev-guide/python-i18n.md](docs/zh/dev-guide/python-i18n.md)

**核心原则**：所有显示到用户界面的字符串必须使用**英文源文本 + 行内中文注释**模式（`tr("English") //cn:中文` / `_("English") # cn:中文`），翻译通过 `.ts`/`.po` 文件提供。源码中**禁止**直接写中文作为 UI 显示文本。

**关键约束**：

- **开发诊断日志不翻译**：`qInfo`/`qWarning`/`qCritical`/`qDebug`/`logger.*`/`print` 只写日志文件不进 UI 队列，保持纯英文
- **DA 界面消息必须翻译**：`daInfo`/`daWarning`/`daCritical`（`src/DAMessageHandler/DALogCategory.h`，category=`da.user`）既写日志文件又进 UI 消息队列（用户可见），**必须**用 `tr("English")  //cn:中文` 翻译
- **选宏判断标准**：写给用户看的操作反馈（成功/失败提示、可理解错误、状态栏消息）用 `da*` + `tr()`；写给开发者排查的技术诊断（`ClassName::method:` 前缀、异常 `what()` 转储、内部状态）用 `q*` 保持英文。开发诊断信息**禁止**用 `da*`（会污染 UI 消息队列），详见 [i18n.md](docs/zh/dev-guide/i18n.md) § da* 与 q* 宏的选择
- **`@NodeDef(name=...)` 不翻译**：`name` 参与 `qualified_name` 序列化，翻译会破坏已存工程
- **Python 包 `setup_i18n()` 必须在 `__init__.py` 顶部、节点模块导入之前调用**

## 注释与文档规范

> 📖 **详细规范见** [docs/zh/dev-guide/coding-standard.md](docs/zh/dev-guide/coding-standard.md) 的"注释规范"章节

**核心原则**：函数的 Doxygen 注释写在 `.cpp` 文件中，头文件只保留单行中文简要注释（`//`）。头文件仅可写类/信号/枚举的注释，**禁止**在头文件中写入类成员函数的 Doxygen 块注释（hpp文件除外，头文件的模板函数除外）。

## 插件系统

插件位于 `plugins` 目录下。涉及插件开发时请阅读：

- [插件项目创建](./docs/zh/dev-guide/plugin-project-create.md)
- [插件与接口](./docs/zh/dev-guide/plugins-interfaces.md)
- [插件模块 DAPluginSupport](./docs/zh/dev-guide/plugin-module.md)
- [插件开发创建 UI](./docs/zh/dev-guide/plugin-dev-create-ui.md)
- 参考现有插件结构（如 `plugins/DataAnalysis/`），确保与主程序接口兼容

## Git 提交规范

在完成当前任务后，需提交所有更改到 Git 仓库。
创建有意义的提交信息保证下次任务能清楚了解这次任务的实现情况
提交信息最好包含以下信息：

- 任务类型（例如：实现、修复、文档更新）
- 实现内容的简要描述
- 相关文件列表
- 关联到计划书（如果适用）

## COMMANDS

> **🔴 强制规则：构建前必须先阅读 root `build.md`**
>
> 不要凭记忆或假设执行构建命令。`build.md` 包含完整的构建流程、常见问题排查和 Agent 快速参考。
> 如果构建失败，**首先检查 `build.md` 中的"常见构建问题"章节**，不要自行猜测原因。

### 日常构建（Agent 直接执行）

> **前提**：第三方库已编译安装（项目根目录下 `bin_*` 安装目录已存在）。第三方库仅需首次构建或 submodule 更新时编译，日常开发无需重复。

#### Windows（使用脚本，推荐）

`scripts/build.ps1` 自动探测 Qt/VS 路径，**Agent 在 Windows 上应优先使用此脚本**。

```powershell
.\scripts\build.ps1 -Target DAPyWorkFlow       # 编译指定模块
.\scripts\build.ps1 -Target DAPyWorkFlow -Test  # 编译并运行测试
.\scripts\build.ps1 -Full                        # 完整构建
.\scripts\build.ps1 -Clean                       # 清理重新配置+编译
```

> **🔴 Windows 禁止使用 Ninja 生成器**：必须使用 Visual Studio 生成器。PowerShell 中 MSVC 环境无法正确注入，Ninja 会导致 `fatal error C1083: 无法打开包括文件: "type_traits"` 等错误。
>
> 如果 `build/` 目录已存在且是用 Ninja 配置的，必须使用 `.\scripts\build.ps1 -Clean` 清理后重新配置。

#### Linux / WSL

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
```

### 首次构建：编译第三方库

仅首次构建或第三方库 submodule 有更新时需要执行。产物安装到 `bin_<BuildType>_qt<QtVersion>_<Compiler>_<Arch>/`。

```powershell
# Windows
cmake -S src/3rdparty -B build-3rdparty -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build-3rdparty --config Release --parallel
cmake --install build-3rdparty --config Release
```

```bash
# Linux / WSL
cmake -S src/3rdparty -B build-linux-3rdparty -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux-3rdparty --parallel
cmake --install build-linux-3rdparty
```

### 运行测试

#### Windows

```powershell
# 使用脚本（推荐）
.\scripts\build.ps1 -Target DAPyWorkFlow -Test

# 手动（Qt Test 在 Windows 上 stdout 不可见，必须用 -o）
.\build\src\tst\<测试模块>\Release\<测试模块>.exe -o test_result.txt
Get-Content test_result.txt
```

#### Linux / WSL

```bash
./build-linux/src/tst/<测试模块>/<测试模块> -o test_result.txt
cat test_result.txt
```

| 注意事项 | Windows | Linux |
|----------|---------|-------|
| 测试 exe 路径 | VS: `build\src\tst\<模块>\<Config>\` | Ninja: `build-linux/src/tst/<模块>/` |
| 输出捕获 | **必须**用 `-o file.txt` | 可直接 stdout 或 `-o file.txt` |
| 退出码 | `$LASTEXITCODE` | `$?` 或 `echo $?` |

### 生成器选择

| 平台 | 推荐生成器 | 说明 |
|------|-----------|------|
| **Windows** | Visual Studio | 自动检测 MSVC，无需手动初始化环境 |
| **Windows** | Ninja ⚠️ | **必须**在 Developer Command Prompt 中运行 |
| **Linux / WSL** | Ninja ✅ 推荐 | 编译速度快，无需特殊环境设置 |

## 跨平台构建注意 (Linux / WSL)

项目原本在 Windows 下开发，Linux/WSL 构建需注意以下差异：

### Windows-only 代码需 `if(WIN32)` 保护

以下模块/链接选项仅在 Windows 有效，必须在 `if(WIN32)` 内：

- `Qt6::AxContainer` — MSVC ActiveX 容器，Linux 无此模块
- `DAWorkbench::DAAxOfficeWrapper` — Office 封装库，仅 Win
- `/SUBSYSTEM:WINDOWS`、`/SUBSYSTEM:CONSOLE` — MSVC 链接器选项，Linux GCC 不识别

### Linux uint64_t ≠ unsigned long long

Linux GCC 下 `uint64_t` 是 `unsigned long`，MSVC 下是 `unsigned long long`。调用 `QDomElement::setAttribute(ulonglong)` 或 `QVariant` 相关函数时会产生重载歧义。

- **解决方案**：使用 `qulonglong`（Qt 类型，跨平台统一为 `unsigned long long`）或 `static_cast<qulonglong>()` 显式转换

### Qt6 隐式头文件变化

Qt6 不再通过 `QDataStream` 隐式包含 `<QIODevice>`。如编译报 `QIODevice` 不完整类型，手动添加 `#include <QIODevice>`。

### 信号槽传递自定义类型指针

Qt 信号槽中传递自定义类指针（如 `DAPyNodeGraphicsItem*`），若头文件仅有前向声明，`connect` 会导致不完整类型错误。

- **解决方案**：在 .cpp 文件中 `#include` 完整头文件，而非仅依赖前向声明

## ANTI-PATTERNS (THIS PROJECT)

- QwtPlotItem相关的类不继承QObject，不要使用Qt的信号槽机制，继承 Qwt 非 QObject 类时**不能使用 Q_OBJECT 宏**
- 禁止使用 `slot`、`signal` 小写命名的宏，统一使用 `Q_SLOTS`、`Q_SIGNALS`
- 禁止在头文件中写入类成员函数的 Doxygen 块注释（仅限类的注释、信号注释、枚举注释）
- 禁止在 QwtPlotItem 子类中使用信号槽
- 禁止使用已废弃的 DAPyNodeConfigDialog / DAPyNodeWidget — 统一使用 `src/DAGui/NodeSetting/` 中基于 `DAPropertyFormWidget` 的通用参数面板
- **禁止在错误的模块创建类** — 创建新类前必须对照 § MODULE DEPENDENCY 确定它属于哪个模块（典型反面：通用工具放进 DAPyWorkFlow）
- **禁止对非 const Qt 容器直接使用范围迭代** — `for(T& v : container)` 和 `for(const T& v : container)` 对非 const 容器都会触发 COW 深拷贝。必须用 `const` 声明容器或 `std::as_const()` 包裹（详见 § Qt 容器范围迭代）
- **禁止在 `.cpp` 中使用 Qt↔Python 类型转换而未 `#include "DAPybind11QtCaster.hpp"`** — pybind11 的 `type_caster` 是 **per-translation-unit** 生效的，仅 include `DAPybind11InQt.h` 不够。每个 `.cpp` 文件只要出现以下任意调用形式，就必须在该文件顶部 include `src/DAPyBindQt/DAPybind11QtCaster.hpp`：
  - `attr(...)(QString)` / `attr(...)(QVariant)` / `attr(...)(QDateTime)` 等 — 把 Qt 类型作为参数传给 Python 可调用对象
  - `pybind11::cast(QString)` / `pybind11::cast<QVariant>(...)` — 显式 cast Qt 类型
  - `.cast<QString>()` / `.cast<QVariant>()` — 从 Python 对象 cast 到 Qt 类型
  - 涉及的 Qt 类型包括：`QString`、`QByteArray`、`QDate`、`QTime`、`QDateTime`、`QList<T>`、`QVector<T>`(Qt5)、`QSet<T>`、`QHash<K,V>`、`QMap<K,V>`、`QVariant`

  遗漏 include **不会编译报错**（头文件间接可见时能编译通过），但运行时会抛 `Unable to convert call argument 'N' of type 'QString' to Python object`，且异常被 `dealException` 吞掉后表现为后续业务逻辑静默失败（如节点查找 KeyError、数据丢失等），极难排查。详见 `docs/zh/dev-guide/dapybind11-qt-caster.md` 与 `src/DAPyWorkFlow/AGENTS.md` § 类型转换铁律
- **禁止在源码中直接写中文作为 UI 显示文本** — 必须用 `tr("English") //cn:中文`（C++）或 `_("English") # cn:中文`（Python）模式。源文本统一英文，翻译放 `.ts`/`.po` 文件。详见 § 国际化（i18n）规范
- **禁止翻译 `@NodeDef(name=...)`** — `name` 参与 `qualified_name` 序列化（如 `DASystemNodes.Delay`），翻译会破坏已存工程的节点匹配。`category`/`description` 可翻译
- **禁止翻译开发诊断日志** — `qInfo`/`qWarning`/`qCritical`/`qDebug`/`logger.*`/`print` 只写日志文件不进 UI 队列，保持纯英文便于跨语言检索
- **禁止用 `da*` 宏输出开发诊断信息** — `daInfo`/`daWarning`/`daCritical` 会进 UI 消息队列（用户可见）且必须翻译；带 `ClassName::method:` 前缀的技术诊断、异常 `what()` 转储等开发诊断应改用 `q*` 宏保持英文。详见 § 国际化规范
- **禁止在 Python 节点包中遗漏 `setup_i18n()` 调用** — 必须在包 `__init__.py` 顶部、节点模块导入之前调用 `setup_i18n()`，否则 `_()` 未定义会导致节点注册失败

## UNIQUE STYLES

- 项目使用PIMPL模式，PIMPL相关宏定义在`src/DAGlobals.h`中,主要有如下宏需要使用：
  - `DA_DECLARE_PRIVATE`:在`MyClass`中定义
  - `DA_DECLARE_PUBLIC`:在`MyClass::PrivateData`中声明
  - `DA_PIMPL_CONSTRUCT`:在`MyClass::在MyClass`构造函数中初始化
  - `DA_D`:在`MyClass::fun()`中获取`MyClass::PrivateData`的指针
  - `DA_DC`:在`MyClass::fun() const`中获取`MyClass::PrivateData`的const指针
- 所有类、文件名统一 `DA` 前缀，放入 `DA` 命名空间
- Doxygen 注释使用中文
- 头文件保持简洁：仅单行中文注释，详细文档在 .cpp 中

## 相关文件

| 文件 | 说明 |
|------|------|
| [README.md](README.md) | 项目简介和第三方库说明 |
| [docs/doc-writing-guide.md](docs/doc-writing-guide.md) | 文档撰写规范手册，涉及文档撰写时阅读 |
| [docs/zh/index.md](docs/zh/index.md) | 中文文档入口 |

## NOTES

- Python 解释器路径通过 `DAPyInterpreter::getPythonInterpreterPath()` 自动检测
- 第三方库 (SARibbon, qwt, Qt-Advanced-Docking-System 等) 需先通过 `src/3rdparty/CMakeLists.txt` 编译安装
- `src/3rdparty/qwt/` 是 Qwt 7.x 维护分支, 有自己的 AGENTS.md
- CI 通过 `.github/workflows/build.yml` 自动构建
- 崩溃转储 (.dmp) 生成在 `dumps/` 目录，由 `DADumpCapture` 管理
- 翻译文件通过 CMake option `DA_ENABLE_AUTO_TRANSLATE` 自动生成
- 构建请优先阅读 root `build.md`（包含 PowerShell 专用说明），或直接使用 `scripts/build.ps1`
- `src/DAGui/NodeSetting/` 为工作流节点通用设置面板模块，遵循 ChartSetting 的三层架构 (基类→面板→具体面板 + 单例工厂 + QStackedWidget 调度器)
- 如果你首次编译或者首次使用`git worktree`命令,你需要先拉取第三方库：`git submodule update --init --recursive`
