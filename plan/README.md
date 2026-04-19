# AI Agent 升级计划文档总览

本目录包含 data-workbench 项目从「工作流驱动数据分析软件」升级为「AI Agent 驱动数据分析软件」的详细实施计划。

## 📋 计划文档列表

| 阶段 | 文档 | 周期 | 优先级 | 状态 |
|------|------|------|--------|------|
| Phase 0 | [00-pre-research.md](./00-pre-research.md) | 2 周 | 🔴 最高 | 预研 |
| Phase 1 | [01-foundation-adaptation.md](./01-foundation-adaptation.md) | 4 周 | 🔴 最高 | 基础 |
| Phase 2 | [02-agent-orchestration.md](./02-agent-orchestration.md) | 4-5 周 | 🔴 最高 | 编排 |
| Phase 3 | [03-ai-framework-integration.md](./03-ai-framework-integration.md) | 3-4 周 | 🔴 最高 | 集成 |
| Phase 4 | [04-experience-and-demo.md](./04-experience-and-demo.md) | 2 周 | 🔴 最高 | 体验 |
| Phase 5 | [05-security-measures.md](./05-security-measures.md) | 2 周 | 🔴 最高 | 安全 |

**总周期**: 15-17 周（约 4 个月）

---

## 🎯 项目概述

### 升级目标

将 data-workbench 从「工作流驱动数据分析软件」升级为「AI Agent 驱动数据分析平台」，实现：

- 🤖 **AI Agent 可视化编排**：通过拖拽方式搭建智能体工作流
- 🔗 **主流 AI 框架兼容**：原生支持 crewAI、LangChain、LangGraph
- 🐍 **Python 双向交互**：开放全部软件 API 给 Python 调用
- 📊 **Agent 运行状态监控**：实时查看 Agent 执行进度、日志、调用链路
- 🔌 **Agent 插件市场**：支持自定义开发 Agent 插件

### 核心模块

