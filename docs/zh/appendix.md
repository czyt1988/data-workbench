# 附录

本页面汇总 DAWorkBench 项目的补充信息，包括配置格式、术语表、CMake 宏说明等。

## 配置文件格式说明

### 项目配置文件 (.dawproj)

```json
{
    "version": "1.0",
    "name": "MyProject",
    "created": "2024-03-10T10:30:00",
    "workflow": {
        "path": "workflow.daw",
        "nodes": [
            {
                "id": "node_001",
                "prototype": "DataAnalysis.IO.CSVReader",
                "position": {"x": 100, "y": 100}
            }
        ]
    },
    "data": {
        "files": [
            "data/input.csv",
            "data/output.xlsx"
        ]
    },
    "plugins": {
        "MyPlugin": {
            "config": "plugins/MyPlugin/config.json"
        }
    }
}
```

### 工作流配置文件 (.daw)

```json
{
    "version": "1.0",
    "nodes": [
        {
            "id": "node_001",
            "prototype": "My.Factory.Process",
            "name": "Data Process",
            "position": {"x": 200, "y": 150},
            "data": {
                "algorithm": "default",
                "threshold": 0.5
            }
        }
    ],
    "links": [
        {
            "from_node": "node_001",
            "from_key": "output",
            "to_node": "node_002",
            "to_key": "input"
        }
    ]
}
```

### 插件配置文件格式

```json
{
    "version": 2,
    "general": {
        "auto_save": true,
        "max_cache_size": 100,
        "log_level": "info"
    },
    "processing": {
        "algorithm": "advanced",
        "threshold": 0.7,
        "filters": ["filter1", "filter2"]
    },
    "ui": {
        "dock_position": "right",
        "show_toolbar": true
    }
}
```

## 术语表

| 术语 | 英文 | 说明 |
|------|------|------|
| **工作流** | Workflow | 数据处理流程的有向图描述 |
| **节点** | Node | 工作流中的处理单元 |
| **连接点** | Link Point | 节点的输入输出接口 |
| **原型** | Prototype | 节点的唯一标识符 |
| **元数据** | Metadata | 节点的固定属性描述 |
| **节点工厂** | Node Factory | 创建节点的工厂类 |
| **执行器** | Executer | 执行工作流的引擎 |
| **图元** | Graphics Item | 节点的可视化表示 |
| **场景** | Scene | 图形视图的场景管理 |
| **Dock 窗口** | Dock Widget | 可停靠的窗口控件 |
| **Ribbon** | Ribbon | Office 风格的工具栏 |
| **Action** | Action | Qt 的动作对象 |
| **接口** | Interface | 插件与主程序通信的抽象类 |
| **核心接口** | Core Interface | 顶层接口，获取所有其他接口 |
| **序列化** | Serialization | 对象到数据格式的转换 |
| **持久化** | Persistence | 数据的长期存储 |

## 缩略语解释

| 缩略语 | 全称 | 说明 |
|--------|------|------|
| **DA** | Data Analysis | 数据分析（项目前缀） |
| **ETL** | Extract, Transform, Load | 数据抽取、转换、加载 |
| **GUI** | Graphical User Interface | 图形用户界面 |
| **API** | Application Programming Interface | 应用程序编程接口 |
| **IID** | Interface Identifier | 接口标识符 |
| **MVC** | Model-View-Controller | 模型-视图-控制器架构 |
| **Qt** | Qt Framework | Qt 应用框架 |
| **Py** | Python | Python 语言 |
| **DF** | DataFrame | pandas DataFrame 数据结构 |
| **JSON** | JavaScript Object Notation | JSON 数据格式 |
| **XML** | Extensible Markup Language | XML 标记语言 |
| **CSV** | Comma-Separated Values | CSV 数据格式 |

## CMake 辅助宏说明

### damacro_plugin_setting

设置插件基本信息：

```cmake
damacro_plugin_setting(
    "PluginName"        # 插件名称
    "Description"       # 插件描述
    0                   # 主版本号
    0                   # 次版本号
    1                   # 补丁版本号
    ${INSTALL_DIR}      # 安装目录
)
```

生成的变量：

- `DA_PLUGIN_NAME` - 插件名称
- `DA_PLUGIN_VERSION` - 完整版本号
- `DA_PLUGIN_FULL_DESCRIPTION` - 完整描述

### damacro_import_*

导入第三方库：

```cmake
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${INSTALL_DIR})
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${INSTALL_DIR})
```

### damacro_plugin_install

安装插件到目标目录：

```cmake
damacro_plugin_install()
```

## 模块组件列表

| 模块 | 可用组件 |
|------|----------|
| **DAUtils** | 日志、配置、文件工具、字符串处理 |
| **DAWorkFlow** | 工作流管理、节点工厂、执行器 |
| **DAGraphicsView** | 可缩放视图、redo/undo、图元管理 |
| **DAFigure** | 图表绘制、坐标轴、图例 |
| **DAData** | DataFrame 管理、数据对象 |
| **DACommonWidgets** | 通用对话框、表格控件 |
| **DAGui** | Ribbon、Dock、主界面 |
| **DAInterface** | 所有接口定义 |
| **DAPluginSupport** | 插件管理器、插件基类 |

## 社区支持与反馈渠道

### 官方渠道

| 渠道 | 地址 | 说明 |
|------|------|------|
| **GitHub** | https://github.com/czyt1988/data-workbench | 主项目仓库 |
| **Gitee** | https://gitee.com/czyt1988/data-workbench | 国内镜像仓库 |
| **文档站点** | https://czyt1988.github.io/data-workbench | 在线文档 |

### 问题反馈

1. **Bug 报告**：在 GitHub Issues 提交，包含：
   - 问题描述
   - 复现步骤
   - 系统环境信息
   - 相关日志

2. **功能建议**：在 GitHub Discussions 讨论

3. **代码贡献**：提交 Pull Request

### 开发交流

- 关注项目动态：Watch GitHub 仓库
- 参与讨论：GitHub Discussions
- 查看更新：Release Notes

## 更新日志摘要

### v0.0.3 (当前版本)

- 完善插件系统架构
- 增加工作流执行回调
- 优化节点连接机制
- 改进数据序列化

### v0.0.2

- 基础工作流功能
- 简单数据处理节点
- 基础图表绘制

### v0.0.1

- 项目初始化
- 基础框架搭建
- 插件系统原型

## 相关项目

| 项目 | 说明 |
|------|------|
| **SARibbon** | Ribbon 界面框架 |
| **Qt-Advanced-Docking** | 高级 Dock 窗口系统 |
| **qwt** | 科学图表库 |
| **pybind11** | Python/C++ 绑定 |

## 许可证说明

本项目采用 LGPL 3.0 许可证：

- 可以自由使用、修改、分发
- 修改后的代码需要保持开源
- 可以作为其他软件的依赖库

详见 [LICENSE](../LICENSE) 文件。