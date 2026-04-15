# 性能优化建议

本文档提供 DAWorkBench 插件和节点开发的性能优化建议，帮助开发者编写高效的数据处理代码。

## 主要功能特性

**特性**

- ✅ **数据处理优化**：避免深拷贝、分块处理、使用 pandas 高效操作、缓存计算结果
- ✅ **内存管理优化**：智能指针使用、及时释放大对象、限制缓存大小
- ✅ **UI 性能优化**：批量更新、异步加载、延迟渲染
- ✅ **线程优化**：避免 exec() 阻塞、合理使用线程池、控制并发数
- ✅ **工作流执行优化**：合理拓扑顺序、减少节点间数据传递、使用条件节点
- ✅ **配置优化**：缓存过期设置、日志级别优化
- ✅ **性能监控**：添加计时统计、内存使用监控

## 数据处理优化

### 1. 避免深拷贝

!!! tip "核心原则"
    大数据操作应尽量使用引用或浅拷贝，避免不必要的深拷贝，减少内存占用和拷贝时间。

下面的代码展示了正确的数据处理方式：

```cpp
// ❌ 错误：深拷贝整个 DataFrame - 内存翻倍，耗时增加
DA::DADataPackage copy = inputData;  // 复制所有数据

// ✅ 正确：使用引用 - 无额外内存开销
const DA::DADataPackage& ref = inputData;

// ✅ 正确：Qt 隐式共享 - 自动优化，避免深拷贝
QVariant output;
output.setValue(ref);  // Qt 自动使用隐式共享机制
```

### 2. 分块处理大数据

对于超过 10 万行的数据，建议分块处理，避免内存占用过高和 UI 卡顿。

下面的代码展示了分块处理的实现方式：

```cpp
bool MyWorker::exec()
{
    auto df = getInputDataFrame();         // 获取输入 DataFrame
    int rowCount = df.row_count();         // 获取总行数
    int chunkSize = 50000;                 // 分块大小：每块 5 万行
    
    for (int i = 0; i < rowCount; i += chunkSize) {
        int end = std::min(i + chunkSize, rowCount);  // 计算当前块结束位置
        processChunk(df, i, end);                       // 处理当前数据块
        
        // 定期更新进度（避免频繁更新）- 每 5 个块更新一次
        if (i % (chunkSize * 5) == 0) {
            emit progressUpdated(i * 100 / rowCount);  // 发送进度信号
        }
    }
    
    return true;
}
```

分块处理可以有效控制内存峰值使用，同时让用户感知处理进度。

### 3. 使用 pandas 高效操作

利用 pandas 内置的高效操作，避免 Python 循环处理。

下面的 Python 示例展示了高效和低效的处理方式：

```python
# ✅ 使用 pandas 内置函数 - 向量化操作，性能高
df.dropna()                      # 删除空值，比 Python 循环快得多
df.groupby('column').mean()      # 高效聚合计算

# ❌ 避免 Python 循环处理 - 效率低，应使用 pandas 方法
for i in range(len(df)):
    if pd.isna(df.iloc[i]):
        # 逐行处理效率低下
```

### 4. 缓存计算结果

避免重复计算相同结果，使用缓存提高效率。

下面的代码展示了带过期机制的缓存管理：

```cpp
class MyDataCache
{
public:
    // 缓存命中检查 - 检查是否存在且未过期
    bool hasCache(const QString& key) const {
        return m_cache.contains(key) && !isExpired(key);
    }
    
    // 获取缓存值
    QVariant getCache(const QString& key) const {
        return m_cache.value(key);
    }
    
    // 设置缓存值
    void setCache(const QString& key, const QVariant& value) {
        m_cache[key] = value;                      // 存储缓存数据
        m_cacheTime[key] = QDateTime::currentDateTime();  // 记录缓存时间
    }
    
private:
    // 过期检查 - 根据缓存时间判断是否需要重新计算
    bool isExpired(const QString& key) const {
        return m_cacheTime[key].addSecs(3600) < QDateTime::currentDateTime();  // 1 小时过期
    }
    
    QMap<QString, QVariant> m_cache;      // 缓存数据存储
    QMap<QString, QDateTime> m_cacheTime; // 缓存时间记录
};
```

## 内存管理优化

### 1. 使用智能指针

智能指针自动管理内存，避免手动 delete 导致的泄漏和错误。

下面的代码展示了智能指针的正确使用：

```cpp
// ✅ 推荐：智能指针自动管理 - 离开作用域自动释放
QSharedPointer<MyDataProcessor> m_processor;
m_processor = QSharedPointer<MyDataProcessor>::create();  // 创建并自动管理

// ❌ 避免：裸指针需要手动管理 - 容易忘记 delete
MyDataProcessor* m_processor;  // 需要 delete，易出错
```

