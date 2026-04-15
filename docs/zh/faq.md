# 常见问题解答 (FAQ)

本页面汇总 DAWorkBench 开发和使用中的常见问题及解决方案，帮助用户快速定位和解决问题。

## 主要功能特性

**特性**

- ✅ **构建相关问题**：Qt 路径、第三方库、Python 环境等常见构建问题
- ✅ **插件开发问题**：插件加载、节点显示、工作流执行等开发问题
- ✅ **运行时问题**：节点显示、插件调试等运行相关问题
- ✅ **数据处理问题**：大型文件、DataFrame 传递等数据相关问题
- ✅ **界面扩展问题**：Ribbon 按钮、Dock 窗口等界面扩展问题
- ✅ **配置管理问题**：插件配置、配置文件位置等配置问题
- ✅ **跨平台问题**：Windows/Linux 差异、Qt5/Qt6 兼容性

## 构建相关问题

### Q: 构建时找不到 Qt 怎么解决？

**A:** 确保正确设置 Qt 路径：

```bash
# 方式1：设置环境变量
export Qt5_DIR=/path/to/Qt/5.15.2/msvc2019_64

# 方式2：CMake 参数
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/msvc2019_64 ..
```

### Q: 第三方库构建失败怎么解决？

**A:** 常见原因和解决方案：

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| submodule 未拉取 | 直接下载 zip | 使用 git clone + submodule update |
| zlib 缺失 | quazip 依赖 zlib | 先构建并安装 zlib |
| 编译器不匹配 | Qt 和编译器版本不一致 | 使用匹配的 Qt 和编译器 |

### Q: Python 环境相关问题？

**A:**

```bash
# 禁用 Python 支持
cmake -DDA_ENABLE_PYTHON=OFF ..

# 自动配置 Python 环境
cmake -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON ..
```

## 插件开发问题

### Q: 插件加载失败怎么办？

**A:** 检查以下几点：

1. **接口导出**：确保正确的 Q_PLUGIN_METADATA 声明
```cpp
Q_PLUGIN_METADATA(IID "Plugin.YourPlugin")
Q_INTERFACES(DA::DAAbstractNodePlugin)
```

2. **initialize 返回值**：必须返回 true
```cpp
bool MyPlugin::initialize() {
    // 所有初始化成功
    return true;  // 不能是 false
}
```

3. **依赖库位置**：确保所有依赖 dll/so 在 bin 目录

### Q: 节点不显示在列表中？

**A:** 检查节点元数据注册：

```cpp
QList<DA::DANodeMetaData> MyNodeFactory::getNodeMetaDataList() const
{
    // 确保返回有效数据
    if (m_nodePrototypes.isEmpty()) {
        qWarning() << "No node prototypes registered!";
    }
    return m_nodePrototypes.values();
}
```

### Q: 工作流执行时崩溃？

**A:** 最常见的原因是在 exec() 中操作 UI：

```cpp
bool MyWorker::exec()
{
    // ❌ 错误：直接操作 UI
    QMessageBox::show(...);  // 会崩溃！
    
    // ✅ 正确：使用信号通知主线程
    emit notifyUI("Processing complete");
    
    return true;
}
```

## 运行时问题

### Q: 程序启动后没有显示任何节点？

**A:** 可能原因：

1. 插件未正确加载 - 检查插件目录
2. 节点工厂未注册 - 检查 initialize() 调用
3. 节点元数据未定义 - 检查 getNodeMetaDataList()

### Q: 如何调试插件？

**A:** 使用日志系统：

```cpp
// 在关键位置添加日志
DA_LOG_DEBUG("Plugin initializing...");
DA_LOG_INFO("Node {} created", node->getID());
DA_LOG_ERROR("Failed to process: {}", errorMessage);
```

## 数据处理问题

### Q: 如何处理大型数据文件？

**A:** 建议：

1. 使用分块处理
2. 避免深拷贝
3. 利用 pandas 的高效操作

```cpp
bool MyWorker::exec()
{
    // 分块处理
    int chunkSize = 10000;
    for (int i = 0; i < totalRows; i += chunkSize) {
        processChunk(df, i, min(i + chunkSize, totalRows));
    }
}
```

