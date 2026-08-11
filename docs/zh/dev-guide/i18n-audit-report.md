# i18n 全盘审计报告与修复计划

> 审计日期：2026-08-11
> 审计范围：`src/`（C++，排除 `src/3rdparty/`）、`plugins/`（C++ + Python）、`src/PyScripts/`（Python）、翻译文件（`.ts`/`.po`）
> 依据规范：`AGENTS.md` § 国际化规范、`docs/zh/dev-guide/i18n.md`、`docs/zh/dev-guide/python-i18n.md`

---

## 〇、先决事项：规则冲突（已解决 ✅）

多个审计 agent 独立发现 `docs/zh/dev-guide/i18n.md` 与 `AGENTS.md` 自相矛盾，已由用户拍板解决：

| 来源 | 对 `daInfo`/`daWarning`/`daCritical` 的定性 |
|------|---------------------------------------------|
| `AGENTS.md` § 国际化规范 | **界面消息，必须翻译**（"都是会显示到 ui 界面的消息，必需翻译"）✅ 正确 |
| `docs/zh/dev-guide/i18n.md` L50/L149（旧） | **日志消息，不翻译** ❌ 文档错误，已修正 |
| 代码实证 `src/DAAgent/DAAgentModule.cpp:250` 注释 | "daCritical 会路由到 UI 日志窗口"——确认 UI 可见 |

**已确认的规则**（已同步修正 `i18n.md` + `AGENTS.md`）：

1. `daInfo`/`daWarning`/`daCritical` 是**面向用户的界面消息**（进 UI 消息队列），**必须**用 `tr("English")  //cn:中文` 翻译
2. `qInfo`/`qWarning`/`qCritical`/`qDebug` 是**开发诊断日志**（不进 UI 队列），保持纯英文不翻译
3. **带 `ClassName::method:` 前缀的开发诊断式消息不应使用 `da*` 宏**——它们应改用 `q*` 宏保持英文，避免污染 UI 消息队列

> 文档已修正：`i18n.md` 新增「da* 与 q* 宏的选择」决策表；`AGENTS.md` 关键约束与 ANTI-PATTERNS 已同步更新。

---

## 一、违规总览（按严重度）

| # | 违规类型 | HIGH | MEDIUM | LOW | 小计 | 涉及区域 |
|---|---------|------|--------|-----|------|---------|
| 1 | C++ 中文源文本写在 `tr()` 内 | 14 | — | — | 14 | `src/DAAgent`、`src/DAPyWorkFlow` |
| 2 | C++ 中文 UI 文本未用 `tr()` | 39 | — | — | 39 | `src/DAGui`、`plugins/DataAnalysis`、测试 |
| 3 | C++ `daInfo/daWarning/daCritical` 未用 `tr()` | — | 68 | — | 68 | `src/DAPyWorkFlow`、`src/DAAgent`、`src/DAData` 等 |
| 4 | C++ 日志（`q*`）被 `tr()` 包裹 | — | 44 | — | 44 | `src/DAGui`、`src/APP`、`src/DAPyWorkFlow` |
| 5 | C++ 文件过滤器通配符在 `tr()` 内 | — | 0 | — | 0 | — |
| 6 | C++ `tr()` 缺 `//cn:` 注释 | — | 2 | 13 | 15 | `plugins/DataAnalysis`、`src/DAAgent`、`src/APP` |
| 7 | C++ 日志字符串含中文（未 tr） | — | 14 | — | 14 | `src/DAPyWorkFlow`、`src/DAPyBindQt`、`src/APP` |
| 8 | Python 中文源文本写在 `_()` 内 | 0 | — | — | 0 | — |
| 9 | Python 用户可见文本未用 `_()` | 2 | ~34 | 1 | ~37 | `src/PyScripts`（raise/工具描述/send_error） |
| 10 | Python `@NodeDef` 字段违规 | 0 | 7 | — | 7 | `src/PyScripts/.../style_demo_nodes.py` |
| 11 | Python `setup_i18n()` 缺失/错位 | — | — | 1 | 1 | `plugins/.../DADataAnalysisGui/__init__.py` |
| 12 | Python `i18n` 基础设施缺失 | 1 | — | — | 1 | `src/PyScripts/DAWorkbench` 无 `locale/` |
| 13 | Python `# cn:` 注释缺失 | — | — | 42 | 42 | `plugins/.../DADataAnalysisGui` |
| 14 | Python 中文 docstring | 8 | — | ~50 | ~58 | `src/PyScripts`、`plugins/.../Core` |
| 15 | Python 日志含中文 | — | — | 13 | 13 | `src/PyScripts/DAWorkFlowPy`、`agent` |
| 16 | 翻译文件占位符不匹配 | — | 3 | — | 3 | `DADataAnalysisGui.po` zh_CN |
| 17 | 文档规则冲突 | — | 1 | — | 1 | `i18n.md` |
| **合计** | | **64** | **173** | **161** | **~398** | |

