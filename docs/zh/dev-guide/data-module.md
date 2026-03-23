# 数据模块 DAData

数据模块 `DAData` 负责管理 DAWorkBench 中所有数据对象的创建、存储、查询和操作。它是连接 Python 数据处理能力和 C++ GUI 框架的核心桥梁。

## 模块依赖

`DAData` 模块依赖以下模块：

- `DAUtils` - 基础工具类
- `DAPyBindQt` - Python与Qt绑定
- `DAPyScripts` - Python脚本包装
- `Qt::Core`、`Qt::Gui`、`Qt::Widgets`

## 核心类

### DAData

`DAData` 是数据对象的智能指针包装类，是整个数据模块对外暴露的核心类型。

特点：

- 使用隐式共享（Implicit Sharing），可安全地进行值传递
- 可放入 `QMap`、`QHash` 等容器中
- 支持与 Python 对象的互相转换

```cpp
// 创建一个包含 DataFrame 的数据对象
DA::DAPyDataFrame df(pyObject);
DA::DAData data(df);
data.setName("sensor_data");
data.setDescribe("传感器原始数据");
```

### DAAbstractData

`DAAbstractData` 是所有数据类型的抽象基类，定义了数据对象的基本接口：

- `getName()` / `setName()` - 数据名称
- `getDescribe()` / `setDescribe()` - 数据描述
- `getDataType()` - 获取数据类型

### 数据类型继承体系

```
DAAbstractData (抽象基类)
└── DADataPyObject (Python对象包装)
    ├── DADataPyDataFrame (pandas DataFrame)
    └── DADataPySeries (pandas Series)
```

| 类名 | 说明 |
|------|------|
| `DADataPyObject` | 通用Python对象的包装类 |
| `DADataPyDataFrame` | pandas DataFrame 的包装类，支持行列操作 |
| `DADataPySeries` | pandas Series 的包装类 |

### DADataManager

`DADataManager` 是数据管理器，负责管理所有 `DAData` 对象的生命周期。

主要功能：

- 添加/删除数据（支持 Undo/Redo）
- 按名称或正则表达式查找数据
- 数据改名、改值、改描述
- 脏标记管理（标识数据是否已修改）
- 信号通知机制

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

### 信号机制

`DADataManager` 提供以下信号用于通知数据变化：

| 信号 | 说明 |
|------|------|
| `dataAdded(const DAData&)` | 数据添加时触发 |
| `dataRemoved(const DAData&)` | 数据删除时触发 |
| `dataChanged(const DAData&)` | 数据内容变化时触发 |
| `datasCleared()` | 所有数据被清空时触发 |

## 支持的文件格式

通过 Python 的 pandas 库，DAData 模块支持以下文件格式的读写：

| 格式 | 扩展名 | 读取 | 写入 |
|------|--------|------|------|
| CSV | `.csv` | 支持 | 支持 |
| Excel | `.xlsx` / `.xls` | 支持 | 支持 |
| 文本文件 | `.txt` | 支持 | 支持 |
| Pickle | `.pkl` | 支持 | 支持 |

## 与其他模块的交互

### 与工作流模块

工作流节点可以通过输入/输出端口传递 `DAData` 对象，节点之间的数据流动通过数据管理器统一协调。

### 与绘图模块

绘图模块从数据管理器获取 `DADataPyDataFrame` 或 `DADataPySeries` 对象，将其转换为图表数据系列进行可视化。

### 与插件

插件通过 `DADataManagerInterface` 接口访问数据管理器，可以添加、修改、删除数据，也可以监听数据变化信号进行响应。

```cpp
// 在插件中获取数据管理器
DACoreInterface* c = core();
DADataManagerInterface* dataMgr = c->getDataManagerInterface();

// 监听数据变化
connect(dataMgr, &DADataManagerInterface::dataAdded,
        this, &MyPlugin::onDataAdded);
```
