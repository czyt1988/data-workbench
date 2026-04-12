# 术语表

本页面汇总 DAWorkBench 项目中使用的专业术语和缩略语，便于查阅和理解。

## 核心概念

### 工作流 (Workflow)

**定义**：数据处理流程的有向图描述，包含节点和连接关系。

**用途**：组织和自动化数据处理流程，支持可重复执行。

### 节点 (Node)

**定义**：工作流中的处理单元，代表一个数据处理步骤。

**特性**：
- 拥有输入和输出连接点
- 可配置参数
- 执行特定数据处理任务

### 连接点 (Link Point)

**定义**：节点的输入或输出接口，用于节点间的数据传递。

**类型**：
- **输入连接点**：接收数据
- **输出连接点**：发送数据

### 原型 (Prototype)

**定义**：节点的唯一标识符，用于区分不同类型的节点。

**格式**：`[Plugin].[Factory].[NodeName]`

**示例**：`DataAnalysis.IO.CSVReader`

### 元数据 (Metadata)

**定义**：节点的固定属性描述，包括名称、图标、连接点等。

**用途**：用于节点列表显示和节点创建。

### 节点工厂 (Node Factory)

**定义**：创建特定类型节点的工厂类。

**职责**：
- 注册节点元数据
- 创建节点实例
- 管理节点生命周期钩子

### 执行器 (Executer)

**定义**：执行工作流的引擎，负责按拓扑顺序执行节点。

**特性**：
- 在独立线程运行
- 支持进度回调
- 支持错误处理

## 图形相关

### 图元 (Graphics Item)

**定义**：节点的可视化表示，显示在工作流画布上。

**职责**：
- 渲染节点外观
- 处理用户交互
- 管理连接点位置

### 场景 (Scene)

**定义**：图形视图的场景管理类，管理所有图元和连接线。

**用途**：协调图元显示和工作流逻辑。

### 视图 (View)

**定义**：显示场景的控件，支持缩放、平移等交互。

**特性**：
- 可缩放图元
- 支持选中、拖动
- 支持快捷键操作

## 界面相关

### Dock 窗口 (Dock Widget)

**定义**：可停靠的窗口控件，可在界面边缘浮动或停靠。

**用途**：显示工具面板、数据列表、属性编辑器等。

### Ribbon

**定义**：Office 风格的工具栏，包含选项卡和面板。

**组成**：
- **Category**：选项卡
- **Panel**：面板
- **Action**：按钮/命令

### Action

**定义**：Qt 的动作对象，代表一个可执行的命令。

**用途**：菜单项、工具栏按钮、快捷键绑定。

## 插件相关

### 插件 (Plugin)

**定义**：独立的扩展模块，提供额外功能。

**特性**：
- 动态加载
- 通过接口通信
- 独立编译

### 接口 (Interface)

**定义**：插件与主程序通信的抽象类。

**层次**：
- **DACoreInterface**：顶层接口
- **DAAppUIInterface**：UI 接口
- **DADataManagerInterface**：数据接口

### 核心接口 (Core Interface)

**定义**：顶层接口，通过它可以获取所有其他接口。

**用途**：插件访问主程序功能的入口。

## 数据相关

### DataFrame

**定义**：pandas 的二维表格数据结构。

**用途**：数据处理和分析的核心数据格式。

### 数据包 (Data Package)

**定义**：DAWorkBench 的数据包装类，用于在工作流中传递数据。

**内容**：可包含 DataFrame、自定义数据等。

### 序列化 (Serialization)

**定义**：对象到数据格式的转换过程。

**用途**：保存工作流、节点配置、数据等。

### 持久化 (Persistence)

**定义**：数据的长期存储，保存到文件系统。

**形式**：配置文件、数据文件、缓存文件等。

## 架构相关

### MVC

**定义**：Model-View-Controller 架构模式。

**对应**：
- **Model**：DAWorkFlow（工作流逻辑）
- **View**：DANodeGraphicsScene（图形显示）
- **Controller**：用户交互处理

### 松耦合 (Loose Coupling)

**定义**：模块间通过接口通信，无直接依赖。

**优势**：
- 易于扩展
- 易于测试
- 易于维护

### 热插拔 (Hot Plug)

**定义**：插件可在运行时加载或卸载。

**实现**：Qt 插件机制 + 动态库加载。

## 英文缩写

| 缩写 | 全称 | 中文 |
|------|------|------|
| DA | Data Analysis | 数据分析 |
| ETL | Extract, Transform, Load | 数据抽取转换加载 |
| GUI | Graphical User Interface | 图形用户界面 |
| API | Application Programming Interface | 应用程序接口 |
| IID | Interface Identifier | 接口标识符 |
| Qt | Qt Framework | Qt 框架 |
| Py | Python | Python 语言 |
| DF | DataFrame | 数据框 |
| JSON | JavaScript Object Notation | JSON 格式 |
| CSV | Comma-Separated Values | CSV 格式 |
| XML | Extensible Markup Language | XML 格式 |

## 下一步

- [:material-book: 开发指南](./dev-guide/coding-standard.md) - 编码规范
- [:material-api: API 文档](./api-reference.md) - API 参考
- [:material-puzzle: 插件开发](./plugin-development.md) - 插件开发指南