> 注：数字经去重；Type 1 中 2 条同时计入 Type 4（中文源 + 日志被 tr），不重复累加到合计。

---

## 二、修复计划（分阶段，按优先级）

### 阶段 0：定调与文档对齐（已完成 ✅）

| 项 | 操作 | 状态 |
|----|------|------|
| 0.1 | **确认 `da*` 宏定性**：用户已拍板——`da*` 是 UI 消息必须翻译；开发诊断式消息应改用 `q*` | ✅ 已确认 |
| 0.2 | 修正 `i18n.md` L50/L149：将 `daInfo/daWarning/daCritical` 从"不翻译"列表移除，归入"必须翻译的 UI 消息"；新增「da* 与 q* 宏的选择」决策表 | ✅ 已修正 |
| 0.3 | 同步 `AGENTS.md` 关键约束与 ANTI-PATTERNS：补选宏判断标准、新增"禁止用 da* 输出开发诊断"反模式 | ✅ 已修正 |

---

### 阶段 1：HIGH 严重度修复（中文源文本/中文 UI 无 tr）

#### 1A — C++ 中文写在 `tr()` 内（14 处）

改为英文源文本 + `//cn:中文`。

| 文件 | 行 | 当前 | 改为 |
|------|----|------|------|
| `src/DAAgent/DAAgentBridge.cpp` | 95 | `tr("Agent 进程启动超时")` | `tr("Agent process startup timed out")  //cn:Agent 进程启动超时` |
| `src/DAAgent/DAAgentBridge.cpp` | 123 | `tr("Agent 子进程启动后 %1 毫秒内未就绪...")` | `tr("Agent subprocess not ready within %1 ms, initialization may have failed, check logs")  //cn:Agent 子进程启动后 %1 毫秒内未就绪...` |
| `src/DAAgent/DAAgentBridge.cpp` | 200 | `tr("写入 agent 子进程 stdin 失败")` | `tr("Failed to write to agent subprocess stdin")  //cn:写入 agent 子进程 stdin 失败` |
| `src/DAAgent/DAAgentModule.cpp` | 252 | `tr("无法找到 Python 解释器路径...")` | `tr("Cannot find Python interpreter path, please configure it in settings")  //cn:无法找到 Python 解释器路径，请在设置页配置` |
| `src/DAAgent/DAAgentModule.cpp` | 256 | `tr("无法找到 agent_runner.py 路径: %1")` | `tr("Cannot find agent_runner.py path: %1")  //cn:无法找到 agent_runner.py 路径: %1` |
| `src/DAPyWorkFlow/DAPyWorkFlowSceneSerializer.cpp` | 101,131,160,194,206,300,332,342,350 | `DA_SERIALIZER_TR("中文...")` 共 9 处 | 全部改为 `DA_SERIALIZER_TR("English")  //cn:中文` |

> `DA_SERIALIZER_TR` = `QCoreApplication::translate("DAPyWorkFlowSceneSerializer", ...)`，本质等同 `tr()`，中文源文本同样是违规。其中 131/160 行同时是日志被 tr（Type 4），修复时一并处理：这两行的 `qWarning() << DA_SERIALIZER_TR(...)` 应改为纯英文且不翻译（日志保持英文），错误信息另存到 `mLastErrorString` 的部分用翻译。

#### 1B — C++ 中文 UI 文本未用 `tr()`（39 处）

| 文件 | 行 | 说明 |
|------|----|------|
| `src/DAGui/DANodeItemSettingWidget.cpp` | 84-107,132,166 | 14 处 `QString::fromUtf8("中文")` 作为属性面板标签 → 改 `tr("English")  //cn:中文`。如 L84 `addGroupLabel(tr("Size"))  //cn:尺寸` |
| `src/DAGui/NodeSetting/DANodeParamSettingPanel.cpp` | 42 | `QStringLiteral("无可配置参数")` → `tr("No configurable parameters")  //cn:无可配置参数` |
| `plugins/DataAnalysis/DataframeIOWorker.cpp` | 130 | `QString(u8"保存为 excel 文件")` 标题 → `tr("Save as Excel File")  //cn:保存为 excel 文件` |
| `src/tst/DAPropertyPanelDemo/main.cpp` | 24-69 | 23 处测试 demo 中文标签。**建议**：测试 demo 也应合规（演示代码是模板），改为 `tr("English")  //cn:中文` |

