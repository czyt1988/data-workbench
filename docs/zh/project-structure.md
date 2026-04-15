# 项目结构详解

本文档详细说明 DAWorkBench 的目录结构、模块职责和关键文件，帮助开发者快速理解项目组织方式和各模块的功能。

## 主要功能特性

**特性**

- ✅ **目录结构总览**：src、plugins、docs、cmake 等顶层目录说明
- ✅ **核心模块架构**：应用层、插件层、界面层、业务层、基础层的分层架构
- ✅ **模块详细说明**：各模块职责和依赖关系的完整表格
- ✅ **关键文件说明**：根目录、src 目录、cmake 目录的关键文件列表
- ✅ **模块设计原则**：分层架构、插件化设计、命名约定、Qt 信号槽机制
- ✅ **插件目录结构**：标准插件结构和模板生成方法
- ✅ **第三方库目录**：src/3rdparty 结构和构建顺序
- ✅ **输出目录结构**：构建输出的 bin 目录组织方式

---

## 目录结构总览

下面的目录结构展示了项目的整体组织方式：

```text
data-workbench/
├── src/                      # 源代码目录 - 所有模块源码
│   ├── APP/                  # 主应用程序 - 程序入口和主窗口
│   ├── DAInterface/          # 接口模块 - 插件和主程序通信桥梁
│   ├── DAPluginSupport/      # 插件支持模块 - 插件加载和管理
│   ├── DAGui/                # GUI 界面模块 - Ribbon、Dock 等
│   ├── DAWorkFlow/           # 工作流核心模块 - 有向图管理
│   ├── DAData/               # 数据管理模块 - DataFrame 操作
│   ├── DAFigure/             # 图表模块 - 基于 qwt 的科学图表
│   ├── DAGraphicsView/       # 图形视图模块 - 可缩放视图
│   ├── DACommonWidgets/      # 通用控件模块 - 常用 UI 控件
│   ├── DAUtils/              # 核心工具模块 - 基础函数和类
│   ├── DAMessageHandler/     # 日志处理模块 - 基于 spdlog
│   ├── DAPyBindQt/           # Python-Qt 绑定模块 - pybind11 封装
│   ├── DAPyScripts/          # Python 脚本包装模块 - Python API
│   ├── DAPyCommonWidgets/    # Python 相关控件模块 - Python UI
│   ├── DAShared/             # 共享头文件模块 - 公共定义
│   ├── DAAxOfficeWrapper/    # Office 自动化模块 - 仅 Windows
│   ├── 3rdparty/             # 第三方库源码 - submodule 管理
│   ├── PyScripts/            # Python 脚本资源 - 内置脚本
│   ├── i18n/                 # 国际化翻译文件 - .ts 文件
│   ├── tst/                  # 测试代码目录 - 单元测试
│   ├── DAConfigs.h           # 编译生成的配置头文件
│   ├── DAConfigs.h.in        # 配置文件生成模板
│   ├── DAGlobals.h           # 全局定义和宏
│   └── template-python-config.json  # Python 环境配置模板
├── plugins/                  # 插件目录 - 各功能插件
│   ├── DataAnalysis/         # 数据分析插件（示例）
│   ├── plugin-template/      # 插件模板生成工具
│   └── CMakeLists.txt        # 插件构建配置
├── docs/                     # 文档目录 - MkDocs 文档
│   ├── zh/                   # 中文文档
│   ├── assets/               # 文档资源文件
│   └── stylesheets/          # 文档样式文件
├── cmake/                    # CMake 辅助脚本 - 构建工具
├── scripts/                  # 构建辅助脚本 - 自动化脚本
├── stubs/                    # Python stubs 文件 - 类型提示
├── mkdocs.yml                # 文档配置文件
├── CMakeLists.txt            # 主 CMake 配置文件
├── readme.md                 # 项目说明文件
├── requirements.txt          # Python 运行依赖
└── requirements-docs.txt     # 文档生成依赖
```

上述目录结构遵循标准的大型 C++ 项目组织方式，便于维护和扩展。

---

## 模块职责划分

### 核心模块架构

下面的 mermaid 图表展示了项目的分层架构和模块依赖关系：

图表展示了应用层、插件层、界面层、业务层、基础层、Python 层六个层次及其依赖关系：

