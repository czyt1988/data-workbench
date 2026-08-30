# 更新日志

本页面记录 DAWorkBench 项目的版本变更历史，包括新功能、改进、修复和破坏性变更。

## 主要功能特性

- ✅ **版本化记录**：按版本号组织所有变更
- ✅ **变更分类**：新功能、改进、修复、破坏性变更清晰标注
- ✅ **语义化版本**：遵循语义化版本规范（主.次.补丁）

---

## v0.0.6（开发中）

### 新功能

- 🟢 **Agent / AI 分析子系统**：新增 `DAAgent` 模块（`src/DAAgent/`），通过 `DAAgentInterface` 对外暴露：
    - 多供应商多模型 LLM 配置（`getProviders`/`setProviders`/`getActiveProvider`/`setActiveModel`/`getAvailableModels`），运行中热切换模型不丢会话状态
    - 提示词库（`registerBuiltinAgent`/`runAgent`/`agentPromptOps`），Ribbon「AI 分析」标签页 gallery 一键管理/执行
    - 工具调用：`registerTool` 注册 `DAAbstractAgentTool`；`DAAgentTools` 插件内置 20 个工具（数据查询、图表生成、文件读写、代码执行等）
    - 会话持久化（`createSession`/`switchSession`/`deleteSession`/`listSessions`/`exportActiveSessions`/`loadSessionsFromProject`），随 `.dapro` 存为 `agent_sessions/<id>.jsonl`
    - 子进程架构：独立 Python 子进程（LangGraph）驱动推理与工具循环；预启动（`auto_prestart`，默认开）、3 态 UI（启动中/就绪/忙碌）、累计 token 统计（`tokenUsageUpdated`）、看门狗与空闲计时（`inactivity_timeout_sec`）、`recursion_limit` 限制
- 🟢 **图表嵌套停靠**：`DAChartOperateWidget` 由 `QTabWidget` 迁移到 ADS 嵌套停靠区，可与其它 Dock 统一编组 / 浮动 / 记忆布局
- 🟢 **图表文件重组**：`DAChart*` 系列源文件重新组织
- 🟢 **插件**：新增 `DASystemNodes`（系统流控制节点）与 `DAAgentTools`（Agent 工具，20 个）插件
- 🟢 **Markdown 渲染**：新增 `DAMarkdownView` 通用 Markdown 渲染控件（`src/DAGui/MarkdownView/`）
- 🟢 **AI 分析 Ribbon 标签**：新增「AI 分析」标签页（`mCategoryAgent` / `mAgentGallery` / `populateAgentGallery`）
- 🟢 **Agent 权限模式**：AI 分析新增权限模式——全自动 / 智能判定 / 每次询问三档可选，聊天工具栏随时切换；**默认模式为全自动（yolo）**，未显式配置时静默生效。文件写入与代码执行等危险操作执行前自动拦截或弹审批卡征求确认；系统目录写入在任何模式下禁止。智能判定模式下代码内容先经静态危险模式规则、再经可选判官模型自动放行/拒绝，拿不准时降级询问用户。设置页可自定义路径规则、代码危险模式清单与工具风险分级；会话内切入全自动需二次确认，跨重启时仅对用户显式设置过的全自动弹启动确认卡（默认全自动不弹卡）
- 🟢 **子 Agent 委派**：AI 分析支持子 Agent——主 AI 可将探索调研类任务委派给多个子 Agent 并行执行并汇总结论（如“并行探索工作区每个数据集的结构与数据质量”），聊天界面实时展示各子任务进度；内置只读探索子 Agent `explore`，任意权限模式下零打扰。Agent 管理新增「子 Agent」页签，可新建/编辑/删除子 Agent（自定义描述、可用工具范围与指令）；子 Agent 的写文件、执行代码等操作与主 AI 走同一权限管控，需要批准时审批卡会标注来源子 Agent，任务超时或被停止后未决的审批自动撤销。设置页新增子 Agent 单任务超时与推理步数上限

### 改进

- 🔵 **国际化加固**：翻译工作流加固，CI 校验，确立 `da*` / `q*` 日志宏命名规则
- 🔵 **构建**：移除 `DA_ENABLE_PYTHON` 开关——Python 成为硬依赖；`DAConfigs.h` 无条件生成

### 破坏性变更

