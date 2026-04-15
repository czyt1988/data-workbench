# 模块依赖关系说明

data-workbench采用分层模块化架构，各模块之间有明确的依赖关系。本文档详细说明模块的层次结构和依赖配置。

## 主要模块概览

**模块分类**

- ✅ **基础层模块**：DAShared、DAUtils - 提供基础工具和共享定义
- ✅ **功能层模块**：DAData、DAFigure、DAWorkFlow - 核心业务功能
- ✅ **界面层模块**：DAGui、DACommonWidgets - GUI界面组件
- ✅ **接口层模块**：DAInterface、DAPluginSupport - 插件和扩展接口
- ✅ **应用层模块**：APP - 主应用程序

## 分层架构说明

data-workbench采用五层架构设计，从底层到应用层依次为：基础层、功能层、界面层、接口层和应用层。

### 架构层次图

下图展示了 data-workbench 的五层架构设计，从基础层到应用层的模块依赖关系：

```mermaid
flowchart TB
    subgraph Layer5["应用层 (Layer 5)"]
        APP["APP<br/>主应用程序"]
    end
    
    subgraph Layer4["接口层 (Layer 4)"]
        DAInterface["DAInterface<br/>接口定义"]
        DAPluginSupport["DAPluginSupport<br/>插件支持"]
    end
    
    subgraph Layer3["界面层 (Layer 3)"]
        DAGui["DAGui<br/>GUI模块"]
        DACommonWidgets["DACommonWidgets<br/>通用控件"]
    end
    
    subgraph Layer2["功能层 (Layer 2)"]
        DAData["DAData<br/>数据处理"]
        DAFigure["DAFigure<br/>图表模块"]
        DAWorkFlow["DAWorkFlow<br/>工作流"]
        DAGraphicsView["DAGraphicsView<br/>图形视图"]
        DAPyCommonWidgets["DAPyCommonWidgets<br/>Python控件"]
        DAPyScripts["DAPyScripts<br/>Python脚本"]
    end
    
    subgraph Layer1["基础层 (Layer 1)"]
        DAUtils["DAUtils<br/>核心工具"]
        DAShared["DAShared<br/>共享定义"]
        DAMessageHandler["DAMessageHandler<br/>日志处理"]
        DAPyBindQt["DAPyBindQt<br/>Python绑定"]
    end
    
    APP --> DAPluginSupport
    DAPluginSupport --> DAInterface
    DAInterface --> DAGui
    DAGui --> DAData & DAFigure & DAWorkFlow & DACommonWidgets
    DAData --> DAPyBindQt & DAPyScripts
    DAFigure --> DAUtils
    DAWorkFlow --> DAGraphicsView
    DAGraphicsView --> DAUtils
    DACommonWidgets --> DAUtils
    DAPyBindQt --> DAUtils
    DAMessageHandler --> DAUtils
DAUtils --> DAShared
    ```

上图展示了五层架构结构：
- **Layer 1 基础层**：DAUtils、DAShared、DAMessageHandler、DAPyBindQt 提供底层支撑
- **Layer 2 功能层**：DAData、DAFigure、DAWorkFlow、DAGraphicsView 等实现核心业务
- **Layer 3 界面层**：DAGui、DACommonWidgets 整合可视化功能
- **Layer 4 接口层**：DAInterface、DAPluginSupport 定义插件和扩展机制
- **Layer 5 应用层**：APP 主程序整合所有模块

### 各层职责说明

| 层次 | 职责 | 模块 |
|------|------|------|
| **基础层** | 提供基础工具类、模板类、日志处理和Python绑定 | DAShared, DAUtils, DAMessageHandler, DAPyBindQt |
| **功能层** | 实现核心业务功能：数据处理、图表绑制、工作流、图形视图 | DAData, DAFigure, DAWorkFlow, DAGraphicsView, DAPyScripts, DAPyCommonWidgets |
| **界面层** | 提供GUI组件和界面功能，整合所有可视化元素 | DAGui, DACommonWidgets |
| **接口层** | 定义插件接口和扩展机制，实现模块间通信 | DAInterface, DAPluginSupport |
| **应用层** | 主程序入口，整合所有模块实现完整功能 | APP |

## 模块依赖表

data-workbench主要有以下模块：

