# 最佳实践与常见问题

本指南汇集插件开发的最佳实践、性能优化建议、常见问题解答，帮助开发者高效开发高质量插件。

## 主要功能特性

**特性**

- ✅ **架构设计原则**：单一职责、松耦合、高内聚、可配置的设计原则指导
- ✅ **代码组织规范**：推荐的插件文件目录结构和命名规范
- ✅ **资源管理实践**：智能指针使用、线程安全设计的最佳实践
- ✅ **性能优化建议**：数据处理、缓存策略、内存管理、UI 性能优化技巧
- ✅ **错误处理规范**：统一错误处理和异常处理策略
- ✅ **常见问题解答**：构建、插件加载、节点开发、UI 扩展等常见问题及解决方案

## 插件开发最佳实践

### 1. 架构设计原则

| 原则 | 说明 | 实现建议 |
|------|------|----------|
| **单一职责** | 每个插件专注于单一功能领域 | 按功能拆分多个插件 |
| **松耦合** | 插件间通过接口通信，无直接依赖 | 使用 `DACoreInterface` |
| **高内聚** | 相关功能集中在一个插件内 | 统一的节点工厂管理 |
| **可配置** | 关键参数可通过配置文件调整 | 提供 GUI 配置界面 |

### 2. 代码组织规范

推荐的插件文件组织方式遵循统一的目录结构，便于维护和团队协作。

下面的目录结构展示了标准插件的组织方式：

```text
// 推荐的文件组织方式
MyPlugin/
├── MyPlugin.h/cpp            # 插件主类 - 必须的入口文件
├── MyNodeFactory.h/cpp       # 节点工厂（单文件） - 工作流插件必须
├── Nodes/                    # 节点实现目录 - 多个节点可分文件存放
│   ├── DataProcessNode.h/cpp # 数据处理节点
│   ├── TransformNode.h/cpp   # 数据转换节点
│   └── ExportNode.h/cpp      # 数据导出节点
├── UI/                       # 界面目录 - Dock 窗口、对话框等
│   ├── MyDockWidget.h/cpp/ui # Dock 窗口
│   ├── ConfigDialog.h/cpp/ui # 配置对话框
│   └── NodeConfigWidget.h/cpp/ui  # 节点配置控件
├── Utils/                    # 工具类目录 - 辅助功能类
│   ├── DataConverter.h/cpp   # 数据转换工具
│   ├── AlgorithmHelper.h/cpp # 算法辅助类
├── Resources/                # 资源文件
│   ├── MyPlugin.qrc          # Qt 资源文件
│   ├── translations/         # 翻译文件目录
│   │   └── myplugin_zh.qm    # 中文翻译
│   └── icons/                # 图标目录
│       └── *.png             # 各尺寸图标
└── CMakeLists.txt            # 构建配置文件
```

上述结构将插件功能按职责分离，便于代码维护和团队协作。

### 3. 资源管理最佳实践

!!! tip "使用智能指针"
    优先使用 Qt 智能指针管理动态对象，避免手动内存管理导致的泄漏和错误。

下面的代码展示了智能指针的正确使用方式：

```cpp
// 推荐：使用 QSharedPointer 自动管理内存
class MyPlugin : public DA::DAAbstractNodePlugin
{
private:
    QSharedPointer<MyDataCache> m_cache;        // 数据缓存智能指针
    QSharedPointer<MyAlgorithmEngine> m_engine; // 算法引擎智能指针
};

// 初始化时创建智能指针 - 使用 ::create() 静态方法
m_cache = QSharedPointer<MyDataCache>::create();

// 自动释放，无需手动 delete，离开作用域自动析构

// 避免：裸指针需要手动管理，容易忘记 delete
MyDataCache* m_cache;  // 需要手动 delete，易出错
```

智能指针确保资源在对象生命周期结束时自动释放，防止内存泄漏。

### 4. 线程安全设计

!!! warning "工作流执行线程"
    工作流执行在独立线程，节点 `exec()` 方法不要直接操作 UI，否则会导致崩溃。

下面的代码展示了正确的线程安全处理方式：

```cpp
bool MyWorker::exec()
{
    // ❌ 错误：直接操作 UI - 会崩溃！
    QMessageBox::information(nullptr, "Info", "Processing complete");
    
    // ✅ 正确：使用信号通知主线程
    emit processingComplete(this);  // 主线程接收到信号后处理 UI
    
    return true;
}
```

