# 性能优化建议

本文档提供 DAWorkBench 插件和节点开发的性能优化建议。

---

## 数据处理优化

### 1. 避免深拷贝

!!! tip "核心原则"
    大数据操作应尽量使用引用或浅拷贝，避免不必要的深拷贝。

```cpp
// ❌ 错误：深拷贝整个 DataFrame
DA::DADataPackage copy = inputData;  // 复制所有数据

// ✅ 正确：使用引用
const DA::DADataPackage& ref = inputData;

// ✅ 正确：Qt 隐式共享
QVariant output;
output.setValue(ref);  // Qt 自动使用隐式共享
```

### 2. 分块处理大数据

对于超过 10 万行的数据，建议分块处理：

```cpp
bool MyWorker::exec()
{
    auto df = getInputDataFrame();
    int rowCount = df.row_count();
    int chunkSize = 50000;  // 分块大小
    
    for (int i = 0; i < rowCount; i += chunkSize) {
        int end = std::min(i + chunkSize, rowCount);
        processChunk(df, i, end);
        
        // 定期更新进度（避免频繁更新）
        if (i % (chunkSize * 5) == 0) {
            emit progressUpdated(i * 100 / rowCount);
        }
    }
    
    return true;
}
```

### 3. 使用 pandas 高效操作

利用 pandas 内置的高效操作：

```python
# ✅ 使用 pandas 内置函数
df.dropna()  # 比 Python 循环快得多
df.groupby('column').mean()  # 高效聚合

# ❌ 避免 Python 循环处理
for i in range(len(df)):
    if pd.isna(df.iloc[i]):
        # ...
```

### 4. 缓存计算结果

避免重复计算相同结果：

```cpp
class MyDataCache
{
public:
    bool hasCache(const QString& key) const {
        return m_cache.contains(key) && !isExpired(key);
    }
    
    QVariant getCache(const QString& key) const {
        return m_cache.value(key);
    }
    
    void setCache(const QString& key, const QVariant& value) {
        m_cache[key] = value;
        m_cacheTime[key] = QDateTime::currentDateTime();
    }
    
private:
    bool isExpired(const QString& key) const {
        return m_cacheTime[key].addSecs(3600) < QDateTime::currentDateTime();
    }
    
    QMap<QString, QVariant> m_cache;
    QMap<QString, QDateTime> m_cacheTime;
};
```

---

## 内存管理优化

### 1. 使用智能指针

```cpp
// ✅ 推荐：智能指针自动管理
QSharedPointer<MyDataProcessor> m_processor;
m_processor = QSharedPointer<MyDataProcessor>::create();

// ❌ 避免：裸指针需要手动管理
MyDataProcessor* m_processor;  // 需要 delete，易出错
```

### 2. 及时释放大对象

```cpp
bool MyWorker::exec()
{
    // 处理完成后立即释放临时数据
    {
        DA::DADataPackage tempData = loadLargeData();
        processData(tempData);
    }  // tempData 自动释放
    
    // 只保留必要的结果
    setOutputData("output", m_result);
    
    return true;
}
```

### 3. 限制缓存大小

```cpp
void MyPlugin::checkCacheLimit()
{
    qint64 cacheSize = calculateCacheSize();
    qint64 maxCacheSize = 100 * 1024 * 1024;  // 100 MB
    
    if (cacheSize > maxCacheSize) {
        clearOldestCacheEntries();
    }
}
```

---

## UI 性能优化

### 1. 批量更新 UI

```cpp
// ❌ 错误：频繁更新
for (int i = 0; i < rowCount; ++i) {
    m_tableView->updateRow(i);  // 每次触发重绘
}

// ✅ 正确：批量更新
m_tableView->setUpdatesEnabled(false);
m_tableView->setModel(newModel);  // 一次性设置
m_tableView->setUpdatesEnabled(true);  // 只重绘一次
```

### 2. 异步加载大数据

```cpp
// 使用线程加载大数据
QFuture<DA::DADataPackage> future = QtConcurrent::run([this]() {
    return loadLargeDataFile(m_filePath);
});

// 使用 watcher 监听完成
QFutureWatcher<DA::DADataPackage>* watcher = new QFutureWatcher(this);
connect(watcher, &QFutureWatcher::finished, this, [this, watcher]() {
    m_dataView->setData(watcher->result());
    watcher->deleteLater();
});
watcher->setFuture(future);
```

### 3. 延迟渲染

对于复杂图形，使用延迟渲染：