### 2. 及时释放大对象

处理完成后立即释放临时数据，只保留必要结果。

下面的代码展示了正确的内存释放方式：

```cpp
bool MyWorker::exec()
{
    // 处理完成后立即释放临时数据
    {
        DA::DADataPackage tempData = loadLargeData();  // 加载大数据
        processData(tempData);                          // 处理数据
    }  // tempData 自动释放，离开作用域
    
    // 只保留必要的结果
    setOutputData("output", m_result);
    
    return true;
}
```

### 3. 限制缓存大小

设置缓存上限，避免占用过多内存。

下面的代码展示了缓存大小检查：

```cpp
void MyPlugin::checkCacheLimit()
{
    qint64 cacheSize = calculateCacheSize();           // 计算当前缓存大小
    qint64 maxCacheSize = 100 * 1024 * 1024;           // 最大 100 MB
    
    if (cacheSize > maxCacheSize) {
        clearOldestCacheEntries();                     // 清理最旧的缓存条目
    }
}
```

## UI 性能优化

### 1. 批量更新 UI

减少 UI 刷新频率，使用批量更新模式避免频繁重绘。

下面的代码展示了正确的批量更新方式：

```cpp
// ❌ 错误：频繁更新 - 每行刷新一次，界面卡顿
for (int i = 0; i < rowCount; ++i) {
    m_tableView->updateRow(i);  // 每次触发重绘，太频繁
}

// ✅ 正确：批量更新 - 先暂停再一次性更新
m_tableView->setUpdatesEnabled(false);   // 暂停刷新信号
m_tableView->setModel(newModel);         // 设置新模型
m_tableView->setUpdatesEnabled(true);    // 恢复刷新，触发一次性重绘
```

### 2. 异步加载大数据

使用线程加载大数据，避免阻塞主线程导致界面卡顿。

下面的代码展示了异步加载的实现方式：

```cpp
// 使用线程加载大数据 - QtConcurrent 简化异步操作
QFuture<DA::DADataPackage> future = QtConcurrent::run([this]() {
    return loadLargeDataFile(m_filePath);  // 在后台线程加载
});

// 使用 watcher 监听完成 - 在主线程处理结果
QFutureWatcher<DA::DADataPackage>* watcher = new QFutureWatcher(this);
connect(watcher, &QFutureWatcher::finished, this, [this, watcher]() {
    m_dataView->setData(watcher->result());  // 主线程更新 UI
    watcher->deleteLater();                   // 清理 watcher
});
watcher->setFuture(future);
```

### 3. 延迟渲染

对于复杂图形，使用延迟渲染只在必要时重新计算。

下面的代码展示了延迟渲染的实现：

```cpp
void MyChartWidget::paintEvent(QPaintEvent* event)
{
    if (m_needsFullRedraw) {
        // 只在必要时重新计算 - 避免每次绘制都计算
        recalculateChartData();
        m_needsFullRedraw = false;
    }
    
    // 快速绘制缓存的结果 - 直接使用预计算数据
    drawCachedChart();
}
```

## 线程优化

### 1. 避免 exec() 阻塞

不要在 exec() 中进行长时间阻塞操作，应使用进度回调让用户感知进度。

下面的代码展示了正确的非阻塞处理：

```cpp
// ❌ 错误：长时间阻塞 - 工作流线程被阻塞
bool MyWorker::exec()
{
    while (!processComplete()) {
        // 阻塞整个工作流执行
    }
}

// ✅ 正确：使用进度回调 - 分步处理并报告进度
bool MyWorker::exec()
{
    int steps = calculateTotalSteps();           // 计算总步骤数
    for (int i = 0; i < steps; ++i) {
        processStep(i);                           // 处理单步
        emit progressUpdated(i * 100 / steps);   // 报告进度
    }
}
```

### 2. 合理使用线程池

并行处理多个数据文件，充分利用多核 CPU。

下面的代码展示了并行处理的方式：

```cpp
// 并行处理多个数据文件
QList<QFuture<QVariant>> futures;
for (const QString& file : files) {
    futures.append(QtConcurrent::run([file]() {
        return processFile(file);               // 在线程池中处理
    }));
}

// 等待所有完成 - 使用 waitForFinished 阻塞等待
for (auto& future : futures) {
    future.waitForFinished();                   // 等待单个任务完成
    results.append(future.result());            // 收集结果
}
```

### 3. 控制并发数

限制线程池最大线程数，避免过多线程竞争资源。

