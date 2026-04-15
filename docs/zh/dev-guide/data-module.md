# 数据模块 DAData

数据模块 `DAData` 负责管理 DAWorkBench 中所有数据对象的创建、存储、查询和操作，是连接 Python 数据处理能力和 C++ GUI 框架的核心桥梁。

## 主要功能特性

**特性**

- ✅ **隐式共享设计**：`DAData` 使用隐式共享，可安全地进行值传递
- ✅ **Python 对象包装**：完整封装 pandas DataFrame 和 Series
- ✅ **统一数据管理**：`DADataManager` 提供数据的增删改查和信号通知
- ✅ **多格式支持**：通过 pandas 支持 CSV、Excel、Pickle 等格式读写

## 模块依赖

`DAData` 模块依赖以下模块：

| 模块 | 说明 |
|------|------|
| `DAUtils` | 基础工具类 |
| `DAPyBindQt` | Python 与 Qt 绑定 |
| `DAPyScripts` | Python 脚本包装 |
| `Qt::Core`、`Qt::Gui`、`Qt::Widgets` | Qt 基础模块 |

## 核心类

### DAData

`DAData` 是数据对象的智能指针包装类，是整个数据模块对外暴露的核心类型。

以下代码展示了如何创建一个包含 DataFrame 的数据对象并设置名称和描述：

```cpp
// 创建一个包含 DataFrame 的数据对象
DA::DAPyDataFrame df(pyObject);
DA::DAData data(df);
data.setName("sensor_data");
data.setDescribe("传感器原始数据");
```

上述代码展示了 DAData 的基本用法：
- 创建 `DAPyDataFrame` 包装 Python DataFrame 对象
- 使用 `DAData` 创建数据对象并传入 DataFrame
- 通过 `setName()` 设置数据名称，用于标识和搜索
- 通过 `setDescribe()` 设置数据描述，用于说明数据来源

**核心特性**

- 使用隐式共享（Implicit Sharing），可安全地进行值传递
- 可放入 `QMap`、`QHash` 等容器中
- 支持与 Python 对象的互相转换

### 数据类型继承体系

下图展示了数据类型的类继承关系，从抽象基类到具体的 DataFrame 和 Series 包装类：

```mermaid
classDiagram
    class DAAbstractData {
        <<abstract>>
        +getName() QString
        +setName(QString)
        +getDescribe() QString
        +setDescribe(QString)
        +getDataType() int
    }
    
    class DADataPyObject {
        +toDataFrame() DAPyDataFrame
        +toSeries() DAPySeries
    }
    
    class DADataPyDataFrame {
        +rowCount() int
        +columnCount() int
        +columnNames() QStringList
    }
    
    class DADataPySeries {
        +size() int
        +values() QVariantList
    }
    
    DAAbstractData <|-- DADataPyObject
    DADataPyObject <|-- DADataPyDataFrame
DADataPyObject <|-- DADataPySeries
    ```

上图展示了数据类型的继承层次：
- `DAAbstractData` 是抽象基类，定义基本接口（名称、描述、数据类型）
- `DADataPyObject` 继承抽象基类，封装通用 Python 对象
- `DADataPyDataFrame` 继承 `DADataPyObject`，封装 pandas DataFrame
- `DADataPySeries` 继承 `DADataPyObject`，封装 pandas Series

| 类名 | 说明 |
|------|------|
| `DAAbstractData` | 所有数据类型的抽象基类，定义基本接口 |
| `DADataPyObject` | 通用 Python 对象的包装类 |
| `DADataPyDataFrame` | pandas DataFrame 的包装类，支持行列操作 |
| `DADataPySeries` | pandas Series 的包装类 |

### DADataManager

`DADataManager` 是数据管理器，负责管理所有 `DAData` 对象的生命周期。

**主要功能**

- 添加/删除数据（支持 Undo/Redo）
- 按名称或正则表达式查找数据
- 数据改名、改值、改描述
- 脏标记管理（标识数据是否已修改）
- 信号通知机制

以下代码展示了如何通过接口获取数据管理器并进行数据操作：

```cpp
// 通过接口获取数据管理器
DADataManagerInterface* mgr = core->getDataManager();

// 添加数据
mgr->addData(data);

// 按名称查找
DAData found = mgr->getDataByName("sensor_data");

// 遍历所有数据
for (int i = 0; i < mgr->getDataCount(); ++i) {
    DAData d = mgr->getData(i);
    // ...
}
```

