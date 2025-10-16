# 模块依赖关系说明

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

DA各个模块的依赖关系图如下：

``` mermaid
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