节点执行时处于工作流线程，UI 操作必须在主线程执行，应使用信号槽机制进行跨线程通信。

## 性能优化建议

### 1. 数据处理优化

对于超过 10 万行的大型数据，建议使用分块处理方式，避免内存占用过高和 UI 卡顿。

下面的代码展示了大数据分块处理的实现方式：

```cpp
// 大数据分块处理 - 避免一次性加载全部数据
bool MyWorker::exec()
{
    DA::DADataPackage input = getInputData("input_data");
    
    // 获取 DataFrame - pandas DataFrame 对象
    auto df = input.getDataFrame();
    int rowCount = df.row_count();
    int chunkSize = 10000;  // 分块大小：每块处理 1 万行
    
    for (int i = 0; i < rowCount; i += chunkSize) {
        int end = std::min(i + chunkSize, rowCount);  // 计算当前块结束位置
        
        // 处理当前块 - 在 processChunk 中实现具体逻辑
        processChunk(df, i, end);
        
        // 更新进度（避免频繁更新）- 每 10 个块更新一次进度
        if (i % (chunkSize * 10) == 0) {
            emit progressUpdated(i * 100 / rowCount);  // 发送进度信号
        }
    }
    
    return true;
}
```

分块处理可以有效控制内存使用，同时定期更新进度让用户感知处理进度。

### 2. 缓存策略

使用智能缓存管理避免重复计算，提高处理效率。缓存应支持过期检查和大小限制。

下面的代码展示了带过期机制的缓存管理类：

```cpp
// 智能缓存管理 - 支持过期检查
class MyDataCache
{
public:
    // 缓存命中检查 - 检查是否存在且未过期
    bool hasCache(const QString& key) const
    {
        return m_cache.contains(key) && !isExpired(key);
    }
    
    // 缓存过期检查 - 根据缓存时间判断是否过期
    bool isExpired(const QString& key) const
    {
        QDateTime cacheTime = m_cacheTime[key];  // 获取缓存创建时间
        return cacheTime.addSecs(m_cacheTimeoutSec) < QDateTime::currentDateTime();
    }
    
private:
    QMap<QString, QVariant> m_cache;      // 缓存数据存储
    QMap<QString, QDateTime> m_cacheTime; // 缓存时间记录
    int m_cacheTimeoutSec = 3600;         // 缓存过期时间：1 小时
};
```

缓存策略应根据数据特性设置合理的过期时间和大小限制，避免占用过多内存。

### 3. 内存管理

避免不必要的深拷贝，利用 Qt 的隐式共享机制减少内存占用。

下面的代码展示了正确的内存管理方式：

```cpp
// 避免不必要的深拷贝 - 使用引用或 Qt 隐式共享
bool MyWorker::exec()
{
    // ❌ 错误：深拷贝 - 复制整个 DataFrame，内存翻倍
    DA::DADataPackage copy = input;
    
    // ✅ 正确：引用或浅拷贝
    const DA::DADataPackage& ref = input;
    
    // 处理完成后只存储必要的结果
    QVariant result;
    result.setValue(ref);  // Qt 的隐式共享
    
    setOutputData("output_data", result);
}
```

### 4. UI 性能优化

减少 UI 刷新频率，使用批量更新模式避免频繁重绘导致的界面卡顿。

下面的代码展示了正确的 UI 批量更新方式：

```cpp
// 减少 UI 刷新频率 - 使用批量更新模式
void MyDockWidget::updateDataView()
{
    // ❌ 错误：每条数据刷新一次 - 导致界面卡顿
    for (int i = 0; i < rowCount; ++i) {
        m_tableView->updateRow(i);  // 每次触发重绘，太频繁！
    }
    
    // ✅ 正确：批量刷新 - 先暂停再一次性更新
    m_tableView->setUpdatesEnabled(false);  // 暂停刷新信号
    m_tableView->setModel(newModel);        // 设置新模型
    m_tableView->setUpdatesEnabled(true);   // 恢复刷新，触发一次性重绘
}
```

批量更新通过暂时禁用刷新，在所有数据加载完成后再一次性更新，显著提升性能。

## 错误处理规范

### 1. 统一错误处理

