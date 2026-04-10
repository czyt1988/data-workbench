# 最佳实践与常见问题

本指南汇集插件开发的最佳实践、性能优化建议、常见问题解答，帮助开发者高效开发高质量插件。

---

## 插件开发最佳实践

### 1. 架构设计原则

| 原则 | 说明 | 实现建议 |
|------|------|----------|
| **单一职责** | 每个插件专注于单一功能领域 | 按功能拆分多个插件 |
| **松耦合** | 插件间通过接口通信，无直接依赖 | 使用 `DACoreInterface` |
| **高内聚** | 相关功能集中在一个插件内 | 统一的节点工厂管理 |
| **可配置** | 关键参数可通过配置文件调整 | 提供 GUI 配置界面 |

### 2. 代码组织规范

```cpp
// 推荐的文件组织方式
MyPlugin/
├── MyPlugin.h/cpp            # 插件主类
├── MyNodeFactory.h/cpp       # 节点工厂（单文件）
├── Nodes/                    # 节点实现目录
│   ├── DataProcessNode.h/cpp
│   ├── TransformNode.h/cpp
│   └── ExportNode.h/cpp
├── UI/                       # 界面目录
│   ├── MyDockWidget.h/cpp/ui
│   ├── ConfigDialog.h/cpp/ui
│   └── NodeConfigWidget.h/cpp/ui
├── Utils/                    # 工具类目录
│   ├── DataConverter.h/cpp
│   ├── AlgorithmHelper.h/cpp
├── Resources/                # 资源文件
│   ├── MyPlugin.qrc
│   ├── translations/         # 翻译文件
│   │   └── myplugin_zh.qm
│   └── icons/                # 图标
│       └── *.png
└── CMakeLists.txt
```

### 3. 资源管理最佳实践

!!! tip "使用智能指针"
    优先使用 Qt 智能指针管理动态对象。

```cpp
// 推荐：使用 QSharedPointer
class MyPlugin : public DA::DAAbstractNodePlugin
{
private:
    QSharedPointer<MyDataCache> m_cache;
    QSharedPointer<MyAlgorithmEngine> m_engine;
};

// 初始化
m_cache = QSharedPointer<MyDataCache>::create();

// 自动释放，无需手动 delete

// 避免：裸指针
MyDataCache* m_cache;  // 需要手动 delete，易出错
```

### 4. 线程安全设计

!!! warning "工作流执行线程"
    工作流执行在独立线程，节点 `exec()` 方法不要直接操作 UI。

```cpp
bool MyWorker::exec()
{
    // ❌ 错误：直接操作 UI
    QMessageBox::information(nullptr, "Info", "Processing complete");  // 会崩溃！
    
    // ✅ 正确：使用信号通知
    emit processingComplete(this);
    
    return true;
}

// UI 线程处理信号
connect(worker, &MyWorker::processingComplete,
        this, &MyPlugin::onProcessingComplete,
        Qt::QueuedConnection);  // 跨线程连接
```

---

## 性能优化建议

### 1. 数据处理优化

```cpp
// 大数据分块处理
bool MyWorker::exec()
{
    DA::DADataPackage input = getInputData("input_data");
    
    // 获取 DataFrame
    auto df = input.getDataFrame();
    int rowCount = df.row_count();
    int chunkSize = 10000;  // 分块大小
    
    for (int i = 0; i < rowCount; i += chunkSize) {
        int end = std::min(i + chunkSize, rowCount);
        
        // 处理当前块
        processChunk(df, i, end);
        
        // 更新进度（避免频繁更新）
        if (i % (chunkSize * 10) == 0) {
            emit progressUpdated(i * 100 / rowCount);
        }
    }
    
    return true;
}
```

### 2. 缓存策略

```cpp
// 智能缓存管理
class MyDataCache
{
public:
    // 缓存命中检查
    bool hasCache(const QString& key) const
    {
        return m_cache.contains(key) && !isExpired(key);
    }
    
    // 缓存过期检查
    bool isExpired(const QString& key) const
    {
        QDateTime cacheTime = m_cacheTime[key];
        return cacheTime.addSecs(m_cacheTimeoutSec) < QDateTime::currentDateTime();
    }
    
private:
    QMap<QString, QVariant> m_cache;
    QMap<QString, QDateTime> m_cacheTime;
    int m_cacheTimeoutSec = 3600;  // 1小时过期
};
```

### 3. 内存管理

```cpp
// 避免不必要的深拷贝
bool MyWorker::exec()
{
    // ❌ 错误：深拷贝
    DA::DADataPackage copy = input;  // 复制整个 DataFrame
    
    // ✅ 正确：引用或浅拷贝
    const DA::DADataPackage& ref = input;
    
    // 处理完成后只存储必要的结果
    QVariant result;
    result.setValue(ref);  // Qt 的隐式共享
    
    setOutputData("output_data", result);
}
```

### 4. UI 性能优化

```cpp
// 减少 UI 刷新频率
void MyDockWidget::updateDataView()
{
    // ❌ 错误：每条数据刷新一次
    for (int i = 0; i < rowCount; ++i) {
        m_tableView->updateRow(i);  // 太频繁！
    }
    
    // ✅ 正确：批量刷新
    m_tableView->setUpdatesEnabled(false);  // 暂停刷新
    m_tableView->setModel(newModel);
    m_tableView->setUpdatesEnabled(true);  // 恢复刷新，一次性更新
}
```

---

## 错误处理规范

### 1. 统一错误处理

```cpp
bool MyWorker::exec()
{
    DA_LOG_INFO("Node {} starting execution", getID());
    
    try {
        // 输入验证
        if (!validateInput()) {
            DA_LOG_ERROR("Node {}: Invalid input", getID());
            setStatus(DA::DAAbstractNode::StatusError);
            return false;
        }
        
        // 执行处理
        processData();
        
        // 输出验证
        if (!validateOutput()) {
            DA_LOG_ERROR("Node {}: Invalid output", getID());
            setStatus(DA::DAAbstractNode::StatusError);
            return false;
        }
        
        DA_LOG_INFO("Node {} completed successfully", getID());
        setStatus(DA::DAAbstractNode::StatusFinished);
        return true;
        
    } catch (const std::exception& e) {
        DA_LOG_ERROR("Node {}: Exception - {}", getID(), e.what());
        setStatus(DA::DAAbstractNode::StatusError);
        return false;
    }
}
```

### 2. 异常处理策略

```cpp
// 分级异常处理
void MyPlugin::handleOperation()
{
    try {
        performOperation();
        
    } catch (const std::runtime_error& e) {
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

---

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

---

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

---

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

---

## 下一步

- [:material-book-open: 编码规范](./dev-guide/coding-standard.md) - 详细编码规范
- [:material-api: API 文档](./api-reference.md) - API 参考手册
- [:material-github: 贡献指南](./contribution-guide.md) - 参与开源贡献