下面的代码展示了并发数控制：

```cpp
// 设置最大线程数 - 避免线程过多导致资源竞争
int maxThreads = QThreadPool::globalInstance()->maxThreadCount();
QThreadPool::globalInstance()->setMaxThreadCount(std::min(maxThreads, 4));  // 限制为 4
```

## 工作流执行优化

### 1. 合理设置拓扑顺序

确保工作流拓扑正确，避免不必要的等待。

下面的代码展示了拓扑检查：

```cpp
// 检查节点依赖 - 在节点加入工作流时验证拓扑
bool MyNodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode* node)
{
    // 验证节点拓扑约束
    if (hasCircularDependency(node)) {
        DA_LOG_WARNING("Circular dependency detected");  // 检测循环依赖
    }
}
```

### 2. 减少节点间数据传递

合并相邻节点减少数据传递次数。

下面的 mermaid 流程图展示了节点合并的优化思路：

图表展示了将多个相邻节点合并为单一节点的优化方式，减少数据传递开销：

```mermaid
graph LR
    A[读取] --> B[清洗]
    B --> C[转换]
    
    D[读取+清洗+转换]  # 合并后的单一节点
```

合并节点可以减少数据在工作流中的传递次数，降低拷贝开销。

### 3. 使用条件节点

避免不必要的节点执行，通过条件判断跳过处理。

下面的代码展示了条件节点的实现：

```cpp
bool ConditionalNode::exec()
{
    bool shouldProcess = evaluateCondition();      // 评估条件
    if (!shouldProcess) {
        // 跳过处理，直接传递输出 - 避免不必要的计算
        setOutputData("output", getInputData("input"));
        return true;
    }
    
    return process();                               // 条件满足时执行处理
}
```

## 配置优化

### 1. 合理设置缓存过期

根据数据特性设置合理的缓存过期时间。

下面的代码展示了缓存策略的定义：

```cpp
// 根据数据特性设置缓存时间
struct CachePolicy {
    int maxSizeMB = 100;          // 最大缓存大小：100 MB
    int expireSeconds = 3600;     // 缓存过期时间：1 小时
    int maxEntries = 1000;        // 最大缓存条目数
};
```

### 2. 优化日志级别

生产环境使用较低日志级别减少开销，调试时使用详细日志。

下面的代码展示了日志级别设置：

```cpp
// 生产环境使用较低日志级别 - 减少 IO 开销
DA_LOG_SET_LEVEL(spdlog::level::info);

// 调试时使用详细日志 - 方便排查问题
DA_LOG_SET_LEVEL(spdlog::level::debug);
```

## 性能监控

### 1. 添加性能计时

在关键位置添加计时统计，监控执行性能。

下面的代码展示了性能计时的实现：

```cpp
bool MyWorker::exec()
{
    DA_LOG_INFO("Node {} starting", getID());       // 记录开始
    
    QElapsedTimer timer;
    timer.start();                                  // 开始计时
    
    processData();                                   // 执行处理
    
    qint64 elapsed = timer.elapsed();               // 获取耗时（毫秒）
    DA_LOG_INFO("Node {} completed in {} ms", getID(), elapsed);
    
    // 超时警告 - 发现性能问题
    if (elapsed > 5000) {
        DA_LOG_WARNING("Node {} took too long", getID());
    }
    
    return true;
}
```

### 2. 内存使用监控

监控内存使用，及时发现内存泄漏或过度占用。

下面的代码展示了内存监控：

```cpp
void MyPlugin::checkMemoryUsage()
{
    qint64 usedMemory = calculateUsedMemory();                  // 计算已使用内存
    DA_LOG_DEBUG("Memory usage: {} MB", usedMemory / 1024 / 1024);
    
    if (usedMemory > 500 * 1024 * 1024) {  // 500 MB 阈值
        DA_LOG_WARNING("High memory usage detected");
        clearCaches();                   // 清理缓存释放内存
    }
}
```

## 最佳实践总结

| 领域 | 建议 |
|------|------|
| **数据处理** | 分块处理、避免深拷贝、使用缓存 |
| **内存管理** | 智能指针、及时释放、限制缓存 |
| **UI 性能** | 批量更新、异步加载、延迟渲染 |
| **线程使用** | 控制并发、避免阻塞、合理等待 |
| **工作流** | 优化拓扑、合并节点、条件执行 |

## 下一步

- [:material-book: 最佳实践](./best-practices.md) - 综合最佳实践
- [:material-help-circle: FAQ](./faq.md) - 常见问题解答
- [:material-api: API 文档](./api-reference.md) - API 参考