节点执行应使用统一的错误处理流程，包括输入验证、执行处理、输出验证和异常捕获。

下面的代码展示了标准的节点错误处理实现：

```cpp
// 统一错误处理流程 - 包含验证、执行、异常捕获
bool MyWorker::exec()
{
    DA_LOG_INFO("Node {} starting execution", getID());  // 记录开始
    
    try {
        // 输入验证 - 检查输入数据有效性
        if (!validateInput()) {
            DA_LOG_ERROR("Node {}: Invalid input", getID());  // 记录错误
            setStatus(DA::DAAbstractNode::StatusError);       // 设置错误状态
            return false;
        }
        
        // 执行处理 - 核心业务逻辑
        processData();
        
        // 输出验证 - 检查输出数据有效性
        if (!validateOutput()) {
            DA_LOG_ERROR("Node {}: Invalid output", getID());
            setStatus(DA::DAAbstractNode::StatusError);
            return false;
        }
        
        DA_LOG_INFO("Node {} completed successfully", getID());  // 记录成功
        setStatus(DA::DAAbstractNode::StatusFinished);           // 设置完成状态
        return true;
        
    } catch (const std::exception& e) {
        DA_LOG_ERROR("Node {}: Exception - {}", getID(), e.what());  // 捕获异常
        setStatus(DA::DAAbstractNode::StatusError);
        return false;
    }
}
```

统一的错误处理流程确保问题能被正确记录和追踪，便于后续调试。

### 2. 异常处理策略

分级异常处理根据异常类型采取不同的处理策略，区分可恢复错误和严重错误。

下面的代码展示了分级异常处理方式：

```cpp
// 分级异常处理 - 根据异常类型采取不同策略
void MyPlugin::handleOperation()
{
    try {
        performOperation();
        
    } catch (const std::runtime_error& e) {
        // 可恢复错误 - 显示警告，继续运行
        DA_LOG_WARNING("Runtime error: {}", e.what());
        showWarningToUser(tr("Operation warning: %1").arg(e.what()));
        
    } catch (const std::logic_error& e) {
        // 逻辑错误 - 需要修复，显示错误信息
        DA_LOG_ERROR("Logic error: {}", e.what());
        showErrorToUser(tr("Internal error: %1").arg(e.what()));
        
    } catch (const std::exception& e) {
        // 未预期的错误 - 记录严重错误
        DA_LOG_CRITICAL("Unexpected error: {}", e.what());
        showErrorToUser(tr("Unexpected error occurred"));
    }
}
        // 可恢复错误
        DA_LOG_WARNING("Runtime error: {}", e.what());
        showWarningToUser(tr("Operation warning: %1").arg(e.what()));
        
    } catch (const std::logic_error& e) {
        // 逻辑错误，需要修复
        DA_LOG_ERROR("Logic error: {}", e.what());
        showErrorToUser(tr("Internal error: %1").arg(e.what()));
        
    } catch (const std::exception& e) {
        // 未预期的错误
        DA_LOG_CRITICAL("Unexpected error: {}", e.what());
        showErrorToUser(tr("Unexpected error occurred"));
    }
}
```

## 常见问题解答 (FAQ)

### 构建相关问题

#### Q1: 找不到 DAWorkbench 模块

**问题**：CMake 报错 `Could not find DAWorkbench`

**解决**：

1. 确保 data-workbench 已正确构建和安装
2. 检查 `DAWorkbench_DIR` 设置是否正确
3. 安装路径格式应为 `bin_Release_qtX.X_MSCV_x64`

```cmake
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")
```

#### Q2: Qt 版本不匹配

**问题**：插件使用 Qt5，主程序使用 Qt6

**解决**：确保插件和主程序使用相同的 Qt 版本

```cmake
# 检查 Qt 版本
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
message(STATUS "Using Qt version: ${QT_VERSION}")
```

### 插件加载问题

#### Q3: 插件加载失败

**问题**：插件目录中有 .dll/.so 文件，但未加载

**排查步骤**：

1. 检查插件是否正确导出接口
```cpp
Q_PLUGIN_METADATA(IID "Plugin.YourPlugin")
Q_INTERFACES(DA::DAAbstractNodePlugin)
```

2. 检查 initialize() 是否返回 true
```cpp
bool MyPlugin::initialize()
{
    // 所有初始化必须成功
    return true;  // 不能返回 false
}
```

