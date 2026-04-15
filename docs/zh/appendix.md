# 附录

本页面汇总 DAWorkBench 项目的补充信息，包括配置文件格式、术语表、CMake 宏说明、社区支持等，作为开发文档的参考附录。

## 主要功能特性

**特性**

- ✅ **配置文件格式说明**：项目、工作流、插件的配置文件 JSON 格式定义
- ✅ **术语表**：项目核心术语的中英文对照和解释
- ✅ **缩略语解释**：常用缩略语的全称和含义
- ✅ **CMake 辅助宏**：插件构建常用的宏命令说明
- ✅ **社区支持渠道**：问题反馈和开发交流的联系方式

## 配置文件格式说明

### 项目配置文件 (.dawproj)

项目配置文件使用 JSON 格式，存储项目的基本信息、工作流关联、数据文件列表和插件配置。

下面的 JSON 示例展示了典型项目配置文件的结构：

```json
{
    "version": "1.0",
    "name": "MyProject",
    "created": "2024-03-10T10:30:00",
    "workflow": {
        "path": "workflow.daw",           // 工作流文件路径
        "nodes": [
            {
                "id": "node_001",          // 节点唯一标识
                "prototype": "DataAnalysis.IO.CSVReader",  // 节点原型
                "position": {"x": 100, "y": 100}  // 节点位置
            }
        ]
    },
    "data": {
        "files": [
            "data/input.csv",              // 数据文件列表
            "data/output.xlsx"
        ]
    },
    "plugins": {
        "MyPlugin": {
            "config": "plugins/MyPlugin/config.json"  // 插件配置路径
        }
    }
}
```

上述配置文件包含项目名称、创建时间、工作流信息、数据文件和插件配置等核心内容。

### 工作流配置文件 (.daw)

工作流配置文件存储节点列表、连接关系和分组信息，是工作流持久化的核心文件。

下面的 JSON 示例展示了工作流配置文件的结构：

```json
{
    "version": "1.0",
    "nodes": [
        {
            "id": "node_001",                       // 节点唯一标识
            "prototype": "My.Factory.Process",      // 节点原型（工厂.节点名）
            "name": "Data Process",                 // 节点显示名称
            "position": {"x": 200, "y": 150},       // 画布位置坐标
            "data": {
                "algorithm": "default",             // 节点自定义数据
                "threshold": 0.5                    // 配置参数
            }
        }
    ],
    "links": [
        {
            "from_node": "node_001",                // 连接起始节点
            "from_key": "output",                   // 起始节点输出端口
            "to_node": "node_002",                  // 连接目标节点
            "to_key": "input"                       // 目标节点输入端口
        }
    ]
}
```

工作流文件通过节点 ID 和端口名称建立连接关系，支持节点的自定义数据存储。

### 插件配置文件格式

插件配置文件支持 JSON 和 INI 两种格式，用于存储插件的设置参数。

下面的 JSON 示例展示了插件配置文件的典型结构：

```json
{
    "version": 2,                            // 配置文件版本号
    "general": {
        "auto_save": true,                   // 自动保存开关
        "max_cache_size": 100,               // 最大缓存大小（MB）
        "log_level": "info"                  // 日志级别
    },
    "processing": {
        "algorithm": "advanced",             // 处理算法选择
        "threshold": 0.7,                    // 阈值参数
        "filters": ["filter1", "filter2"]    // 过滤器列表
    },
    "ui": {
        "dock_position": "right",            // Dock 窗口位置
        "show_toolbar": true                 // 工具栏显示开关
    }
}
```

插件配置建议包含版本号，以便后续升级时的兼容性处理。配置文件通常存储在项目的 `plugins/[插件名]/` 目录下。

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

设置插件基本信息，包括名称、描述和版本号。此宏简化了插件 CMake 配置的编写。

下面的 CMake 示例展示了插件设置宏的使用方法：

```cmake
damacro_plugin_setting(
    "PluginName"        # 插件名称 - 将生成 DA_PLUGIN_NAME 变量
    "Description"       # 插件描述 - 将生成 DA_PLUGIN_FULL_DESCRIPTION 变量
    0                   # 主版本号
    0                   # 欯版本号
    1                   # 补丁版本号
    ${INSTALL_DIR}      # 安装目录路径
)
```

执行此宏后，将生成以下可用变量：

- `DA_PLUGIN_NAME` - 插件名称
- `DA_PLUGIN_VERSION` - 完整版本号
- `DA_PLUGIN_FULL_DESCRIPTION` - 完整描述

### damacro_import_*

导入第三方库，简化第三方库的链接配置。这些宏自动处理库路径和依赖关系。

下面的 CMake 示例展示了第三方库导入宏的使用方法：

```cmake
# 导入各类第三方库 - 参数为目标名称和安装目录
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${INSTALL_DIR})    # Ribbon 界面框架
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${INSTALL_DIR})      # CTK 精简版
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${INSTALL_DIR})  # Dock 窗口系统
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${INSTALL_DIR})  # 属性浏览器
damacro_import_qwt(${DA_PLUGIN_NAME} ${INSTALL_DIR})            # 科学图表库
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${INSTALL_DIR})     # 有序 map 实现
```

这些宏将自动设置链接库路径和依赖，确保插件能正确使用主程序安装的第三方库。

### damacro_plugin_install

安装插件到目标目录，将编译产物复制到主程序的插件目录。

```cmake
damacro_plugin_install()
```

此宏无需参数，自动将插件安装到 `bin/plugins/` 目录下。

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