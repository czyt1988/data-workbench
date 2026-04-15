# API 文档索引

本页面提供 DAWorkBench API 文档的快速导航，帮助开发者快速定位所需的接口和类。

## 主要功能特性

**特性**

- ✅ **Doxygen 完整文档**：自动生成的 API 文档，包含所有类、函数、枚举的详细说明
- ✅ **核心接口速览**：顶层接口和常用类的快速参考
- ✅ **插件开发 API**：插件基类和节点相关接口说明
- ✅ **工作流 API**：工作流管理和节点元数据接口
- ✅ **模块列表索引**：各模块功能概览表格

## Doxygen API 文档

完整的 API 文档由 Doxygen 生成，包含所有类、函数、枚举的详细说明。

- [:material-file-document: API 文档首页](../doxygen/index.html)
- [:material-format-list-bulleted: 类索引](../doxygen/classes.html)
- [:material-folder: 文件列表](../doxygen/files.html)

## 核心接口速览

### DACoreInterface

顶层接口，获取所有其他接口。插件通过此接口访问主程序的所有功能，是实现插件与主程序松耦合通信的核心入口。

下面的代码展示了 DACoreInterface 的核心方法定义：

```cpp
class DACoreInterface
{
public:
    // 获取 UI 接口 - 用于访问 Ribbon、Dock 等界面组件
    virtual DAAppUIInterface* getUiInterface() = 0;
    
    // 获取项目管理接口 - 用于项目创建、打开、保存等操作
    virtual DAProjectInterface* getProjectInterface() = 0;
    
    // 获取数据管理接口 - 用于数据对象的增删改查
    virtual DADataManagerInterface* getDataManagerInterface() = 0;
    
    // 获取工作流接口 - 用于工作流执行和管理
    virtual DAWorkFlowInterface* getWorkFlowInterface() = 0;
};
```

上述接口是插件开发的基石，通过 `core()` 方法获取 DACoreInterface 实例后，即可访问主程序的所有功能模块。

### DAAppUIInterface

UI 相关接口，提供对主程序界面组件的访问能力。通过此接口，插件可以扩展 Ribbon 工具栏、添加 Dock 窗口、注册 Action 等。

下面的代码展示了 DAAppUIInterface 的核心方法定义：

```cpp
class DAAppUIInterface
{
public:
    // 获取 Ribbon 区域接口 - 用于添加自定义工具栏按钮
    virtual DARibbonAreaInterface* getRibbonArea() = 0;
    
    // 获取 Dock 区域接口 - 用于添加自定义停靠窗口
    virtual DADockingAreaInterface* getDockingArea() = 0;
    
    // 获取 Actions 管理接口 - 用于注册和管理动作对象
    virtual DAActionsInterface* getActionInterface() = 0;
    
    // 获取命令接口 - 用于执行撤销/重做等操作
    virtual DACommandInterface* getCommandInterface() = 0;
    
    // 添加设置页面 - 在设置对话框中添加插件配置页
    virtual void addSettingsPage(const QString& name, QWidget* page) = 0;
};
```

通过上述接口，插件可以无缝集成到主程序界面中，实现功能扩展。

## 插件开发 API

### DAAbstractPlugin

插件基类，所有插件必须继承此类。该类定义了插件的基本结构、生命周期方法和元信息接口，是插件系统的核心抽象。

下面的代码展示了插件基类的核心定义：

```cpp
class DAAbstractPlugin : public QObject
{
    Q_OBJECT
public:
    // 获取核心接口 - 插件访问主程序功能的唯一入口
    DACoreInterface* core() const;
    
    // 插件信息 - 必须实现的元信息方法
    virtual QString pluginName() const = 0;       // 插件名称
    virtual QString pluginVersion() const = 0;    // 插件版本
    virtual QString pluginDescription() const = 0; // 插件描述
    
    // 初始化（必须实现）- 主程序加载插件后立即调用
    virtual bool initialize() = 0;
    
    // 语言变更 - 多语言支持回调
    virtual void retranslate();
};
```

插件开发者必须实现 `initialize()` 方法，在此方法中完成资源初始化、节点注册、界面设置等工作。

### DAAbstractNodePlugin

节点插件基类，继承自 DAAbstractPlugin，用于提供工作流节点的插件。此类扩展了节点工厂管理功能。

下面的代码展示了节点插件基类的核心定义：

```cpp
class DAAbstractNodePlugin : public DAAbstractPlugin
{
    Q_OBJECT
public:
    // 获取节点工厂列表 - 返回插件提供的所有节点工厂
    virtual QList<DAAbstractNodeFactory*> getFactories() const = 0;
    
    // 注册节点元数据 - 将节点信息注册到主程序
    virtual void registerNodeMetaData(DAAbstractNodeFactory* factory) = 0;
};
```

工作流节点插件需要实现上述方法，将自定义节点注册到系统中，使其出现在节点列表中供用户使用。

