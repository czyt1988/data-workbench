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
    virtual DAUIInterface* getUiInterface() const = 0;

    // 获取项目管理接口 - 用于项目创建、打开、保存等操作
    virtual DAProjectInterface* getProjectInterface() const = 0;

    // 获取数据管理接口 - 用于数据对象的增删改查
    virtual DADataManagerInterface* getDataManagerInterface() const = 0;

    // 获取 Agent 接口 - 用于 AI 分析子系统（LLM 配置/会话/提示词库/工具注册）
    virtual DAAgentInterface* getAgentInterface() const = 0;
};
```

上述接口是插件开发的基石，通过 `core()` 方法获取 DACoreInterface 实例后，即可访问主程序的所有功能模块，包括通过 `getAgentInterface()` 获取的 `DAAgentInterface`（AI 分析子系统入口，定义于 `src/DAAgent/DAAgentInterface.h`）。

### DAUIInterface

UI 相关接口，提供对主程序界面组件的访问能力。通过此接口，插件可以扩展 Ribbon 工具栏、添加 Dock 窗口、注册 Action 等。

下面的代码展示了 DAUIInterface 的核心方法定义：

```cpp
class DAUIInterface
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
class DAAbstractPlugin   // 注意：不继承 QObject，通过 Q_DECLARE_INTERFACE 注册 IID
{
public:
    // 插件元信息 - 纯虚函数，必须实现
    virtual QString getIID() const = 0;           // 接口标识符
    virtual QString getName() const = 0;          // 插件名称
    virtual QString getVersion() const = 0;       // 插件版本
    virtual QString getDescription() const = 0;   // 插件描述

    // 生命周期 - 默认实现，按需重载
    virtual bool initialize();                    // 默认返回 true
    virtual bool finalize();                       // 默认返回 true
    virtual void retranslate();                    // 语言变更回调，默认空实现
    virtual DAAbstractSettingPage* createSettingPage();          // 默认返回 nullptr
    virtual std::shared_ptr<DAAbstractArchiveTask> createArchiveTask(bool isSave);  // 默认返回 nullptr

    // 获取核心接口 - 插件访问主程序功能的唯一入口
    DACoreInterface* core() const;
};
```

插件开发者需实现元信息方法（`getIID`/`getName`/`getVersion`/`getDescription`），并按需重载 `initialize()` 完成资源初始化、节点注册、界面设置等工作；通过 `core()` 获取 `DACoreInterface` 进而访问主程序功能。导出插件还需实现 `plugin_create()` / `plugin_destory()` 两个 C 函数并使用 `Q_DECLARE_INTERFACE`/`Q_PLUGIN_METADATA` 注册。

### DAAbstractNodePlugin

节点插件基类，继承自 DAAbstractPlugin，用于提供工作流节点的插件。此类扩展了节点工厂管理功能。

下面的代码展示了节点插件基类的核心定义：

```cpp
class DAAbstractNodePlugin : public DAAbstractPlugin
{
    Q_OBJECT
public:
    // 创建节点工厂 - 插件提供 DAPyNodeFactory 实例
    virtual DAPyNodeFactory* createNodeFactory() = 0;
    
    // 销毁节点工厂 - 释放工厂资源
    virtual void destroyNodeFactory(DAPyNodeFactory* p) = 0;
    
    // 节点加载完成回调 - 所有节点加载后调用
    virtual void afterLoadedNodes();
};
```

工作流节点插件需要实现上述方法，创建 DAPyNodeFactory 实例以注册节点到系统中。

### DAPyNodeFactory

节点工厂代理类，封装 Python 层的 `DANodeFactory` 对象。每个工厂管理一组通过 `@NodeDef` 装饰器定义的节点类型，负责节点元数据的注册和节点实例的创建。

下面的代码展示了节点工厂的核心方法：

```cpp
class DAPyNodeFactory : public DAPyObjectWrapper
{
public:
    // 获取工厂信息
    QString getFactoryName() const;        // 工厂名称
    QString getFactoryDescription() const; // 工厂描述

    // 节点元数据 - 返回此工厂管理的所有节点类型
    QList<DAPyNodeMetaData> getNodeMetaDataList() const;