### Q: DataFrame 如何在工作流中传递？

**A:** 使用 DADataPackage：

```cpp
// 输出 DataFrame
DA::DADataPackage pkg;
pkg.setDataFrame(df);

QVariant output;
output.setValue(pkg);
setOutputData("output", output);

// 输入 DataFrame
QVariant input = getInputData("input");
DA::DADataPackage pkg = input.value<DA::DADataPackage>();
auto df = pkg.getDataFrame();
```

## 界面扩展问题

### Q: 如何添加自定义 Ribbon 按钮？

**A:**

```cpp
bool MyPlugin::initialize()
{
    DA::DARibbonAreaInterface* ribbon = ui->getRibbonArea();
    
    // 创建 Category
    SARibbonCategory* cat = ribbon->addCategory(tr("My Tools"));
    
    // 创建 Panel
    SARibbonPanel* panel = cat->addPanel(tr("Actions"));
    
    // 创建 Action
    QAction* action = new QAction(QIcon(":/icon.png"), tr("My Action"));
    connect(action, &QAction::triggered, this, &MyPlugin::onAction);
    
    // 添加到 Panel
    panel->addLargeAction(action);
}
```

### Q: 如何创建自定义 Dock 窗口？

**A:**

```cpp
bool MyPlugin::initialize()
{
    DA::DADockingAreaInterface* dock = ui->getDockingArea();
    
    // 创建控件
    m_myWidget = new MyCustomWidget();
    
    // 注册到 Dock 系统
    dock->addDockWidget(
        m_myWidget,
        tr("My Window"),
        Qt::RightDockWidgetArea,
        "myplugin.dock.widget"
    );
}
```

## 配置管理问题

### Q: 如何保存插件配置？

**A:** 使用 QSettings 或 JSON：

```cpp
// QSettings 方式
QSettings settings;
settings.beginGroup("MyPlugin");
settings.setValue("option1", value1);
settings.endGroup();
settings.sync();

// JSON 方式
QJsonObject config;
config["option1"] = value1;
QJsonDocument doc(config);
file.write(doc.toJson());
```

### Q: 配置文件在哪里？

**A:**

| 类型 | 位置 |
|------|------|
| 全局配置 | 用户 AppData 目录 |
| 项目配置 | 项目目录 |
| 插件配置 | 项目/plugins/插件名/ |

## 跨平台问题

### Q: Windows 和 Linux 有什么差异？

**A:** 主要差异：

| 方面 | Windows | Linux |
|------|---------|-------|
| 动态库 | .dll | .so |
| Office 支持 | 支持（COM） | 不支持 |
| 路径分隔符 | `\` | `/` |
| 配置目录 | AppData | .config |

### Q: Qt5 和 Qt6 兼容性？

**A:** 项目同时支持 Qt5 和 Qt6：

```cmake
# 自动选择 Qt 版本
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# Qt6 需要额外模块
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS OpenGLWidgets Core5Compat REQUIRED)
endif()
```

## 性能问题

### Q: 工作流执行慢怎么优化？

**A:** 建议：

1. 减少节点间数据拷贝
2. 使用缓存避免重复计算
3. 合理设置线程数
4. 避免频繁的 UI 更新

### Q: 内存占用过高？

**A:** 检查：

1. 是否有大量缓存未清理
2. 是否有循环引用
3. 大数据是否正确释放

## 其他问题

### Q: 如何获取帮助？

**A:**

- 查看文档：https://czyt1988.github.io/data-workbench
- GitHub Issues：提交 bug 报告
- GitHub Discussions：功能讨论

### Q: 如何参与开发？

**A:**

1. Fork 仓库
2. 创建功能分支
3. 提交 Pull Request
4. 等待审核合并

详见 [贡献指南](./contribution-guide.md)。

## 下一步

- [:material-book: 最佳实践](./best-practices.md) - 开发最佳实践
- [:material-lightning-bolt: 性能优化](./performance-tips.md) - 性能优化建议
- [:material-puzzle: 插件开发](./dev-guide/plugin-project-create.md) - 插件开发指南