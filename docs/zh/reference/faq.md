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
# Python 为强制依赖，无法禁用；以下为自动配置 Python 环境示例
cmake -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON ..
```

## 插件开发问题

### Q: 插件加载失败怎么办？

**A:** 检查以下几点：

1. **接口导出**：确保正确的 Q_PLUGIN_METADATA 声明

```cpp
Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
Q_INTERFACES(DA::DAAbstractNodePlugin)
```

1. **initialize 返回值**：必须返回 true

```cpp
bool MyPlugin::initialize() {
    // 所有初始化成功
    return true;  // 不能是 false
}
```

1. **依赖库位置**：确保所有依赖 dll/so 在 bin 目录

### Q: 节点不显示在列表中？

**A:** 当前工作流节点采用 Python-first `@NodeDef` 模型，节点通过 `DAPyNodeFactory::discoverNodes()` 扫描 Python 包自动发现。检查以下几点：

1. **Python 包结构**：节点包的 `__init__.py` 顶部必须先调 `setup_i18n()` 再导入节点模块，且 `setup.py` 通过 `entry_points` 声明 `data_workbench.plugin`
2. **`@NodeDef` 装饰器**：每个节点类必须用 `@NodeDef(name=..., category=...)` 装饰
3. **`__init__` 调用 `super().__init__()`**：否则 `_output_data` 等实例属性不存在，节点注册可能失败
4. **构建同步**：修改 Python 文件后必须同步到 build 目录（`bin/<Config>/pyplugins/<包名>/`），并重启程序（不支持热重载）
5. **看日志面板**：Python 异常会被 C++ 代理层吞掉，只有日志面板能看到完整错误

旧 C++ 节点工厂的 `getNodeMetaDataList()` 等接口已废弃，新插件不要使用：

```cpp
// 旧 C++ 节点工厂（已废弃，当前 DAPyNodeFactory 不再有此虚函数）
QList<DA::DANodeMetaData> MyNodeFactory::getNodeMetaDataList() const  // 无法编译
{
    if (m_nodePrototypes.isEmpty()) {
        qWarning() << "No node prototypes registered!";
    }
    return m_nodePrototypes.values();
}
```

参考 `plugins/DASystemNodes/AGENTS.md` 了解 Python-first 节点开发规范。

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
2. 节点工厂未注册 - 检查 `createNodeFactory()` 调用（C++ 入口）或 Python 包 `entry_points` 声明
3. 节点未自动发现 - 检查 `@NodeDef` 装饰器和 `__init__.py` 的 `setup_i18n()` 调用顺序

### Q: 如何调试插件？

**A:** 使用日志系统（详见 [日志系统文档](../dev-guide/general/logging.md)）：

```cpp
#include "DALogCategory.h"

// 在关键位置添加日志
daDebug << "Plugin initializing...";
daInfo << "Node created:" << node->getID();
daCritical << "Failed to process:" << errorMessage;
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

**A:** 数据包装类是 `DAData`（`src/DAData/DAData.h`），**不是** `DADataPackage`。当前推荐用 Python-first `@NodeDef` 节点，通过 `execute(self, inputs, params)` 的 `inputs` 接收、`self._output_data` 输出：

```python
# Python-first 节点（推荐）
def execute(self, inputs=None, params=None):
    if inputs is None:
        inputs = {}
    df = inputs.get("input")  # 读取上游 DataFrame
    if df is None:
        return False
    # ... 处理 ...
    self._output_data["output"] = result  # 通过 _output_data 写输出
    return True
```

旧 C++ 节点用 `DAData` 而非 `DADataPackage`：

```cpp
// 输出 DataFrame —— 用 DAData，不是 DADataPackage
DA::DAData outData(df);  // cn:从 DAPyDataFrame 构造 DAData
QVariant output;
output.setValue(outData);
setOutputData("output", output);

// 输入 DataFrame
QVariant input = getInputData("input");
DA::DAData data = input.value<DA::DAData>();  // cn:取出 DAData
if (data.isDataFrame()) {
    DA::DAPyDataFrame df = data.toDataFrame();
}
```

## 界面扩展问题

### Q: 如何添加自定义 Ribbon 按钮？

**A:** `DARibbonAreaInterface` 自身没有 `addCategory()`；添加 Category 要通过 `ribbonBar()->addCategory(...)`，Action 用 `DAActionsInterface::createAction(objname)` 创建：

```cpp
bool MyPlugin::initialize()
{
    DA::DAUIInterface* ui = core()->getUiInterface();
    DA::DARibbonAreaInterface* ribbon = ui->getRibbonArea();
    DA::DAActionsInterface* actions = ui->getActionInterface();

    // 通过 ribbonBar()->addCategory 创建 Category（不是 ribbon->addCategory）
    SARibbonCategory* cat = ribbon->ribbonBar()->addCategory(tr("My Tools"));

    // 创建 Panel
    SARibbonPanel* panel = cat->addPanel(tr("Actions"));

    // 用 createAction(objname) 创建托管 Action（参数是 object name，不是显示文本）
    QAction* action = actions->createAction("myplugin.action.my");
    action->setIcon(QIcon(":/icon.png"));
    action->setText(tr("My Action"));  // cn:显示文本单独设
    connect(action, &QAction::triggered, this, &MyPlugin::onAction);

    // 添加到 Panel
    panel->addLargeAction(action);
    return true;
}
```

### Q: 如何创建自定义 Dock 窗口？

**A:** 用 `DADockingAreaInterface::createDockWidget(QWidget*, ads::DockWidgetArea, const QString& widgetName, ...)`，不是 `addDockWidget`；停靠区域用 `ads::DockWidgetArea`，不是 `Qt::RightDockWidgetArea`：

```cpp
#include "ads_globals.h"

bool MyPlugin::initialize()
{
    DA::DAUIInterface* ui = core()->getUiInterface();
    DA::DADockingAreaInterface* dock = ui->getDockingArea();

    // 创建控件
    m_myWidget = new MyCustomWidget();

    // 注册到 Dock 系统 —— createDockWidget(QWidget*, ads::DockWidgetArea, QString, ...)
    dock->createDockWidget(
        m_myWidget,
        ads::DockWidgetArea::RightDockWidgetArea,  // cn:非 Qt::RightDockWidgetArea
        tr("My Window"));
    return true;
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

- 查看文档：<https://czyt1988.github.io/data-workbench>
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
- [:material-puzzle: 插件开发](../plugin/plugin-development.md) - 插件开发指南