3. 检查依赖库是否在 bin 目录

#### Q4: 接口访问返回 nullptr

**问题**：`core()` 或 `getUiInterface()` 返回空指针

**解决**：

1. 确保在 `initialize()` 之后访问接口
2. 不要在构造函数中访问接口

```cpp
MyPlugin::MyPlugin()
{
    // ❌ 错误：构造函数中访问接口
    DACoreInterface* core = this->core();  // nullptr!
}

bool MyPlugin::initialize()
{
    // ✅ 正确：initialize() 中访问
    DACoreInterface* core = this->core();  // 有效
}
```

### 节点开发问题

#### Q5: 节点不显示在节点列表中

**问题**：节点工厂已创建，但节点不显示

**解决**：

1. 检查 `getNodeMetaDataList()` 返回值
```cpp
QList<DA::DANodeMetaData> MyNodeFactory::getNodeMetaDataList() const
{
    return m_nodePrototypes.values();  // 确保返回有效数据
}
```

2. 检查节点元数据是否正确注册
```cpp
void MyNodeFactory::registerNodePrototypes()
{
    DA::DANodeMetaData meta;
    meta.setPrototype("Unique.Prototype.Name");  // 必须唯一
    // ...
    m_nodePrototypes[meta.prototype()] = meta;
}
```

#### Q6: 节点执行崩溃

**问题**：执行工作流时节点崩溃

**常见原因**：

1. 在 `exec()` 中操作 UI
2. 访问无效的输入数据
3. 未处理空指针

```cpp
bool MyWorker::exec()
{
    // 验证输入
    QVariant input = getInputData("input_data");
    if (!input.isValid()) {
        return false;  // 安全返回
    }
    
    // 不操作 UI
    // 不访问可能为空的指针
}
```

### UI 扩展问题

#### Q7: Ribbon 图标不显示

**问题**：添加的 Action 图标不显示

**解决**：

1. 检查图标路径是否正确
2. 确保资源文件已加载

```cpp
// 检查资源加载
QIcon icon(":/icon/myicon.png");
if (icon.isNull()) {
    DA_LOG_WARNING("Icon not found: :/icon/myicon.png");
}
```

#### Q8: Dock 窗口位置不对

**问题**：Dock 窗口出现在非预期位置

**解决**：

```cpp
// 明确指定位置
dock->addDockWidget(m_widget, 
                    tr("My Window"), 
                    Qt::RightDockWidgetArea,  // 指定区域
                    "myplugin.dock.widget");
```

## 兼容性注意事项

### 跨版本适配

```cpp
// 配置版本兼容
void MyPlugin::loadConfig()
{
    QJsonObject config = readConfigFile();
    
    int version = config["version"].toInt(1);
    
    // 适配不同版本
    if (version == 1) {
        // V1 格式
        m_setting = config["setting_name"].toString();
    } else if (version >= 2) {
        // V2+ 格式（字段名变更）
        m_setting = config["new_setting_name"].toString();
    }
}
```

### Qt 版本兼容

```cpp
// Qt5/Qt6 兼容代码
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 代码
    find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)
#else
    // Qt5 代码
    find_package(Qt5 COMPONENTS OpenGL REQUIRED)
#endif
```

## 贡献指南

### 提交 PR 前检查清单

- [ ] 代码符合编码规范
- [ ] 所有新增类添加文档注释
- [ ] 提交信息清晰描述变更内容
- [ ] 已添加必要的单元测试
- [ ] 已测试 Qt5 和 Qt6 兼容性
- [ ] 已处理多语言支持
- [ ] 无内存泄漏风险
- [ ] 线程安全已验证

### 代码风格要求

```cpp
// 命名规范
class MyClass;          // PascalCase
void myFunction();      // camelCase
int m_memberVariable;   // m_ prefix + camelCase
#define MY_CONSTANT     // UPPER_CASE

// 注释规范
/**
 * @brief 简短描述
 * @param param1 参数说明
 * @return 返回值说明
 * @note 注意事项
 */
```

## 下一步

- [:material-book-open: 编码规范](./dev-guide/coding-standard.md) - 详细编码规范
- [:material-api: API 文档](./api-reference.md) - API 参考手册
- [:material-github: 贡献指南](./contribution-guide.md) - 参与开源贡献