| 模块名 | 模块简述 | 依赖模块 |
|--------|---------|---------|
| **DAShared** | 共享头文件模块，提供基础的模板类和共享定义 | Qt::Core |
| **DAUtils** | 核心工具模块，提供通用工具函数和基础类 | DAShared, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml |
| **DAMessageHandler** | 日志处理模块，基于spdlog实现 | DAUtils, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, spdlog |
| **DAPyBindQt** | Python与Qt绑定模块，基于pybind11实现 | DAUtils, Qt::Core, Python, pybind11 |
| **DAPyScripts** | Python脚本包装模块 | DAPyBindQt, Qt::Core, Python, pybind11 |
| **DAPyCommonWidgets** | Python相关通用控件模块 | DAPyBindQt, Qt::Core, Qt::Gui, Qt::Widgets, Python, pybind11 |
| **DAData** | 数据处理模块，管理数据对象和操作 | DAUtils, DAPyBindQt, DAPyScripts, Qt::Core, Qt::Gui, Qt::Widgets, Python, pybind11 |
| **DACommonWidgets** | 通用控件模块 | DAUtils, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, DALiteCtk, SARibbonBar |
| **DAGraphicsView** | 图形视图模块 | DAUtils, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, Qt::Svg |
| **DAWorkFlow** | 工作流模块 | DAUtils, DAGraphicsView, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, Qt::Svg, tsl-ordered-map |
| **DAFigure** | 图表模块，基于Qwt实现科学图表功能 | DAUtils, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Concurrent, Qt::PrintSupport, qwt |
| **DAGui** | GUI界面模块，整合所有UI功能 | DAUtils, DAMessageHandler, DAData, DACommonWidgets, DAWorkFlow, DAFigure, DAPyBindQt, DAPyScripts, DAPyCommonWidgets, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, Qt::Svg, Qt6::Core5Compat (Qt6), SARibbonBar, QtAdvancedDocking, qwt, DALiteCtk, quazip |
| **DAInterface** | 接口模块，提供插件和扩展接口 | DAGui, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, SARibbonBar, QtAdvancedDocking, qwt, DALiteCtk, Python, pybind11 |
| **DAPluginSupport** | 插件支持模块 | DAInterface, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, QtAdvancedDocking, Python, pybind11 |
| **DAAxOfficeWrapper** | Office自动化模块（仅Windows） | DAUtils, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, Qt::AxContainer |
| **APP** | 主应用程序 | DAPluginSupport, Qt::Core, Qt::Gui, Qt::Widgets, Qt::Xml, Qt::Svg, Qt::Concurrent, Qt::PrintSupport, Qt::AxContainer (Windows), Qt6::Core5Compat (Qt6), DALiteCtk, SARibbonBar, QtAdvancedDocking, qwt, Python, Dbghelp (Windows) |

!!! tips "备注"
    1. 所有模块都依赖于Qt框架的不同组件
    2. Python相关模块（以"Py"开头的模块）仅在启用Python支持时构建
    3. DAAxOfficeWrapper模块仅在Windows平台上构建
    4. 第三方依赖包括：Qt、Qwt、SARibbonBar、QtAdvancedDocking、spdlog、pybind11、Python、DALiteCtk、tsl-ordered-map、quazip等

## 模块依赖关系图

DA各个模块的依赖关系图如下：

下图展示了所有 DA 模块之间的详细依赖关系，箭头表示依赖方向：

```mermaid
graph BT
    subgraph "DA Modules Dependency"
        DAUtils
        DAShared
        DAMessageHandler
        DAPyBindQt
        DAPyScripts
        DAPyCommonWidgets
        DAData
        DACommonWidgets
        DAGraphicsView
        DAWorkFlow
        DAFigure
        DAGui
        DAInterface
        DAPluginSupport
        DAAxOfficeWrapper
        APP
        
        DAShared --> DAUtils
        DAMessageHandler --> DAUtils
        DAPyBindQt --> DAUtils
        DAPyScripts --> DAPyBindQt
        DAPyCommonWidgets --> DAPyBindQt
        DAData --> DAUtils
        DAData --> DAPyBindQt
        DAData --> DAPyScripts
        DACommonWidgets --> DAUtils
        DAGraphicsView --> DAUtils
        DAWorkFlow --> DAUtils
        DAWorkFlow --> DAGraphicsView
        DAFigure --> DAUtils
        DAGui --> DAUtils
        DAGui --> DAMessageHandler
        DAGui --> DAData
        DAGui --> DACommonWidgets
        DAGui --> DAWorkFlow
        DAGui --> DAFigure
        DAGui --> DAPyBindQt
        DAGui --> DAPyScripts
        DAGui --> DAPyCommonWidgets
        DAInterface --> DAGui
        DAPluginSupport --> DAInterface
        DAAxOfficeWrapper --> DAUtils
        APP --> DAPluginSupport
    end
    
    style DAUtils fill:#e1f5fe
    style DAShared fill:#fce4ec
    style DAMessageHandler fill:#e1f5fe
    style DAPyBindQt fill:#e1f5fe
    style DAPyScripts fill:#e1f5fe
    style DAPyCommonWidgets fill:#e1f5fe
    style DAData fill:#e1f5fe
    style DACommonWidgets fill:#e1f5fe
    style DAGraphicsView fill:#e1f5fe
    style DAWorkFlow fill:#e1f5fe
    style DAFigure fill:#e1f5fe
    style DAGui fill:#e1f5fe
    style DAInterface fill:#e1f5fe
    style DAPluginSupport fill:#e1f5fe
    style DAAxOfficeWrapper fill:#e1f5fe
style APP fill:#fff3e0
    ```

