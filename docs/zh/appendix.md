# 附录

本页面汇总 DAWorkBench 项目的补充信息，包括术语表、CMake 宏说明、社区支持等，作为开发文档的参考附录。

!!! tip "配置文件格式和更新日志"
    配置文件格式详见 [配置文件格式](./appendix-config.md)，版本变更历史详见 [更新日志](./changelog.md)。

## 主要功能特性

**特性**

- ✅ **术语表**：项目核心术语的中英文对照和解释
- ✅ **缩略语解释**：常用缩略语的全称和含义
- ✅ **CMake 辅助宏**：插件构建常用的宏命令说明
- ✅ **社区支持渠道**：问题反馈和开发交流的联系方式

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
| **DAPyWorkFlow** | 工作流管理、节点代理、执行引擎 |
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

## 更新日志

完整的版本变更历史详见 [更新日志](./changelog.md)。

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