上述代码展示了数据管理器的基本操作：
- 通过核心接口获取数据管理器实例
- 使用 `addData()` 添加数据对象
- 使用 `getDataByName()` 按名称查找数据
- 使用循环遍历所有数据，通过 `getData(i)` 获取指定索引的数据

### 信号机制

`DADataManager` 提供以下信号用于通知数据变化：

| 信号 | 参数 | 触发时机 |
|------|------|----------|
| `dataAdded(const DAData&)` | DAData | 数据添加时触发 |
| `dataRemoved(const DAData&)` | DAData | 数据删除时触发 |
| `dataChanged(const DAData&)` | DAData | 数据内容变化时触发 |
| `datasCleared()` | 无 | 所有数据被清空时触发 |

## 支持的文件格式

通过 Python 的 pandas 库，DAData 模块支持以下文件格式的读写：

| 格式 | 扩展名 | 读取 | 写入 |
|------|--------|------|------|
| CSV | `.csv` | 支持 | 支持 |
| Excel | `.xlsx` / `.xls` | 支持 | 支持 |
| 文本文件 | `.txt` | 支持 | 支持 |
| Pickle | `.pkl` | 支持 | 支持 |
| Parquet | `.parquet` | 支持 | 支持 |

## 与其他模块的交互

### 与工作流模块

工作流节点可以通过输入/输出端口传递 `DAData` 对象，节点之间的数据流动通过数据管理器统一协调。

下图展示了工作流节点通过数据管理器传递数据的流程：

```mermaid
flowchart LR
    subgraph Node1["节点1"]
        O1["输出端口"]
    end
    
    subgraph DM["数据管理器"]
        D["DAData"]
    end
    
    subgraph Node2["节点2"]
        I2["输入端口"]
    end
    
    O1 --> D
D --> I2
    ```

上图展示了数据在工作流节点间传递的方式：
- 节点1的输出端口将数据发送到数据管理器
- 数据管理器存储 DAData 对象
- 节点2的输入端口从数据管理器获取数据
- 数据管理器作为中间协调者，解耦了节点间的直接依赖

### 与绘图模块

绘图模块从数据管理器获取 `DADataPyDataFrame` 或 `DADataPySeries` 对象，将其转换为图表数据系列进行可视化。

### 与插件

插件通过 `DADataManagerInterface` 接口访问数据管理器，可以添加、修改、删除数据，也可以监听数据变化信号进行响应。

以下代码展示了插件如何获取数据管理器并监听数据变化信号：

```cpp
// 在插件中获取数据管理器
DACoreInterface* c = core();
DADataManagerInterface* dataMgr = c->getDataManagerInterface();

// 监听数据变化
connect(dataMgr, &DADataManagerInterface::dataAdded,
        this, &MyPlugin::onDataAdded);
```

上述代码展示了插件与数据管理器的交互方式：
- 通过 `core()` 获取核心接口，再获取数据管理接口
- 使用 Qt 信号槽机制监听 `dataAdded` 信号
- 当数据添加时，触发 `onDataAdded` 回调函数

## API 参考

### DAData 核心方法

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `setName(name)` | QString | void | 设置数据名称 |
| `getName()` | 无 | QString | 获取数据名称 |
| `setDescribe(desc)` | QString | void | 设置数据描述 |
| `getDescribe()` | 无 | QString | 获取数据描述 |
| `isDataFrame()` | 无 | bool | 是否为 DataFrame 类型 |
| `toDataFrame()` | 无 | DAPyDataFrame | 转换为 DataFrame |

### DADataManagerInterface 核心方法

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `addData(data)` | DAData | void | 立即添加数据 |
| `addData_(data)` | DAData | void | 添加数据（支持 undo） |
| `removeData(data)` | DAData | void | 立即删除数据 |
| `getDataCount()` | 无 | int | 获取数据数量 |
| `getData(index)` | int | DAData | 获取指定索引数据 |
| `getAllDatas()` | 无 | QList<DAData> | 获取所有数据 |
| `getSelectDatas()` | 无 | QList<DAData> | 获取选中数据 |

## 注意事项

!!! warning "Python GIL 管理"
    操作 Python 数据对象时，需要正确管理 GIL，避免多线程问题。

!!! tip "数据生命周期"
    `DAData` 使用隐式共享，复制操作不会深拷贝数据，修改会影响所有引用。

!!! note "数据类型检查"
    在使用 `toDataFrame()` 或 `toSeries()` 前，应先使用 `isDataFrame()` 或 `isSeries()` 检查类型。

## 参考资料

- [Python 集成指南](./python-in-cpp.md)
- [工作流系统概述](../workflow.md)
- [插件开发指南](./plugin-project-create.md)