#### 1C — Python 中文用户可见文本（HIGH 2 处）

| 文件 | 行 | 说明 |
|------|----|------|
| `src/PyScripts/DAWorkbench/agent/agent_runner.py` | 1007 | `send_error("init 消息缺少 config 字段或格式错误")` → 英文，且因是面向 C++ UI 的错误，应保持与同文件 L1040 一致的英文 + `#cn:` 风格 |
| `src/PyScripts/DAWorkbench/agent/agent_runner.py` | 1014 | `send_error("config 缺少 base_url/api_key/model")` → 同上 |

#### 1D — Python 节点 docstring 中文（HIGH 8 处，节点 tooltip）

`src/PyScripts/DAWorkbench/DAWorkFlowPy/nodes/style_demo_nodes.py` 的 8 个 `@NodeDef` 节点类 docstring 是中文，会作为 tooltip 显示。改为英文 docstring。

---

### 阶段 2：MEDIUM 严重度修复

#### 2A — C++ `da*` 宏未用 `tr()` / 开发诊断误用 `da*`（68 处）

按阶段 0 确认的规则，68 处 `da*` 调用分两类处理：

**类别 1：开发诊断式消息（应从 `da*` 迁移到 `q*`，保持英文不翻译）**

这些消息带 `ClassName::method:` 前缀、是异常 `what()` 转储或内部状态追踪，面向开发者而非用户。按新规则**禁止用 `da*`**，改用 `qCritical()`/`qWarning()`：

| 文件 | 处数 | 当前 | 改为 |
|------|------|------|------|
| `src/DAPyWorkFlow/DAPyWorkFlowManager.cpp` | 29 | `daCritical << "DAPyWorkFlowManager::method:" << e.what();` | `qCritical() << "DAPyWorkFlowManager::method:" << e.what();` |
| `src/DAAgent/DAAgentSessionStore.cpp` | 12 | `daWarning << "DAAgentSessionStore: ..."` | `qWarning() << "DAAgentSessionStore: ..."` |
| `src/DAData/DADataPyDataFrame.cpp` | 3 | `daWarning << "DADataPyDataFrame::setValue failed: ..."` | `qWarning() << ...` |
| `src/DAPyCommonWidgets/DAPyDataframeColumnsListWidget.cpp` | 3 | `daCritical << "Exception in getting selected column:" << e.what();` | `qCritical() << ...` |
| `src/DAPyCommonWidgets/DAPyDTypeComboBox.cpp` | 1 | `daWarning << "DType not in preset list..."` | `qWarning() << ...` |
| `src/tst/MessageHandle/main.cpp` | 7 | 测试夹具 `daInfo << "business info"` | `qInfo() << ...`（测试日志不进 UI） |

**类别 2：用户可见的操作反馈（应保留 `da*` + 加 `tr()` 翻译）**

这些消息面向最终用户，是操作成功/失败反馈：

| 文件 | 处数 | 当前 | 改为 |
|------|------|------|------|
| `src/DAAxOfficeWrapper/DAAxObjectExcelWrapper.cpp` | 8 | `daWarning << "cannot open Excel file..."` 等纯英文 | 逐条判断：面向用户的（如"无法打开 Excel 文件"）→ `daWarning << tr("English")  //cn:中文`；纯技术诊断的 → 迁移 `qWarning()` |
| `src/DAFigure/DAFigureWidget.cpp` | 2 | `daCritical`/`daWarning` 操作反馈 | `da* << tr("English")  //cn:中文` |
| `src/DAPluginSupport/DAPluginManager.cpp` | 1 | `daInfo << "will ignore plugin:"`（已有 `//cn:`） | `daInfo << tr("Will ignore plugin: %1").arg(...)  //cn:将忽略插件：%1` |
| `src/DAPluginSupport/DAPluginOption.cpp` | 1 | `daInfo << "loaded plugin:"`（已有 `//cn:`） | `daInfo << tr("Loaded plugin: %1").arg(...)  //cn:已加载插件：%1` |
| `src/APP/main.cpp` | 1 | `daWarning << "Failed to set console output codepage..."` | `daWarning << tr("...")  //cn:...` |