    // 创建节点 - 根据 qualifiedName 创建 Python 节点代理
    DAPyNode* createNode(const QString& qualifiedName);
};
```

新的 Python-first 架构使用 `@NodeDef` 装饰器自动发现节点，无需手动编写 C++ 节点工厂。DAPyNodeFactory 作为 Python 层 `DANodeFactory` 的 C++ 代理。

### DAPyNode

Python 节点在 C++ 层的代理，继承 `DAPyObjectWrapper`（非 QObject），通过 `attr()` 实时从 Python 对象读取属性，不做本地缓存。

下面的代码展示了节点代理的核心方法：

```cpp
class DAPyNode : public DAPyObjectWrapper
{
public:
    // 节点标识
    QString getNodeId() const;          // 节点实例唯一 ID
    QString getQualifiedName() const;   // 类型标识 "pkg.module.ClassName"
    QString getNodeName() const;        // 显示名称
    QString getNodeCategory() const;    // 分类名称

    // 端口信息
    QList<QString> getInputKeys() const;   // 输入端口 key 列表
    QList<QString> getOutputKeys() const;  // 输出端口 key 列表

    // 样式与状态
    DAPyNodeStyle getNodeStyle() const;
    DAPyNodeState getNodeState() const;

    // 执行
    py::dict execute(py::dict inputs, py::dict params);
};
```

DAPyNode 是工作流执行的核心单元，所有节点业务逻辑均在 Python 层实现。

## 工作流 API

### DAPyWorkFlowManager

工作流管理器（QObject），负责工作流的创建、节点执行调度和信号协调。是 Python-first 架构中工作流执行的中央调度器。

下面的代码展示了工作流管理器的核心方法：

```cpp
class DAPyWorkFlowManager : public QObject
{
    Q_OBJECT
public:
    // 场景管理
    DAPyWorkFlowScene* getWorkFlowScene() const;

    // 执行控制
    void execute();                    // 按拓扑顺序执行工作流
    void cancelExecution();            // 取消执行

    // 节点管理
    DAPyNode* getNode(const QString& nodeId) const;

Q_SIGNALS:
    void executionFinished(bool success);               // 工作流执行完成
    void nodeExecuted(const QString& nodeId, bool ok);  // 节点执行完成
};
```

工作流按照节点拓扑顺序自动执行，支持进度回调和错误处理。

### DAPyNodeMetaData

节点元数据，描述节点的固定属性（名称、图标、连接点等）。由 Python `@NodeDef` 装饰器定义，C++ 侧通过 `DAPyNodeMetaData` 结构体承载。

下面的代码展示了节点元数据的核心属性和方法：

```cpp
class DAPyNodeMetaData
{
public:
    // 基本属性
    QString prototype() const;         // 唯一标识符
    QString name() const;              // 显示名称
    QString qualifiedName() const;     // 完全限定名 "pkg.module.ClassName"
    QString category() const;          // 分类
    QString icon() const;              // 图标路径

    // 连接点配置
    QList<DAPyLinkPoint> getInputPoints() const;   // 输入端口列表
    QList<DAPyLinkPoint> getOutputPoints() const;  // 输出端口列表

    // 节点样式
    DAPyNodeStyle nodeStyle() const;   // 样式配置
};
```

节点元数据用于节点列表显示、节点创建和序列化。

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

### DAData

数据包装类（`src/DAData/DAData.h`），用于在工作流节点间传递数据。它封装 `DAAbstractData` 智能指针，支持隐式共享以减少数据拷贝，可包装 DataFrame、Series、Python 对象等多种数据类型。

下面的代码展示了数据包装器的核心方法：

```cpp
class DAData
{
public:
    DAAbstractData::DataType getDataType() const;          // 数据类型（DataFrame/Series/DataPackage 等）
    bool isDataFrame() const;                              // 是否为 DataFrame
    bool isSeries() const;                                 // 是否为 Series
    bool isDataPackage() const;                            // 是否为 DataPackage 类型

    // 变量元信息
    QString getName() const;                               // 名称
    QString getDescribe() const;                           // 描述
    DAAbstractData::IdType id() const;                     // 数据 id
    std::pair<std::size_t, std::size_t> shape() const;     // 尺寸

    // 转换 - 转为底层 Python 对象
    DAPyDataFrame toDataFrame() const;
    DAPySeries toSeries() const;
    pybind11::object toPyObject() const;
    void setPyObject(const pybind11::object& obj);