### DAAbstractNodeFactory

节点工厂基类，负责创建特定类型的节点实例。每个节点工厂管理一类相关的节点，如数据导入节点、数据处理节点等。

下面的代码展示了节点工厂的核心方法：

```cpp
class DAAbstractNodeFactory : public QObject
{
    Q_OBJECT
public:
    // 创建节点 - 根据元数据创建具体的节点实例
    virtual DAAbstractNode* create(const DANodeMetaData& meta) = 0;
    
    // 获取节点元数据列表 - 返回此工厂支持的所有节点类型信息
    virtual QList<DANodeMetaData> getNodeMetaDataList() const = 0;
    
    // 工厂信息 - 显示在节点列表分组中
    virtual QString getFactoryName() const = 0;        // 工厂名称
    virtual QString getFactoryDescription() const = 0; // 工厂描述
    
    // 生命周期钩子 - 节点加入/移除工作流时的回调
    virtual void nodeAddedToWorkflow(DAAbstractNode* node);  // 节点加入前
    virtual void nodeStartRemove(DAAbstractNode* node);      // 节点移除前
};
```

节点工厂是插件功能的载体，通过工厂可以创建和管理多个不同类型的节点。

### DAAbstractNode

节点基类，工作流中的处理单元。每个节点代表一个数据处理步骤，负责执行具体的业务逻辑。

下面的代码展示了节点基类的核心方法：

```cpp
class DAAbstractNode : public QObject
{
    Q_OBJECT
public:
    // 执行节点（核心方法）- 实现具体的数据处理逻辑
    virtual bool exec() = 0;
    
    // 创建图元 - 返回节点的可视化显示对象
    virtual DAAbstractNodeGraphicsItem* createGraphicsItem() = 0;
    
    // 数据访问 - 获取输入数据和设置输出数据
    QVariant getInputData(const QString& key) const;   // 获取输入端口数据
    void setOutputData(const QString& key, const QVariant& data); // 设置输出端口数据
    
    // 连接点 - 获取节点的输入输出端口列表
    QStringList getInputKeys() const;   // 输入端口名称列表
    QStringList getOutputKeys() const;  // 输出端口名称列表
    
    // 序列化 - 支持节点状态的保存和恢复
    virtual QVariant saveToVariant() const;      // 保存节点状态
    virtual void loadFromVariant(const QVariant& var); // 加载节点状态
};
```

节点开发者需要在 `exec()` 方法中实现数据处理逻辑，并通过输入输出端口传递数据。

## 工作流 API

### DAWorkFlow

工作流管理类，负责工作流的创建、执行和节点管理。工作流以有向图形式组织数据处理流程。

下面的代码展示了工作流的核心方法：

```cpp
class DAWorkFlow : public QObject
{
    Q_OBJECT
public:
    // 创建节点 - 根据元数据在工作流中创建节点实例
    DAAbstractNode* createNode(const DANodeMetaData& meta);
    
    // 节点管理 - 获取和删除工作流中的节点
    QList<DAAbstractNode*> getNodes() const;  // 获取所有节点
    void removeNode(DAAbstractNode* node);    // 删除指定节点
    
    // 连接管理 - 在节点之间建立数据传递连接
    bool linkNodes(DAAbstractNode* from, const QString& fromKey,
                   DAAbstractNode* to, const QString& toKey);
    
    // 执行控制 - 启动和停止工作流执行
    void exec();  // 执行工作流
    void stop();  // 停止执行
    
signals:
    void startExecute();  // 开始执行信号
    void nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state); // 节点执行完成
    void finished(bool success);  // 工作流执行完成
};
```

工作流按照节点拓扑顺序自动执行，支持进度回调和错误处理。

### DANodeMetaData

节点元数据，描述节点的固定属性，包括名称、图标、连接点等。用于节点列表显示和节点创建。

下面的代码展示了节点元数据的属性和方法：

```cpp
class DANodeMetaData
{
public:
    // 基本属性 - 节点显示信息
    QString prototype() const;    // 唯一标识符，如 "Plugin.Factory.NodeName"
    QString name() const;         // 显示名称
    QString group() const;        // 分组名称
    QIcon icon() const;           // 显示图标
    QString description() const;  // 描述信息
    
    // 连接点定义 - 节点的输入输出端口
    void addInputKey(const QString& key, const QString& displayName);   // 添加输入端口
    void addOutputKey(const QString& key, const QString& displayName);  // 添加输出端口
    QStringList getInputKeys() const;   // 获取输入端口列表
    QStringList getOutputKeys() const;  // 获取输出端口列表
};
```

节点元数据在节点工厂初始化时创建，用于向系统注册节点类型信息。

## 数据 API

### DADataManagerInterface

数据管理接口，负责数据对象的创建、管理和查询。通过此接口，插件可以访问和操作项目中的所有数据。

下面的代码展示了数据管理接口的核心方法：