```
┌─────────────────────────────────────────────────────────────┐
│                    AI Agent 驱动数据分析平台                  │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ Agent 编排   │  │ WorkFlow    │  │ Data        │         │
│  │ 可视化编排   │  │ 引擎        │  │ 数据处理    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ Chart       │  │ AI Framework│  │ Security    │         │
│  │ 可视化      │  │ Integration │  │ 安全防护    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

---

## 📅 阶段详情

### Phase 0: 前置预研（2 周）

**核心目标**: 解决所有阻断性问题，确定技术方案

**关键任务**:
- 0.1 Python 版本升级评估（3.7+ → 3.10+）
- 0.2 AI 依赖冲突预检（crewAI, LangChain, LangGraph）
- 0.3 异步架构设计（Qt 事件循环 ↔ asyncio 桥接）
- 0.4 现有代码兼容性验证

**验收标准**:
- [ ] Python 3.10 版本验证通过
- [ ] 所有 AI 依赖可正常导入
- [ ] 异步架构设计文档完成
- [ ] 现有功能在 Python 3.10 下兼容性验证通过

**关键风险**:
- 🔴 Python >=3.10 硬性要求与现有 3.7+ 兼容性冲突
- 🔴 GIL+asyncio 线程安全问题

👉 详细计划：[00-pre-research.md](./00-pre-research.md)

---

### Phase 1: 基础能力适配（4 周）

**核心目标**: 完成 Python API 全量暴露、工作流引擎异步重构、Agent 基础模型适配

**关键任务**:
- 1.1 Python API 接口设计
- 1.2 pybind11 Python API 绑定实现
- 1.3 工作流引擎异步重构
- 1.4 Agent 节点基础模型开发
- 1.5 嵌入式 Python 环境优化

**验收标准**:
- [ ] Python 可导入 `da` 包并调用核心功能
- [ ] DAAbstractNode 支持 `execAsync()` 方法
- [ ] DAWorkFlowExecuter 支持异步工作流执行
- [ ] DAAgentNode 可创建并执行
- [ ] Python 虚拟环境可正常安装 AI 依赖

**交付物**:
- `src/DAPyBindQt/daworkflow_module.cpp` - WorkFlow Python 绑定
- `src/DAPyBindQt/dafigure_module.cpp` - Figure Python 绑定
- `src/DAPyBindQt/dadata_module.cpp` - Data Python 绑定
- `src/DAWorkFlow/DAAgentNode.h/cpp` - Agent 节点实现

👉 详细计划：[01-foundation-adaptation.md](./01-foundation-adaptation.md)

---

### Phase 2: Agent 编排能力建设（4-5 周）

**核心目标**: 实现 Agent 可视化拖拽编排、运行状态实时监控、安全机制

**关键任务**:
- 2.1 Agent 节点 UI 组件开发
- 2.2 Agent 属性配置面板开发
- 2.3 异步调度引擎开发
- 2.4 运行状态监控面板开发
- 2.5 安全机制开发
- 2.6 Agent 工作流保存/加载功能

**验收标准**:
- [ ] Agent 节点可在编辑器中拖拽创建
- [ ] Agent 属性面板可配置所有参数
- [ ] 异步工作流可正常执行，UI 无阻塞
- [ ] 监控面板实时显示执行状态
- [ ] API 密钥存储在 OS 密钥管理器
- [ ] 工作流可保存/加载，密钥不明文存储

**交付物**:
- `src/DAWorkFlow/DAAgentNodeGraphicsItem.h/cpp` - Agent 节点 UI
- `src/DAWorkFlow/DAAgentConfigWidget.h/cpp` - Agent 配置面板
- `src/DAWorkFlow/DAAgentMonitorWidget.h/cpp` - 监控面板
- `src/DAUtils/DASecretManager.h/cpp` - 密钥管理器

👉 详细计划：[02-agent-orchestration.md](./02-agent-orchestration.md)

---

### Phase 3: AI 框架集成（3-4 周）

**核心目标**: 完成 crewAI、LangChain/LangGraph 双框架对接

**关键任务**:
- 3.1 crewAI 框架集成
- 3.2 LangChain & LangGraph 集成
- 3.3 LLM 配置中心开发
- 3.4 工具调用能力对接
- 3.5 双模式执行桥接开发

**验收标准**:
- [ ] crewAI 节点可创建并执行
- [ ] LangChain 节点可创建并执行
- [ ] LangGraph 节点可映射工作流并执行
- [ ] LLM 配置中心可管理多提供商
- [ ] 工具调用可执行数据处理和可视化
- [ ] 同步/异步/混合执行模式可正常切换

**交付物**:
- `src/DAWorkFlow/DACrewAINode.h/cpp` - crewAI 节点
- `src/DAWorkFlow/DALangChainNode.h/cpp` - LangChain 节点
- `src/DAWorkFlow/DALangGraphNode.h/cpp` - LangGraph 节点
- `src/DAUtils/DALLMConfigCenter.h/cpp` - LLM 配置中心

👉 详细计划：[03-ai-framework-integration.md](./03-ai-framework-integration.md)

---

### Phase 4: 体验优化与 Demo（2 周）

**核心目标**: 优化用户体验，提供可演示的示例

**关键任务**:
- 4.1 Agent 示例模板库开发
- 4.2 全流程 Demo 开发
- 4.3 文档更新
- 4.4 兼容性测试与优化

**验收标准**:
- [ ] Agent 模板库包含至少 5 个模板
- [ ] 全流程 Demo 可正常运行
- [ ] 用户指南、开发指南、安全指南完成
- [ ] Qt5/Qt6 兼容性测试通过
- [ ] 所有自动化测试通过

**交付物**:
- `resources/templates/agents/` - Agent 模板（5+ 个）
- `resources/templates/workflows/` - 工作流模板（3+ 个）
- `demos/auto_analysis/` - 完整 Demo
- `docs/zh/user-guide/ai-agent/` - 用户指南

👉 详细计划：[04-experience-and-demo.md](./04-experience-and-demo.md)

---

### Phase 5: 安全措施（2 周，与 Phase 2 并行）

**核心目标**: 实现完整的安全防护体系

**关键任务**:
- 5.1 密钥安全存储（Windows Credential Manager / macOS Keychain / Linux Secret Service）
- 5.2 Python 执行沙箱
- 5.3 数据脱敏
- 5.4 网络安全
- 5.5 审计日志

**验收标准**:
- [ ] API 密钥存储在 OS 密钥管理器
- [ ] Python 沙箱阻止所有危险操作
- [ ] 日志脱敏覆盖所有敏感模式
- [ ] 网络访问限制生效
- [ ] 审计日志记录所有关键事件

**交付物**:
- `src/DAUtils/DASecretManager_*.cpp` - 跨平台密钥管理
- `src/DAPyScripts/sandbox_restricted.py` - Python 沙箱
- `src/DAUtils/DASensitiveDataDetector.h/cpp` - 敏感数据检测
- `src/DAUtils/DAAuditLogger.h/cpp` - 审计日志器

👉 详细计划：[05-security-measures.md](./05-security-measures.md)

---

## 🔗 依赖关系图

```
                    ┌─────────────┐
                    │  Phase 0    │
                    │  预研阶段   │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │  Phase 1    │
                    │  基础适配   │
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
              ▼            ▼            ▼
       ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
       │  Phase 2    │ │  Phase 3    │ │  Phase 5    │
       │  编排能力   │ │  AI 框架    │ │  安全措施   │
       └──────┬──────┘ └──────┬──────┘ └──────┬──────┘
              │               │                │
              └───────────────┼────────────────┘
                              │
                              ▼
                       ┌─────────────┐
                       │  Phase 4    │
                       │  体验优化   │
                       └─────────────┘