> **逐条判断原则**：参照 `i18n.md` 「da* 与 q* 宏的选择」决策表。拿不准时问自己：这条信息用户看得懂吗？看得懂且该看 → `da*`+`tr()`；看不懂/只给开发者 → `q*` 英文。

#### 2B — C++ 日志被 `tr()` 包裹（44 处）

`qInfo/qWarning/qCritical/qDebug << tr(...)` 必须改为纯英文、去掉 `tr()`：

| 文件 | 行 | 处数 |
|------|----|------|
| `src/DAGui/DAXmlHelper.cpp` | 155,158,161,164,167,196,201,206,211,216,528,1249,1580 | 13 |
| `src/DAGui/DAZipArchive.cpp` | 308,367,382,388,398,437,564,575,591,601,780 | 11 |
| `src/APP/DAAppDataManager.cpp` | 63,69,73,84 | 4 |
| `src/APP/AppMainWindow.cpp` | 132,178,180,186 | 4 |
| `src/APP/DAAppPluginManager.cpp` | 72,232,243 | 3 |
| `src/DAPyWorkFlow/DAPyWorkFlowSceneSerializer.cpp` | 219,265 | 2 |
| `src/DAInterface/DADockingAreaInterface.cpp` | 108,114 | 2 |
| `src/DAUtils/DATextReadWriter.cpp` | 179 | 1 |
| `src/APP/DAAppUI.cpp` | 81 | 1 |
| `src/APP/DAPluginManagerDialog.cpp` | 52 | 1 |
| `plugins/DataAnalysis/DataframeIOWorker.cpp` | 223-228 | 1（间接：tr 结果流入 qInfo） |

#### 2C — C++ 日志字符串含中文（14 处）

`qDebug/qWarning/DA_WF_DBG("中文")` 改为英文：

| 文件 | 行 |
|------|----|
| `src/DAPyWorkFlow/DAPyWorkFlowExecutor.cpp` | 78,91,114,127,135 |
| `src/DAPyWorkFlow/DAPyWorkFlowManager.cpp` | 134,137,192,470 |
| `src/DAPyWorkFlow/DAPyWorkFlow.cpp` | 100,184 |
| `src/DAPyWorkFlow/DAPyNodeFactory.cpp` | 108 |
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 284 |
| `src/APP/DADumpCapture.h` | 156 |

#### 2D — Python 中文 raise/工具描述/send_error（MEDIUM ~34 处）

| 文件 | 行 | 处数 | 处理 |
|------|----|------|------|
| `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py` | 84,99,120,123,126,153,155,166,171,185,222,332,451 | 13 | raise 消息改英文；若面向用户则 `_()` 包裹 |
| `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py` | 68,70,72,74,76 | 5 | 同上 |
| `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` | 104,322,335 | 3 | 同上 |
| `src/PyScripts/DAWorkbench/DAWorkFlowPy/serializer.py` | 177,181,411,419 | 4 | 同上 |
| `src/PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py` | 133 | 1 | 同上 |
| `src/PyScripts/DAWorkbench/agent/agent_runner.py` | 470 | 1 | raise 消息改英文 |
| `src/PyScripts/DAWorkbench/agent/agent_runner.py` | 325,331,336,340 | 4 | ask_user 工具 schema description 改英文 |
| `src/PyScripts/DAWorkbench/DAPyBase/dataframe.py` | 301 | 1 | raise 改英文 |

> **判断点**：raise 异常消息是否需要 `_()` 包裹？异常文本若经 C++ 捕获后显示给用户，应翻译；若仅写日志，保持英文。需逐条判断。**默认建议**：改英文源文本，面向用户的加 `_()`。

#### 2E — Python `@NodeDef(category=)` 未用 `_()`（MEDIUM 7 处）

`src/PyScripts/DAWorkbench/DAWorkFlowPy/nodes/style_demo_nodes.py` L27,53,73,98,121,144,169 的 `category="Style Demo"` 改为 `category=_("Style Demo")  # cn:样式演示`。

#### 2F — 文件过滤器未 i18n 化（MEDIUM 1 处）

`plugins/DataAnalysis/DataframeIOWorker.cpp:132` `QString(u8"excel (*.xlsx)")` 改为 `tr("Excel") + " (*.xlsx)"  //cn:Excel`。

#### 2G — 翻译文件占位符不匹配（MEDIUM 3 处）