```mermaid
graph TB
    subgraph "应用层"
        APP[APP 主程序]       # 程序入口和主窗口
    end
    
    subgraph "插件层"
        PS[DAPluginSupport]  # 插件加载和管理
        IF[DAInterface]      # 接口定义
    end
    
    subgraph "界面层"
        GUI[DAGui]           # 界面整合
        CW[DACommonWidgets]  # 通用控件
    end
    
    subgraph "业务层"
        WF[DAWorkFlow]       # 工作流逻辑
        DATA[DAData]         # 数据管理
        FIG[DAFigure]        # 图表绘制
        GV[DAGraphicsView]   # 图形视图
    end
    
    subgraph "基础层"
        UTILS[DAUtils]       # 核心工具
        SHARED[DAShared]     # 共享定义
        MSG[DAMessageHandler] # 日志系统
    end
    
    subgraph "Python层"
        PB[DAPyBindQt]       # Python 绑定
        PS2[DAPyScripts]     # Python 脚本
        PCW[DAPyCommonWidgets] # Python 控件
    end
    
    APP --> PS               # 应用依赖插件支持
    PS --> IF                # 插件支持依赖接口
    IF --> GUI               # 接口依赖界面
    GUI --> WF               # 界面依赖工作流
    GUI --> DATA             # 界面依赖数据
    GUI --> FIG              # 界面依赖图表
    GUI --> CW               # 界面依赖通用控件
    WF --> GV                # 工作流依赖图形视图
    DATA --> PB              # 数据依赖 Python 绑定
    PB --> PS2               # 绑定依赖脚本
    CW --> UTILS             # 控件依赖工具
    MSG --> UTILS            # 日志依赖工具
    UTILS --> SHARED         # 工具依赖共享定义
    
    style APP fill:#fff3e0   # 橙色：应用层
    style PS fill:#e8f5e9    # 绿色：插件层
    style IF fill:#e8f5e9    # 绿色：接口层
    style GUI fill:#e1f5fe   # 蓝色：界面层
    style WF fill:#f3e5f5    # 紫色：业务层
    style DATA fill:#f3e5f5  # 紫色：业务层
    style FIG fill:#f3e5f5   # 紫色：业务层
```

项目采用清晰的分层架构，上层依赖下层，下层不依赖上层，确保模块职责明确。

### 模块详细说明

| 模块名 | 职责说明 | 依赖关系 |
|--------|----------|----------|
| **DAShared** | 共享头文件，基础模板类和定义 | Qt::Core |
| **DAUtils** | 核心工具函数、基础类、配置管理 | DAShared, Qt 组件 |
| **DAMessageHandler** | 基于 spdlog 的日志系统 | DAUtils, spdlog |
| **DAPyBindQt** | Python 与 Qt 的绑定层 | DAUtils, pybind11 |
| **DAPyScripts** | Python 脚本封装 | DAPyBindQt |
| **DAPyCommonWidgets** | Python 相关 Qt 控件 | DAPyBindQt |
| **DAData** | 数据对象管理、DataFrame 操作 | DAUtils, DAPyBindQt |
| **DACommonWidgets** | 通用 Qt 控件库 | DAUtils, SARibbonBar |
| **DAGraphicsView** | 可缩放图形视图、redo/undo | DAUtils |
| **DAWorkFlow** | 工作流核心逻辑、有向图管理 | DAUtils, DAGraphicsView |
| **DAFigure** | 科学图表绘制（基于 qwt） | DAUtils, qwt |
| **DAGui** | 界面整合、Ribbon、Dock 管理 | 所有业务模块 |
| **DAInterface** | 插件接口定义 | DAGui |
| **DAPluginSupport** | 插件加载、管理 | DAInterface |
| **DAAxOfficeWrapper** | Office 自动化（仅 Windows） | DAUtils |
| **APP** | 主程序入口 | DAPluginSupport |

---

## 关键文件说明

### 根目录关键文件

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 主构建配置，定义版本、依赖、编译选项 |
| `DAWorkbenchConfig.cmake.in` | CMake 导出配置模板 |
| `readme.md` | 项目说明和快速上手指南 |
| `mkdocs.yml` | MkDocs 文档配置 |
| `requirements.txt` | Python 运行依赖（pandas, numpy, scipy 等） |
| `requirements-docs.txt` | 文档生成依赖 |

### src 目录关键文件

| 文件路径 | 说明 |
|----------|------|
| `src/DAConfigs.h.in` | 配置文件生成模板 |
| `src/DAConfigs.h` | 编译后生成的配置头文件 |
| `src/DAGlobals.h` | 全局定义和宏 |
| `src/template-python-config.json` | Python 环境配置模板 |

### cmake 目录关键脚本

| 文件 | 说明 |
|------|------|
| `daworkbench_utils.cmake` | 构建辅助工具函数 |
| `daworkbench_3rdparty.cmake` | 第三方库配置 |
| `daworkbench_plugin_utils.cmake` | 插件构建辅助宏 |