```cpp
void MyChartWidget::paintEvent(QPaintEvent* event)
{
    if (m_needsFullRedraw) {
        // 只在必要时重新计算
        recalculateChartData();
        m_needsFullRedraw = false;
    }
    
    // 快速绘制缓存的结果
    drawCachedChart();
}
```

---

## 线程优化

### 1. 避免在 exec() 中阻塞

```cpp
// ❌ 错误：长时间阻塞
bool MyWorker::exec()
{
    while (!processComplete()) {
        // 阻塞整个工作流执行
    }
}

// ✅ 正确：使用进度回调
bool MyWorker::exec()
{
    int steps = calculateTotalSteps();
    for (int i = 0; i < steps; ++i) {
        processStep(i);
        emit progressUpdated(i * 100 / steps);
    }
}
```

### 2. 合理使用线程池

```cpp
// 并行处理多个数据块
QList<QFuture<QVariant>> futures;
for (const QString& file : files) {
    futures.append(QtConcurrent::run([file]() {
        return processFile(file);
    }));
}

// 等待所有完成
for (auto& future : futures) {
    future.waitForFinished();
    results.append(future.result());
}
```

### 3. 控制并发数

```cpp
// 设置最大线程数
int maxThreads = QThreadPool::globalInstance()->maxThreadCount();
QThreadPool::globalInstance()->setMaxThreadCount(std::min(maxThreads, 4));
```

---

## 工作流执行优化

### 1. 合理设置拓扑顺序

确保工作流拓扑正确，避免不必要的等待：

```cpp
// 检查节点依赖
bool MyNodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode* node)
{
    // 验证节点拓扑约束
    if (hasCircularDependency(node)) {
        DA_LOG_WARNING("Circular dependency detected");
    }
}
```

### 2. 减少节点间数据传递

合并相邻节点减少数据传递：

```mermaid
graph LR
    A[读取] --> B[清洗]
    B --> C[转换]
    
    # 可合并为：
    A2[读取+清洗+转换]
```

### 3. 使用条件节点

避免不必要的节点执行：

```cpp
bool ConditionalNode::exec()
{
    bool shouldProcess = evaluateCondition();
    if (!shouldProcess) {
        // 跳过处理，直接传递输入
        setOutputData("output", getInputData("input"));
        return true;
    }
    
    return process();
}
```

---

## 配置优化

### 1. 合理设置缓存过期

```cpp
// 根据数据特性设置缓存时间
struct CachePolicy {
    int maxSizeMB = 100;
    int expireSeconds = 3600;  // 1小时
    int maxEntries = 1000;
};
```

### 2. 优化日志级别

```cpp
// 生产环境使用较低日志级别
DA_LOG_SET_LEVEL(spdlog::level::info);

// 调试时使用详细日志
DA_LOG_SET_LEVEL(spdlog::level::debug);
```

---

## 性能监控

### 1. 添加性能计时

```cpp
bool MyWorker::exec()
{
    DA_LOG_INFO("Node {} starting", getID());
    
    QElapsedTimer timer;
    timer.start();
    
    processData();
    
    qint64 elapsed = timer.elapsed();
    DA_LOG_INFO("Node {} completed in {} ms", getID(), elapsed);
    
    // 超时警告
    if (elapsed > 5000) {
        DA_LOG_WARNING("Node {} took too long", getID());
    }
    
    return true;
}
```

### 2. 内存使用监控

```cpp
void MyPlugin::checkMemoryUsage()
{
    qint64 usedMemory = calculateUsedMemory();
    DA_LOG_DEBUG("Memory usage: {} MB", usedMemory / 1024 / 1024);
    
    if (usedMemory > 500 * 1024 * 1024) {  // 500 MB
        DA_LOG_WARNING("High memory usage detected");
        clearCaches();
    }
}
```

---

## 最佳实践总结

| 领域 | 建议 |
|------|------|
| **数据处理** | 分块处理、避免深拷贝、使用缓存 |
| **内存管理** | 智能指针、及时释放、限制缓存 |
| **UI 性能** | 批量更新、异步加载、延迟渲染 |
| **线程使用** | 控制并发、避免阻塞、合理等待 |
| **工作流** | 优化拓扑、合并节点、条件执行 |

---

## 下一步

- [:material-book: 最佳实践](./best-practices.md) - 综合最佳实践
- [:material-help-circle: FAQ](./faq.md) - 常见问题解答
- [:material-api: API 文档](./api-reference.md) - API 参考