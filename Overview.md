# DAWorkBench 项目总览（AI Agent 导航索引）

> **用途**: 本文件为 AI Agent 提供项目快速导航，帮助理解项目结构、定位核心代码、快速查阅文档。

---

## 项目简介

DAWorkBench 是一个基于工作流的数据分析平台，集成 pandas 数据处理能力，实现交互式数据可视化，支持生成论文级别图片。项目采用 C++17 + Qt 开发，通过 pybind11 绑定 Python 实现数据处理功能。

**核心价值**：
- 工作流驱动自动化处理重复性数据
- GUI 封装 pandas 核心功能，无需编程即可操作
- 交互式图表编辑，生成论文级别矢量图
- 插件化架构，易于扩展自定义功能

---

## 核心模块

| 模块 | 目录 | 职责 | 关键文件 |
|------|------|------|----------|
| **DAData** | `src/DAData/` | 数据管理、数据结构定义 | `DAData.h`, `DADataManager.h` |
| **DAFigure** | `src/DAFigure/` | 图表绘制、编辑、导出 | `DAFigure.h`, `DAChart.h` |
| **DAGraphicsView** | `src/DAGraphicsView/` | 图形视图、节点编辑 | `DAGraphicsNodeItem.h`, `DAGraphicsLinkItem.h` |
| **DAGui** | `src/DAGui/` | GUI 组件、界面逻辑 | `DAMainWindow.h`, `DAWorkbenchOper.h` |
| **DAWorkFlow** | `src/DAWorkFlow/` | 工作流引擎、节点执行 | `DAWorkFlow.h`, `DAAbstractNode.h` |
| **DAPluginSupport** | `src/DAPluginSupport/` | 插件加载、生命周期管理 | `DAPluginManager.h`, `DAAbstractPlugin.h` |
| **DAInterface** | `src/DAInterface/` | 核心接口定义 | `DACoreInterface.h`, `DAUIInterface.h` |
| **APP** | `src/APP/` | 应用主程序入口 | `main.cpp`, `DAApp.h` |

---

## 目录结构

```
data-workbench/
├── src/                    # 源码目录
│   ├── APP/                # 应用主程序
│   ├── DACommonWidgets/    # 通用 UI 组件
│   ├── DAData/             # 数据处理模块
│   ├── DAFigure/           # 图表模块
│   ├── DAGraphicsView/     # 图形视图模块
│   ├── DAGui/              # GUI 模块
│   ├── DAInterface/        # 核心接口定义
│   ├── DAPluginSupport/    # 插件支持模块
│   ├── DAUtils/            # 工具函数库
│   ├── DAWorkFlow/         # 工作流模块
│   └── 3rdparty/           # 第三方库
├── docs/                   # 项目文档
│   ├── doc-writing-guide.md # 文档撰写规范
│   └── zh/                 # 中文文档
│       ├── build/          # 构建文档
│       ├── dev-guide/      # 开发指南
│       └── use-guide/      # 用户指南
├── plugins/                # 插件目录
│   └── DataAnalysis/       # 数据分析示例插件
├── cmake/                  # CMake 配置
├── AGENTS.md               # AI Agent 开发指南
├── CMakeLists.txt          # 主 CMake 文件
└── requirements.txt        # Python 依赖
```

---

## 文档导航

### 开发指南（dev-guide）

| 文档 | 说明 |
|------|------|
| [workflow.md](docs/zh/dev-guide/workflow.md) | 工作流系统核心概念和架构 |
| [workflow-lifecycle.md](docs/zh/dev-guide/workflow-lifecycle.md) | 工作流节点生命周期管理 |
| [plugin-project-create.md](docs/zh/dev-guide/plugin-project-create.md) | 创建插件项目完整指南 |
| [plugin-architecture.md](docs/zh/dev-guide/plugin-architecture.md) | 插件架构设计详解 |
| [plugin-module.md](docs/zh/dev-guide/plugin-module.md) | DAPluginSupport 模块说明 |
| [plugins-interfaces.md](docs/zh/dev-guide/plugins-interfaces.md) | 插件接口体系 |
| [coding-standard.md](docs/zh/dev-guide/coding-standard.md) | 编码规范和命名约定 |
| [module-dependency.md](docs/zh/dev-guide/module-dependency.md) | 模块依赖关系和架构层次 |
| [data-module.md](docs/zh/dev-guide/data-module.md) | DAData 数据模块详解 |
| [python-in-cpp.md](docs/zh/dev-guide/python-in-cpp.md) | Python/C++ 集成和 pybind11 使用 |
| [figure-abstract.md](docs/zh/dev-guide/figure-abstract.md) | DAFigure 图表抽象层 |
| [interface-module.md](docs/zh/dev-guide/interface-module.md) | DAInterface 接口模块 |
| [scalable-graphic-module.md](docs/zh/dev-guide/scalable-graphic-module.md) | 可缩放图形模块 |
| [project-serialization-architecture.md](docs/zh/dev-guide/project-serialization-architecture.md) | 项目序列化架构 |
| [settingwidget-standard.md](docs/zh/dev-guide/settingwidget-standard.md) | 设置控件规范 |
| [third-party-manage.md](docs/zh/dev-guide/third-party-manage.md) | 第三方库管理 |