    // 数据管理器关联
    DADataManager* getDataManager() const;                // 所属数据管理器
    bool isHaveDataManager() const;                       // 是否被管理器管理

    // 写文件
    static bool writeToFile(const DAData& data, const QString& filePath);
};
```

`DAData` 是节点间数据传递的标准包装格式，值类型语义、支持隐式共享；`DataPackage` 是 `DAAbstractData::DataType` 中的一个类型分类，并非独立类。

## 图形视图 API

### DAPyWorkFlowScene

Python 工作流场景管理类，继承自 DAGraphicsScene，负责在 Qt Graphics View 框架中渲染 Python 工作流节点及其连接关系。

下面的代码展示了工作流场景的核心方法：

```cpp
class DAPyWorkFlowScene : public DAGraphicsScene
{
    Q_OBJECT
public:
    // 节点管理
    DAPyNodeGraphicsItem* createPyNode(const DAPyNodeMetaData& metaData,
                                        const QPointF& pos);
    bool removePyNodeItem(DAPyNodeGraphicsItem* item);
    DAPyNodeGraphicsItem* findNodeItemById(const QString& nodeId) const;
    QList<DAPyNodeGraphicsItem*> getPyNodeItems() const;

    // 连接线管理
    DAPyLinkGraphicsItem* addPyNodeLink(DAPyNodeGraphicsItem* fromItem,
                                         const QString& fromOutputKey,
                                         DAPyNodeGraphicsItem* toItem,
                                         const QString& toInputKey);
    bool removePyNodeLink(DAPyLinkGraphicsItem* linkItem);
    QList<DAPyLinkGraphicsItem*> getPyNodeLinkItems() const;

    // 序列化
    bool saveToFile(const QString& filePath);
    bool loadFromFile(const QString& filePath);

Q_SIGNALS:
    void pyNodeItemCreated(DAPyNodeGraphicsItem* item);
    void pyNodeLinkCreated(DAPyLinkGraphicsItem* link);
    void pyNodeStateChanged(DAPyNodeGraphicsItem* item, DAPyNodeState state);
};
```

场景是图形视图的核心，协调节点图元显示和工作流逻辑的交互。

## 日志 API

### 日志宏

DAWorkBench 使用基于 spdlog 的日志系统，通过便捷宏输出日志。业务日志（`da.*` category）会自动进入 UI 日志窗口。

```cpp
#include "DALogCategory.h"

// 日志宏 - 从低到高
daDebug    << "debug message" << arg;     // 调试级别 - 写文件，不进 UI 窗口
daInfo     << "info message" << arg;      // 信息级别 - 正常运行信息，进 UI 窗口
daWarning  << "warning message" << arg;   // 警告级别 - 需要注意的问题，进 UI 窗口
daCritical << "critical message" << arg;  // 严重级别 - 致命错误，进 UI 窗口
```

日志宏支持 `<<` 链式输出，用法与 `qDebug()` / `qInfo()` 一致。建议在节点执行的关键位置添加日志，便于问题排查。

> 详见 [日志系统文档](../dev-guide/general/logging.md)。

## 模块列表

| 模块 | 说明 | 主要类 |
|------|------|--------|
| DAUtils | 工具模块 | 配置、日志、文件处理 |
| DAPyWorkFlow | 工作流模块 | DAPyWorkFlowManager, DAPyNode, DAPyNodeMetaData |
| DAGraphicsView | 图形视图模块 | DAGraphicsScene, DAGraphicsView |
| DAFigure | 图表模块 | DAFigureWidget, DAChart |
| DAData | 数据模块 | DAData, DADataManager, DAAbstractData |
| DAGui | 界面模块 | DARibbonArea, DADockingArea, DAMarkdownView |
| DAInterface | 接口模块 | DACoreInterface, DAUIInterface, DAAgentInterface |
| DAAgent | AI 分析模块 | DAAgentInterface, DAAgentModule, DAAbstractAgentTool |
| DAPluginSupport | 插件模块 | DAAbstractPlugin, DAPluginManager |

## 下一步

- [:material-file-document: Doxygen 文档](../doxygen/index.html) - 完整 API 文档
- [:material-book: 开发指南](../dev-guide/general/coding-standard.md) - 编码规范
- [:material-puzzle: 插件开发](../plugin/plugin-development.md) - 插件开发指南