---

## 模块设计原则

### 1. 分层架构

项目采用清晰的分层架构：

```
应用层 → 插件层 → 界面层 → 业务层 → 基础层
```

- **向下依赖**：上层模块依赖下层模块，下层模块不依赖上层
- **接口隔离**：通过 `DAInterface` 实现插件和主程序的解耦

### 2. 插件化设计

- 所有业务功能通过插件提供
- 主程序仅提供基础框架
- 插件通过 `DACoreInterface` 访问主程序功能

### 3. 命名约定

| 类型 | 前缀 | 示例 |
|------|------|------|
| 模块 | DA | DAWorkFlow |
| 类 | DA | DAAbstractNode |
| 接口 | DA...Interface | DACoreInterface |
| 宏 | DA_ | DA_PLUGIN_NAME |
| 枚举 | DA...Enum | DANodeLinkPoint::Way |

### 4. Qt 信号槽机制

模块间通信主要使用 Qt 的信号槽机制：

```cpp
// 工作流执行完成信号
void DAWorkFlow::finished(bool success);

// 节点执行完成信号
void DAWorkFlow::nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state);
```

---

## 插件目录结构

### 插件标准结构

```text
MyPlugin/
├── CMakeLists.txt           # 插件构建配置
├── src/
│   ├── MyPlugin.h           # 插件主类头文件
│   ├── MyPlugin.cpp         # 插件主类实现
│   ├── MyNodeFactory.h      # 节点工厂（工作流插件）
│   ├── MyNodeFactory.cpp    # 节点工厂实现
│   ├── MyWorker.h           # 工作节点实现类
│   ├── MyWorker.cpp         # 工作节点实现
│   ├── Dialogs/             # 对话框控件
│   ├── icon/                # 图标资源
│   └── PyScripts/           # Python 脚本（可选）
├── data-workbench/          # 主项目子模块引用
└── template.json            # 插件模板配置
```

### 插件模板生成

使用 `plugins/plugin-template/make-plugin.py` 自动生成插件项目结构。

配置文件 `template.json`：

```json
{
    "plugin-base-name": "My",
    "plugin-display-name": "My Plugin",
    "plugin-description": "This is My Plugin",
    "plugin-iid": "Plugin.MyPlugin",
    "factory-prototypes": "My.Factory",
    "factory-name": "My Factory",
    "factory-description": "My Plugin Node Factory"
}
```

---

## 第三方库目录

### src/3rdparty 结构

```text
src/3rdparty/
├── zlib/                    # zlib 库（quazip 依赖）
├── quazip/                  # Qt zip 文件处理
├── spdlog/                  # 高性能日志库
├── SARibbon/                # Ribbon 界面框架
├── ADS/                     # Qt-Advanced-Docking-System
├── pybind11/                # Python/C++ 绑定
├── QtPropertyBrowser/       # Qt 属性浏览器
├── ordered-map/             # 有序 map 实现
├── qwt/                     # 科学图表库
├── ctk/                     # 医疗影像工具包（精简版）
└── CMakeLists.txt           # 第三方库构建配置
```

### 第三方库构建顺序

```mermaid
graph LR
    A[zlib] --> B[quazip]
    C[spdlog] --> D[主程序]
    E[SARibbon] --> D
    F[ADS] --> D
    G[qwt] --> D
    H[pybind11] --> D
    
    style A fill:#ffcdd2
    style B fill:#ffcdd2
    style D fill:#c8e6c9
```

!!! warning "注意"
    quazip 依赖 zlib，必须先构建并安装 zlib，并将 zlib.dll 复制到 bin 目录。

---

## 输出目录结构

构建完成后，二进制文件统一输出到：

```text
bin_{BuildType}_qt{QtVersion}_{Compiler}_{Platform}/
├── bin/
│   ├── DAWorkbench.exe      # 主程序
│   ├── plugins/             # 插件目录
│   └── *.dll                # 动态库
├── lib/
│   ├── cmake/               # CMake 配置文件
│   └── *.lib/*.a            # 静态库
├── share/
│   └── cmake/               # CMake 模块
└── python/                  # Python 环境（可选）
```

示例：使用 Qt 5.14.2、MSVC、Debug 模式、64 位编译：
```
bin_Debug_qt5.14.2_MSVC_x64/
```

---

## 下一步

- [:material-tools: 开发环境搭建](./build/build-instructions.md) - 详细构建指南
- [:material-puzzle: 插件系统概述](./plugin-system.md) - 了解插件架构
- [:material-book-open: 开发规范](./dev-guide/coding-standard.md) - 编码规范