```cpp
class DADataManagerInterface : public QObject
{
    Q_OBJECT
public:
    // 数据操作 - 添加、删除、获取数据对象
    void addData(DADataObject* data);           // 添加数据对象
    void removeData(const QString& name);       // 删除数据对象
    DADataObject* getData(const QString& name) const; // 获取指定名称的数据
    
    // 数据查询 - 获取数据列表和当前选中数据
    QList<DADataObject*> getAllData() const;    // 获取所有数据
    DADataObject* getSelectedData() const;      // 获取当前选中的数据
    
signals:
    void dataAdded(DADataObject* data);         // 数据添加信号
    void dataRemoved(const QString& name);      // 数据删除信号
    void dataChanged(DADataObject* data);       // 数据变更信号
};
```

数据管理器使用信号槽机制通知数据变更，插件可以监听这些信号实现响应式更新。

### DADataPackage

数据包类，用于在工作流节点间传递数据。支持 DataFrame 等多种数据类型的封装和序列化。

下面的代码展示了数据包的核心方法：

```cpp
class DADataPackage
{
public:
    // DataFrame 操作 - pandas DataFrame 的包装
    void setDataFrame(const py::object& df);  // 设置 DataFrame
    py::object getDataFrame() const;          // 获取 DataFrame
    bool hasDataFrame() const;                // 检查是否包含 DataFrame
    
    // 序列化 - 支持数据的保存和恢复
    QVariant serialize() const;               // 序列化为 QVariant
    void deserialize(const QVariant& data);   // 从 QVariant 反序列化
};
```

数据包是节点间数据传递的标准格式，支持隐式共享以减少数据拷贝。

## 图形视图 API

### DANodeGraphicsScene

工作流场景，管理工作流的可视化显示。负责节点图元的创建、连接线的绘制和用户交互处理。

下面的代码展示了工作流场景的核心方法：

```cpp
class DANodeGraphicsScene : public DAGraphicsScene
{
    Q_OBJECT
public:
    // 工作流设置 - 关联工作流数据模型
    void setWorkFlow(DAWorkFlow* workflow);  // 设置工作流
    DAWorkFlow* getWorkFlow() const;         // 获取当前工作流
    
    // 节点创建 - 在场景中创建节点图元
    DAAbstractNodeGraphicsItem* createNode(const DANodeMetaData& meta,
                                            const QPointF& pos);
    
    // 连接操作 - 在节点间添加连接线
    DAAbstractNodeLinkGraphicsItem* addNodeLink_(
        DAAbstractNodeGraphicsItem* from, const DANodeLinkPoint& fromPoint,
        DAAbstractNodeGraphicsItem* to, const DANodeLinkPoint& toPoint);
    
signals:
    void nodeItemLinkPointSelected(DAAbstractNodeGraphicsItem* item,
                                   const DANodeLinkPoint& lp); // 连接点选中信号
};
```

场景是图形视图的核心，协调图元显示和工作流逻辑的交互。

## 日志 API

### 日志函数

DAWorkBench 使用 spdlog 提供高性能日志系统，支持多级别日志输出。

下面的代码展示了可用的日志宏：

```cpp
// 日志级别 - 从低到高的日志输出宏
DA_LOG_TRACE("trace message {}", arg);    // 跟踪级别 - 最详细的调试信息
DA_LOG_DEBUG("debug message {}", arg);    // 调试级别 - 开发调试信息
DA_LOG_INFO("info message {}", arg);      // 信息级别 - 正常运行信息
DA_LOG_WARNING("warning message {}", arg); // 警告级别 - 需要注意的问题
DA_LOG_ERROR("error message {}", arg);    // 错误级别 - 错误但可恢复
DA_LOG_CRITICAL("critical message {}", arg); // 严重级别 - 致命错误
```

日志系统支持格式化输出，使用 `{}` 作为占位符，与 spdlog 格式一致。建议在节点执行的关键位置添加日志，便于问题排查。

## 模块列表

| 模块 | 说明 | 主要类 |
|------|------|--------|
| DAUtils | 工具模块 | 配置、日志、文件处理 |
| DAWorkFlow | 工作流模块 | DAWorkFlow, DAAbstractNode, DANodeMetaData |
| DAGraphicsView | 图形视图模块 | DAGraphicsScene, DAGraphicsView |
| DAFigure | 图表模块 | DAFigureWidget, DAChart |
| DAData | 数据模块 | DADataPackage, DADataManagerInterface |
| DAGui | 界面模块 | DARibbonArea, DADockingArea |
| DAInterface | 接口模块 | DACoreInterface, DAAppUIInterface |
| DAPluginSupport | 插件模块 | DAAbstractPlugin, DAPluginManager |

## 下一步

- [:material-file-document: Doxygen 文档](../doxygen/index.html) - 完整 API 文档
- [:material-book: 开发指南](./dev-guide/coding-standard.md) - 编码规范
- [:material-puzzle: 插件开发](./plugin-development.md) - 插件开发指南