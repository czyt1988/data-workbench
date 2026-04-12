# API 文档索引

本页面提供 DAWorkBench API 文档的快速导航。

## Doxygen API 文档

完整的 API 文档由 Doxygen 生成，包含所有类、函数、枚举的详细说明。

- [:material-file-document: API 文档首页](../doxygen/index.html)
- [:material-format-list-bulleted: 类索引](../doxygen/classes.html)
- [:material-folder: 文件列表](../doxygen/files.html)

## 核心接口速览

### DACoreInterface

顶层接口，获取所有其他接口。

```cpp
class DACoreInterface
{
public:
    // 获取 UI 接口
    virtual DAAppUIInterface* getUiInterface() = 0;
    
    // 获取项目管理接口
    virtual DAProjectInterface* getProjectInterface() = 0;
    
    // 获取数据管理接口
    virtual DADataManagerInterface* getDataManagerInterface() = 0;
    
    // 获取工作流接口
    virtual DAWorkFlowInterface* getWorkFlowInterface() = 0;
};
```

### DAAppUIInterface

UI 相关接口。

```cpp
class DAAppUIInterface
{
public:
    // 获取 Ribbon 区域接口
    virtual DARibbonAreaInterface* getRibbonArea() = 0;
    
    // 获取 Dock 区域接口
    virtual DADockingAreaInterface* getDockingArea() = 0;
    
    // 获取 Actions 管理接口
    virtual DAActionsInterface* getActionInterface() = 0;
    
    // 获取命令接口
    virtual DACommandInterface* getCommandInterface() = 0;
    
    // 添加设置页面
    virtual void addSettingsPage(const QString& name, QWidget* page) = 0;
};
```

## 插件开发 API

### DAAbstractPlugin

插件基类。

```cpp
class DAAbstractPlugin : public QObject
{
    Q_OBJECT
public:
    // 获取核心接口
    DACoreInterface* core() const;
    
    // 插件信息
    virtual QString pluginName() const = 0;
    virtual QString pluginVersion() const = 0;
    virtual QString pluginDescription() const = 0;
    
    // 初始化（必须实现）
    virtual bool initialize() = 0;
    
    // 语言变更
    virtual void retranslate();
};
```

### DAAbstractNodePlugin

节点插件基类。

```cpp
class DAAbstractNodePlugin : public DAAbstractPlugin
{
    Q_OBJECT
public:
    // 获取节点工厂列表
    virtual QList<DAAbstractNodeFactory*> getFactories() const = 0;
    
    // 注册节点元数据
    virtual void registerNodeMetaData(DAAbstractNodeFactory* factory) = 0;
};
```

### DAAbstractNodeFactory

节点工厂基类。

```cpp
class DAAbstractNodeFactory : public QObject
{
    Q_OBJECT
public:
    // 创建节点
    virtual DAAbstractNode* create(const DANodeMetaData& meta) = 0;
    
    // 获取节点元数据列表
    virtual QList<DANodeMetaData> getNodeMetaDataList() const = 0;
    
    // 工厂信息
    virtual QString getFactoryName() const = 0;
    virtual QString getFactoryDescription() const = 0;
    
    // 生命周期钩子
    virtual void nodeAddedToWorkflow(DAAbstractNode* node);
    virtual void nodeStartRemove(DAAbstractNode* node);
};
```

### DAAbstractNode

节点基类。

```cpp
class DAAbstractNode : public QObject
{
    Q_OBJECT
public:
    // 执行节点（核心方法）
    virtual bool exec() = 0;
    
    // 创建图元
    virtual DAAbstractNodeGraphicsItem* createGraphicsItem() = 0;
    
    // 数据访问
    QVariant getInputData(const QString& key) const;
    void setOutputData(const QString& key, const QVariant& data);
    
    // 连接点
    QStringList getInputKeys() const;
    QStringList getOutputKeys() const;
    
    // 序列化
    virtual QVariant saveToVariant() const;
    virtual void loadFromVariant(const QVariant& var);
};
```

## 工作流 API

### DAWorkFlow

工作流管理类。

```cpp
class DAWorkFlow : public QObject
{
    Q_OBJECT
public:
    // 创建节点
    DAAbstractNode* createNode(const DANodeMetaData& meta);
    
    // 节点管理
    QList<DAAbstractNode*> getNodes() const;
    void removeNode(DAAbstractNode* node);
    
    // 连接管理
    bool linkNodes(DAAbstractNode* from, const QString& fromKey,
                   DAAbstractNode* to, const QString& toKey);
    
    // 执行
    void exec();
    void stop();
    
signals:
    void startExecute();
    void nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state);
    void finished(bool success);
};
```

### DANodeMetaData

节点元数据。

```cpp
class DANodeMetaData
{
public:
    // 基本属性
    QString prototype() const;
    QString name() const;
    QString group() const;
    QIcon icon() const;
    QString description() const;
    
    // 连接点
    void addInputKey(const QString& key, const QString& displayName);
    void addOutputKey(const QString& key, const QString& displayName);
    QStringList getInputKeys() const;
    QStringList getOutputKeys() const;
};
```

## 数据 API

### DADataManagerInterface

数据管理接口。

```cpp
class DADataManagerInterface : public QObject
{
    Q_OBJECT
public:
    // 数据操作
    void addData(DADataObject* data);
    void removeData(const QString& name);
    DADataObject* getData(const QString& name) const;
    
    // 数据查询
    QList<DADataObject*> getAllData() const;
    DADataObject* getSelectedData() const;
    
signals:
    void dataAdded(DADataObject* data);
    void dataRemoved(const QString& name);
    void dataChanged(DADataObject* data);
};
```

### DADataPackage

数据包类。

```cpp
class DADataPackage
{
public:
    // DataFrame 操作
    void setDataFrame(const py::object& df);
    py::object getDataFrame() const;
    bool hasDataFrame() const;
    
    // 序列化
    QVariant serialize() const;
    void deserialize(const QVariant& data);
};
```

## 图形视图 API

### DANodeGraphicsScene

工作流场景。

```cpp
class DANodeGraphicsScene : public DAGraphicsScene
{
    Q_OBJECT
public:
    // 工作流设置
    void setWorkFlow(DAWorkFlow* workflow);
    DAWorkFlow* getWorkFlow() const;
    
    // 节点创建
    DAAbstractNodeGraphicsItem* createNode(const DANodeMetaData& meta,
                                            const QPointF& pos);
    
    // 连接操作
    DAAbstractNodeLinkGraphicsItem* addNodeLink_(
        DAAbstractNodeGraphicsItem* from, const DANodeLinkPoint& fromPoint,
        DAAbstractNodeGraphicsItem* to, const DANodeLinkPoint& toPoint);
    
signals:
    void nodeItemLinkPointSelected(DAAbstractNodeGraphicsItem* item,
                                   const DANodeLinkPoint& lp);
};
```

## 日志 API

### 日志函数

```cpp
// 日志级别
DA_LOG_TRACE("trace message {}", arg);
DA_LOG_DEBUG("debug message {}", arg);
DA_LOG_INFO("info message {}", arg);
DA_LOG_WARNING("warning message {}", arg);
DA_LOG_ERROR("error message {}", arg);
DA_LOG_CRITICAL("critical message {}", arg);
```

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