上图展示了模块依赖的详细关系：
- `DAUtils` 是核心基础模块，几乎所有模块都依赖它
- `DAGui` 是最大的 GUI 模块，整合多个功能模块
- `DAInterface` 依赖 `DAGui`，定义核心接口
- `DAPluginSupport` 依赖 `DAInterface`，提供插件支持
- `APP` 依赖 `DAPluginSupport`，是最终的应用程序

## 核心模块详解

### 基础层模块

基础层模块提供底层支撑，是所有上层模块的基础。

**DAUtils模块**

DAUtils是核心工具模块，几乎所有其他模块都依赖它。提供：

- 通用工具函数
- 基础类定义
- 日志宏封装
- 类型定义

**DAShared模块**

DAShared提供共享头文件和模板类，是最底层的模块，仅依赖Qt::Core。

### 功能层模块

功能层模块实现核心业务逻辑，不涉及界面显示。

**DAData模块**

数据处理模块，管理数据对象和操作：

- 数据容器管理
- 数据类型定义
- Python数据交互

**DAWorkFlow模块**

工作流模块，基于有向图描述数据处理流程：

- 节点和连接管理
- 图形场景扩展
- 工作流执行引擎

**DAFigure模块**

图表模块，基于Qwt实现科学图表功能：

- 多图表管理
- Undo/Redo支持
- 交互编辑功能

### 界面层模块

界面层模块整合所有可视化功能。

**DAGui模块**

DAGui是最大的GUI模块，整合了：

- Ribbon界面
- Dock窗口管理
- 属性编辑器
- 图表样式设置

### 接口层模块

接口层定义插件和扩展机制。

**DAInterface模块**

定义核心接口：

- `DACoreInterface` - 核心接口
- `DAUIInterface` - UI接口
- 数据、图表、工作流接口

**DAPluginSupport模块**

提供插件支持：

- `DAAbstractPlugin` - 插件基类
- `DAPluginManager` - 插件管理器
- `DAAbstractNodePlugin` - 节点插件基类

## 依赖配置示例

在CMake中配置模块依赖：

以下代码展示了 CMake 中模块依赖配置的基本示例，包括 Qt 查找和库链接：

```cmake
# 添加模块依赖示例
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 创建DAUtils模块
add_library(DAUtils
    src/DAUtils/DAUtils.cpp
)
target_link_libraries(DAUtils
    PUBLIC
        Qt6::Core
        Qt6::Gui
)

# 创建DAFigure模块，依赖DAUtils
add_library(DAFigure
    src/DAFigure/DAFigureWidget.cpp
    src/DAFigure/DAChartWidget.cpp
)
target_link_libraries(DAFigure
    PUBLIC
        DAUtils
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
    PRIVATE
qwt
    )
```

上述 CMake 配置的关键点：
- 使用 `find_package` 查找 Qt6 组件
- 使用 `add_library` 创建模块库
- 使用 `PUBLIC` 链接导出依赖，使用 `PRIVATE` 链接内部依赖
- DAFigure 依赖 DAUtils 和 Qt 组件，内部依赖 qwt

## 注意事项

!!! warning "依赖顺序"
    模块链接顺序很重要，上层模块必须在上层模块之后链接。

!!! tip "模块隔离"
    功能层模块之间尽量减少直接依赖，通过接口层进行通信。

!!! note "Python模块"
    Python相关模块需要Python环境和pybind11支持，未启用Python时这些模块不会构建。

!!! note "平台差异"
    DAAxOfficeWrapper仅在Windows平台构建，使用Qt::AxContainer组件。

## 参考资料

- [项目构建指南](../build/build-guide.md)
- [插件模块DAPluginSupport](./plugin-module.md)
- [绘图模块概述](./figure-abstract.md)