```

---

## 📊 时间规划总览

| 阶段 | 周次 | 主要任务 |
|------|------|----------|
| Phase 0 | Week 1-2 | 预研、架构设计、依赖验证 |
| Phase 1 | Week 3-6 | Python 绑定、异步引擎、Agent 模型 |
| Phase 2 | Week 7-11 | UI 组件、监控面板、密钥管理 |
| Phase 3 | Week 12-15 | AI 框架集成、工具调用 |
| Phase 4 | Week 16-17 | 模板库、Demo、文档 |
| Phase 5 | Week 7-8 | 安全措施（与 Phase 2 并行） |

**关键里程碑**:
- Week 2: 预研完成，技术方案确定
- Week 6: 基础能力就绪，可开始 Agent 开发
- Week 11: Agent 编排完成，可演示基本功能
- Week 15: AI 框架集成完成
- Week 17: 项目发布

---

## ⚠️ 关键风险汇总

| 风险 ID | 风险描述 | 严重程度 | 缓解方案 | 相关阶段 |
|---------|----------|----------|----------|----------|
| R10 | Python >=3.10 硬性要求与现有 3.7+ 兼容性冲突 | 🔴 致命 | 前置升级嵌入式 Python 到 3.10+ | Phase 0 |
| R1 | GIL+asyncio 线程安全问题 | 🔴 致命 | 设计双事件循环桥接方案 | Phase 0, 1 |
| R3 | 同步执行引擎与异步 Agent 模型不匹配 | 🔴 致命 | 扩展 DAAbstractNode 支持 execAsync() | Phase 1 |
| R4 | API 密钥明文存储风险 | 🔴 致命 | 使用 OS 级密钥管理器 | Phase 2, 5 |
| R2 | crewAI/LangChain 依赖冲突 | 🟡 高风险 | 前置进行依赖预检 | Phase 0 |
| R5 | Python 执行无沙箱 | 🟡 高风险 | 限制危险系统调用 | Phase 5 |
| R6 | 异步执行 UI 阻塞 | 🟡 高风险 | Agent 执行在独立 Python 线程 | Phase 2 |

---

## 📚 相关文档

- **主计划**: `.sisyphus/plans/ai-agent-upgrade-plan.md`
- **学习笔记**: `.sisyphus/notepads/ai-agent-upgrade-plan/learnings.md`
- **项目 README**: `README.md`
- **AGENTS.md**: `AGENTS.md` - 项目开发指南

---

## 🚀 快速开始

### 开发者

1. 阅读主计划了解整体目标
2. 根据当前阶段阅读对应的详细计划
3. 按照计划中的「子步骤」逐步实施
4. 完成后更新验收清单

### 审查者

1. 查看各阶段的「验收标准」
2. 核对「交付物」清单
3. 审查代码是否符合计划设计

### 项目管理者

1. 跟踪各阶段进度
2. 关注关键风险缓解情况
3. 确保阶段间依赖满足

---

## 📝 更新记录

| 日期 | 版本 | 更新内容 | 作者 |
|------|------|----------|------|
| 2026-04-19 | v1.0 | 初始版本，完成所有详细计划文档 | AI Agent |

---

**最后更新**: 2026-04-19