### 构建文档（build）

| 文档 | 说明 |
|------|------|
| [build-instructions.md](docs/zh/build/build-instructions.md) | 完整构建指南 |
| [third-party-build.md](docs/zh/build/third-party-build.md) | 第三方库编译 |
| [python-environment.md](docs/zh/build/python-environment.md) | Python 环境配置 |
| [common-build-errors.md](docs/zh/build/common-build-errors.md) | 常见构建错误解决 |
| [plugin-build.md](docs/zh/build/plugin-build.md) | 插件构建说明 |
| [main-program-build.md](docs/zh/build/main-program-build.md) | 主程序构建详解 |

### 用户指南（use-guide）

| 文档 | 说明 |
|------|------|
| [index.md](docs/zh/use-guide/index.md) | 用户指南入口 |
| [command-line-arguments.md](docs/zh/use-guide/command-line-arguments.md) | 命令行参数说明 |

### 顶层文档

| 文档 | 说明 |
|------|------|
| [overview.md](docs/zh/overview.md) | 项目概览和核心特性 |
| [quick-start.md](docs/zh/quick-start.md) | 快速上手指南 |
| [project-structure.md](docs/zh/project-structure.md) | 项目结构详解 |
| [plugin-development.md](docs/zh/plugin-development.md) | 插件开发总览 |
| [plugin-system.md](docs/zh/plugin-system.md) | 插件系统概述 |
| [api-reference.md](docs/zh/api-reference.md) | API 参考手册 |
| [configuration.md](docs/zh/configuration.md) | 配置系统说明 |
| [best-practices.md](docs/zh/best-practices.md) | 开发最佳实践 |

---

## AI Agent 提示

涉及代码开发，你需要先阅读[开发规范](docs/zh/coding-standard.md)

### 快速定位核心代码

| 功能 | 文件路径 | 说明 |
|------|----------|------|
| **主程序入口** | `src/APP/main.cpp` | 程序启动点 |
| **主窗口** | `src/APP/DAApp.h` / `DAApp.cpp` | 主窗口类定义 |
| **工作流引擎** | `src/DAWorkFlow/DAWorkFlow.h` | 工作流管理核心 |
| **节点基类** | `src/DAWorkFlow/DAAbstractNode.h` | 所有节点的基础类 |
| **节点工厂基类** | `src/DAWorkFlow/DAAbstractNodeFactory.h` | 创建节点的工厂接口 |
| **图形节点** | `src/DAGraphicsView/DAGraphicsNodeItem.h` | 可视化节点图元 |
| **图形连线** | `src/DAGraphicsView/DAGraphicsLinkItem.h` | 可视化连接图元 |
| **插件管理器** | `src/DAPluginSupport/DAPluginManager.h` | 插件加载和管理 |
| **插件基类** | `src/DAPluginSupport/DAAbstractPlugin.h` | 插件基础接口 |
| **数据管理器** | `src/DAData/DADataManager.h` | 数据管理核心 |
| **核心接口** | `src/DAInterface/DACoreInterface.h` | 主程序核心接口 |
| **UI 接口** | `src/DAInterface/DAUIInterface.h` | 界面操作接口 |

### 编码规范要点

**命名约定**：
- 类名：`DA` 开头 + 大驼峰，如 `DAWorkFlowNode`
- 命名空间：统一放入 `DA` 命名空间
- 成员变量：`m_` 前缀 + 小驼峰，如 `m_workflowName`
- 函数：小驼峰，如 `getWorkflow()`
- 信号：`Changed` 后缀，如 `workflowChanged()`
- 槽函数：`on` 前缀，如 `onWorkflowChanged()`

**属性定义**：
```cpp
// 使用 Q_PROPERTY 暴露属性
Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
```

**信号槽语法**：
- **必须使用** `Q_SIGNALS`、`Q_SLOT`、`Q_SIGNAL` 宏
- **禁止使用** `signals`、`slots`、`signal` 小写关键字

**Qt 版本兼容**：
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 的实现
#else
    // Qt6 的实现
#endif
```

**PIMPL 模式宏**：
- `DA_DECLARE_PRIVATE` - 在类中声明私有数据指针
- `DA_DECLARE_PUBLIC` - 在 PrivateData 中声明公有类指针
- `DA_D` / `DA_DC` - 获取私有数据指针

---

## 相关文件

| 文件 | 说明 |
|------|------|
| [AGENTS.md](AGENTS.md) | AI Agent 开发详细指南（编码规范、注释规范、构建指南） |
| [README.md](README.md) | 项目简介和第三方库说明 |
| [docs/doc-writing-guide.md](docs/doc-writing-guide.md) | 文档撰写规范手册 |
| [docs/zh/index.md](docs/zh/index.md) | 中文文档入口 |

---

*本文件专为 AI Agent 导航设计，如需完整信息请查阅 [AGENTS.md](AGENTS.md)。*