- 🔴 **`DAAgentInterface` 会话 API**：新增会话管理接口（`createSession` / `switchSession` / `deleteSession` 等），见 `src/DAAgent/DAAgentInterface.h:75` 注释「破坏性接口变更，插件需重编译」——依赖该接口的插件需重新编译
- 🔴 **`DAAgentInterface` 权限 API**：新增权限层接口（`getPermissionConfig` / `setPermissionConfig` / `getPermissionMode` / `setPermissionMode` / `sendToolApproval` / `setScriptWorkspaceDir`）与审批/模式信号（`agentToolApprovalRequest` / `agentToolApprovalDismissed` / `permissionModeChanged`）——依赖该接口的插件需重新编译
- 🔴 **`DAAgentInterface` 子 Agent API**：新增子 agent 管理接口（`registerBuiltinSubagent` / `subagentDefinitions` / `saveSubagent` / `deleteSubagent` / `registeredToolNames`）与子 agent 信号（`agentSubagentProgress` / `subagentListChanged`）——依赖该接口的插件需重新编译

---

## v0.0.5（当前开发版本）

### 新功能

- **表格样式系统**：新增 `DATableStyleManager` 三层存储架构，支持单元格级条件格式
- **表格样式序列化**：表格样式可序列化到 `table-styles.xml`，随工程文件保存
- **表格样式操作**：新增样式分类、Action 绑定和控制器集成
- **DataFrame Widget 枚举**：`DADataOperateWidget` 新增 `getAllDataFrameWidgets()` 方法

### 改进

- **国际化完善**：审查并修复所有 `tr()` 英文文本和 `//cn:` 注释
- **图表可见性同步**：图表管理树视图同步可见性状态
- **撤销命令优化**：`clearStyleAll` 现在可撤销，空撤销命令增加保护

### 修复

- **图表上下文菜单**：修复可见性双重切换问题
- **消息处理器**：修复应用退出时的 use-after-free 问题
- **Python 模块路径**：修正 Python 模块路径并统一代码格式

---

## v0.0.4

### 新功能

- **统一表单系统**：基于 `DAFormSpec` 的统一节点参数面板（v2 schema）
- **声明式联动规则**：`DAFormRuleEvaluator` 实现 visible/enabled/required 联动
- **表单编辑器注册表**：`DAFormEditorRegistry` 支持 11 种字段类型编辑器
- **属性表单容器**：`DAPropertyFormWidget` 驱动 DAFormSpec + 规则联动

### 改进

- **节点设置面板重构**：`NodeSetting/` 模块采用三层架构（基类→面板→具体面板 + 单例工厂 + QStackedWidget 调度器）
- **废弃旧 API**：废弃 `DAPyNodeConfigDialog` / `DAPyNodeWidget`，统一使用 `DAPropertyFormWidget`

---

## v0.0.3

### 新功能

- **完善插件系统架构**：插件生命周期管理、节点工厂代理
- **工作流执行回调**：`DAPyWorkFlowManager` 执行完成信号通知
- **优化节点连接机制**：DAG 模型验证和拓扑排序
- **改进数据序列化**：DAData 隐式共享和 ZIP 工程文件格式

---

## v0.0.2

### 新功能

- **基础工作流功能**：有向图工作流编辑、节点拖放创建
- **简单数据处理节点**：CSV 读取、DataFrame 过滤等基础节点
- **基础图表绘制**：基于 Qwt 的曲线图、散点图

---

## v0.0.1

### 新功能

- **项目初始化**：项目骨架搭建
- **基础框架**：五层架构设计（基础层→功能层→界面层→接口层→应用层）
- **插件系统原型**：`DAAbstractPlugin` 基类和 `DAPluginManager` 管理器

---

## 变更类型说明

| 标记 | 说明 |
|------|------|
| 🟢 **新功能** | 新增的功能特性 |
| 🔵 **改进** | 对现有功能的优化和增强 |
| 🟡 **修复** | Bug 修复 |
| 🔴 **破坏性变更** | 不兼容的 API 变更，升级时需注意 |

---

## 相关文档

- [配置文件说明](../use-guide/configuration.md) — 配置文件格式详解
- [术语表](./glossary.md) — 项目核心术语
- [CMake 辅助宏](../build/large-cmake-project-guide.md#工程封装的-cmake-辅助宏) — 插件构建宏说明
- [贡献指南](./contribution-guide.md) — 代码贡献流程
- [开发指引](../dev-guide/developer-guide.md) — 开发者入门指南