`plugins/DataAnalysis/PyScripts/DADataAnalysisGui/i18n/locale/zh_CN/LC_MESSAGES/DADataAnalysisGui.po`：

| 行 | msgid | 当前 msgstr（错） | 应改为 |
|----|-------|-------------------|--------|
| 363 | `Failed to replace values: {error}` | `导出Excel文件失败: {err1}\n{err2}` | `替换值失败: {error}` |
| 483 | `Failed to process outliers: {error}` | `导出Excel文件失败: {err1}\n{err2}` | `处理异常值失败: {error}` |
| 598 | `Failed to transform data: {error}` | `导出dataframe失败: {err1}\n{err2}` | `数据变换失败: {error}` |

这 3 条是 copy-paste 自无关的 Excel 导出错误，语义也错。修复后去掉 `fuzzy` 标记，重新 `msgfmt` 编译 `.mo`。

---

### 阶段 3：LOW 严重度修复 + 基础设施补全

#### 3A — Python `i18n` 基础设施缺失（HIGH 但属基础设施，列此阶段）

`src/PyScripts/DAWorkbench/` 有 `i18n/core.py` 和正确的 `setup_i18n()` 调用，但**没有 `locale/` 目录和 `.po/.mo` 文件**——导致所有 `_()` 调用返回英文原文本，`# cn:` 注释完全失效。

**操作**：参照 `plugins/DataAnalysis/PyScripts/DADataAnalysisGui/i18n/` 标杆，为 `DAWorkbench` 包创建：
- `i18n/locale/DAWorkbench.pot`
- `i18n/locale/zh_CN/LC_MESSAGES/DAWorkbench.po`
- `i18n/locale/en/LC_MESSAGES/DAWorkbench.po`
- 用 `xgettext` 提取 `thread_status_manager.py` 等 8 处 `_()` 调用
- 补 `update_po.py` / `generate_i18n.py` / `update-i18n.sh`

#### 3B — Python `setup_i18n()` 错位（LOW 1 处）

`plugins/DataAnalysis/PyScripts/DADataAnalysisGui/__init__.py`：`setup_i18n()` 在 L16，但模块导入在 L8。虽然 `_()` 只在函数体内调用无运行时风险，但应调整顺序：`setup_i18n()` 移到导入之前。

#### 3C — Python `# cn:` 注释缺失（LOW 42 处）

`plugins/DataAnalysis/PyScripts/DADataAnalysisGui/`：
- `dataframe_cleaner.py`：31 处 `_("English")` 缺 `# cn:中文`
- `dataframe_io.py`：11 处同上

补全 `# cn:` 注释，使 `update_po.py` 能自动填充 `.po`。

#### 3D — C++ `tr()` 缺 `//cn:` 注释（LOW 13 处）

| 文件 | 行 |
|------|----|
| `src/DAAgent/DAAgentModule.cpp` | 167,171 |
| `src/DAAgent/DAAgentBridge.cpp` | 279,476 |
| `src/APP/DAAppController.cpp` | 3202,3237 |
| `src/APP/SettingPages/DAAgentSettingsWidget.cpp` | 105,257,258,259 |
| `src/APP/SettingPages/DASettingPageLog.cpp` | 27 |
| `src/APP/SettingPages/DASettingPageAdvanced.cpp` | 20 |
| `src/DAGui/Dialog/DADialogAgentSessionManager.cpp` | 223 |

补 `//cn:中文` 注释。

#### 3E — C++ 空 `//cn:` 注释（MEDIUM 2 处）

| 文件 | 行 | 问题 |
|------|----|------|
| `plugins/DataAnalysis/Dialogs/DataFrameEvalDatasDialog.cpp` | 73 | `// cn:` 后无中文，翻译写在后续普通注释里 |
| `plugins/DataAnalysis/Dialogs/DataFrameQueryDatasDialog.cpp` | 45 | 同上 |

将后续注释的中文合并到 `// cn:` 行内。

#### 3F — Python 中文 docstring（LOW ~50 处）

`src/PyScripts/DAWorkbench/` 下 `DAWorkFlowPy/`、`DAPyBase/`、`agent/` 的模块/类/方法 docstring 多为中文。规则要求节点类 docstring 改英文（tooltip），非节点库 docstring 严重度低。建议批量改英文（开发文档用英文更一致）。

涉及文件：`__init__.py`（多个）、`workflow.py`、`_debug.py`、`da_logger.py`、`app_wrapper.py`、`form_builder.py`、`form_spec.py`、`dataframe.py`、`thread_status_manager.py`、`agent_runner.py`、`context_manager.py`，以及 `plugins/.../DADataAnalysisCore/{operations,io,cleaning}.py` 等。

#### 3G — Python 日志含中文（LOW 13 处）

`src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py`（8 处 `logger.info/debug/warning`）、`agent/agent_runner.py`（2 处 `logger.warning`）、`DAPyBase/utils.py`（1 处测试 print）、`src/i18n/merge_translations.py`（7 处 print 状态输出）、`src/PyScripts/tst.py`（2 处）。

日志改英文。`merge_translations.py`、`tst.py` 是开发工具/测试脚本，优先级最低。

#### 3H — 补全 `.po` 翻译 + 修 `.po` 头

- `DADataAnalysisGui.po` zh_CN 有 120 条空 `msgstr` + 22 条 `fuzzy`（仅 36% 翻译）。用 `update_po.py` 结合 `# cn:` 注释批量填充。
- `DADataAnalysisGui.po` en 的 `Project-Id-Version: PACKAGE VERSION` 改为 `DADataAnalysisGui 1.0`；zh_CN 的 `DADataAnalysis` 改为 `DADataAnalysisGui`（cosmetic）。

#### 3I — 补 `.po` 占位符校验工具

`scripts/check_translations.py` 目前只校验 `.ts`，不校验 `.po`。扩展它（或 `translation_validate.py`）解析 `.po`，对 `{brace}`/`%` 占位符做 source↔translation 一致性校验，接入 CI。

---

## 三、修复后的收尾流程

1. **C++ 侧**：改完 `tr()` 后运行 `cmake --build <build> --target update_translations` 更新 `.ts`，再运行 `scripts/apply_cn_to_ts.py` 用 `//cn:` 注释填充翻译，最后 `python scripts/check_translations.py` 校验通过。
2. **Python 侧**：每个包运行各自的 `update-i18n.sh`（或 `xgettext`+`msgmerge`+`msgfmt`）重新生成 `.pot/.po/.mo`。
3. **构建验证**：`.\scripts\build.ps1 -Full` 确认无编译错误。
4. **运行验证**：启动程序切换中英文，检查 UI 文本、日志面板、文件对话框、节点 tooltip。

---

## 四、违规零项的确认（干净区域）

以下区域经审计确认**无违规**，可作为标杆参考：

- **C++ `.ts` 文件**：占位符/通配符/`cn:` 泄漏/中文 source 全部 0 违规，`check_translations.py` 通过。
- **Python `@NodeDef(name=)`**：所有节点 name 均为英文字面量，无翻译（合规）。
- **Python `@NodeDef(category=)`/`Parameter(description=)`/`Input/Output(description=)`**：`DASystemNodes`、`DADataAnalysisNodes` 全部正确使用 `_()` + `# cn:`（合规）。
- **Python `_()` 内中文源文本**：全项目 0 处（所有 `_()` 均英文源）。
- **Python 节点包 `setup_i18n()` 顺序**：`DASystemNodes`、`DADataAnalysisNodes` 正确在导入前调用（合规）。
- **C++ 文件过滤器通配符在 `tr()` 内**：0 处（合规）。
- **`.ui` 文件**：所有 `<string>` 均英文源文本（合规）。
- **插件 `retranslateUi()`/`retranslate()`**：所有插件均已实现（合规）。

---

## 五、执行建议

| 优先级 | 阶段 | 工作量估计 | 依赖 |
|--------|------|-----------|------|
| 🔴 最高 | 阶段 0（定调 + 文档对齐） | 小 | 无，但需你确认 §〇 |
| 🔴 最高 | 阶段 1（HIGH：中文源/中文 UI 无 tr） | 中（~67 处，集中在少数文件） | 阶段 0 |
| 🟡 中 | 阶段 2（MEDIUM：da* 无 tr/误用、日志被 tr、Python raise） | 大（~170 处） | 阶段 0 已完成 ✅ |
| 🟢 低 | 阶段 3（LOW：cn 注释缺失、docstring、基础设施补全） | 中（~130 处 + 建目录） | 阶段 1/2 完成 |

**建议分批提交**：每个阶段一个 git commit，提交信息标注 `i18n: 阶段X - <内容>`。阶段 1 和 2B/2C 可独立并行推进（不互相依赖）。

---

*本报告由 5 个并行审计 agent 产出，覆盖 C++ src/、C++ plugins/、Python plugins/、Python src/PyScripts、翻译文件五大区域。*
