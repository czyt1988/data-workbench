# DAAgentTools 插件开发指南

DAWorkbench 平台内置 Agent 工具插件，向 LLM 暴露 **18 个工具**（5 数据 + 10 绘图 + 3 文件/报告），让 AI 能直接操作工作区数据、创建/修改图表、读写文件。工具的 OpenAI function schema 经 `DAAgentInterface::registerTool` 下发给 Python 子进程，**真实执行在 C++ 主进程**（不在 Python 端），结果经 stdin 回传。

> ⚠️ 本文件是 AI 开发 Agent 工具（新增/修改工具、改工具参数）的必读指南。改动前先对照 § 陷阱清单。Agent 框架本身（子进程、协议、会话持久化）的设计见 `src/DAAgent/AGENTS.md`，本文件只聚焦「工具本身怎么写」。

---

## 一、目录结构

```
DAAgentTools/
├── CMakeLists.txt              # 插件构建（file GLOB 自动收集 .h/.cpp，新增工具通常无需改）
├── DAAgentToolsPlugin.h/.cpp   # 插件入口：initialize() 注册 18 个工具 + figure_reference 提示词
├── DAAgentChartToolBase.h/.cpp # 图表工具基类（7 个图表访问方法，本插件内部用，无导出宏）
└── tools/                      # 18 个工具实现（每个一对 .h/.cpp）
    ├── DAAgentToolListData.{h,cpp}        # list_data
    ├── DAAgentToolDataInfo.{h,cpp}        # get_data_info
    ├── DAAgentToolQueryData.{h,cpp}       # query_data
    ├── DAAgentToolColumnStats.{h,cpp}     # get_column_stats
    ├── DAAgentToolExportData.{h,cpp}      # export_data
    ├── DAAgentToolCreateChart.{h,cpp}     # create_chart
    ├── DAAgentToolAddCurve.{h,cpp}        # add_curve
    ├── DAAgentToolSetChartStyle.{h,cpp}    # set_chart_style
    ├── DAAgentToolSetAxis.{h,cpp}          # set_axis（坐标轴类型/范围/颜色）
    ├── DAAgentToolUpdateCurveStyle.{h,cpp} # update_curve_style（修改已有曲线样式）
    ├── DAAgentToolRemoveChartItem.{h,cpp}  # remove_chart_item（删除曲线/标注/区域）
    ├── DAAgentToolAddAnnotation.{h,cpp}   # add_annotation（文本/箭头/点/区域标注）
    ├── DAAgentToolCreateSubplots.{h,cpp}  # create_subplots
    ├── DAAgentToolSaveChartImage.{h,cpp}  # save_chart_image（用 Qt::Svg/PrintSupport）
    ├── DAAgentToolListFigures.{h,cpp}     # list_figures
    ├── DAAgentToolReadFile.{h,cpp}        # read_file
    ├── DAAgentToolWriteFile.{h,cpp}       # write_file
    └── DAAgentToolSaveReport.{h,cpp}      # save_report（Win 用 DAAxOfficeWrapper 写 docx）
```

### 构建与运行

- 构建命令：`.\scripts\build.ps1 -Target DAAgentTools`（Windows）。插件是 SHARED 库，产物输出到 `build/<...>/plugins/`。
- 依赖第三方库（首次构建或 submodule 更新时编译，日常无需）：见根 `build.md`。
- 插件依赖：PUBLIC `DAWorkbench::DAAgent` + `DAWorkbench::DAGui`；PRIVATE `DAWorkbench::DAPluginSupport`；Win only PRIVATE `DAWorkbench::DAAxOfficeWrapper`；三方 `QtAdvancedDocking` + `qwt` + `Python3`（经 `damacro_import_*` 宏导入，CMakeLists.txt 已配好）。
- 工具跨 DLL 继承 `DAAgentToolBase` 需要 `DAAgent_API` 导出宏——该宏已加在基类上（`src/DAAgent/DAAgentToolBase.h`），插件直接继承即可，无需额外处理。

---

## 二、继承架构（决定你的工具继承谁）

```
DAAbstractAgentTool          (纯虚接口，src/DAAgent/DAAbstractAgentTool.h)
  │  getToolSpec() / execute() / getOwnerModule()
  ▼
DAAgentToolBase              (src/DAAgent/DAAgentToolBase.h，DAAgent_API 导出，QObject)
  │  mCore / dataMgr() / findData(name) / allDatas()
  │  errorResponse(msg) / successResponse(data|message)
  ├──► 数据工具 (5) + 文件/报告工具 (3)      ← 继承 DAAgentToolBase
  ▼
DAAgentChartToolBase         (本插件 DAAgentChartToolBase.h，无导出宏，仅本 DLL 内可见)
  │  chartOperateWidget() / currentFigure() / currentChart()
  │  findFigureByName(name) / createFigure(name)
  │  findChart(chartId, figureName) / enableAutoScale(chart)
  └──► 绘图工具 (10)                         ← 继承 DAAgentChartToolBase
```

**选择基类的判据**：

| 工具需要访问… | 继承 | 头文件 |
|----------------|------|--------|
| 只需访问数据（DataFrame） | `DAAgentToolBase` | `#include "DAAgentToolBase.h"` |
| 需要访问/创建图表 figure | `DAAgentChartToolBase` | `#include "DAAgentChartToolBase.h"` |
| 只读文件 / 生成报告（不碰数据也不碰图） | `DAAgentToolBase` | `#include "DAAgentToolBase.h"` |

> `DAAgentChartToolBase` 本身继承 `DAAgentToolBase`，故图表工具也能用 `findData` / `errorResponse` / `successResponse`。两个基类构造函数都只接收 `DACoreInterface* core` + `QObject* parent`，子类用 `using Base::Base;` 继承即可，无需手写构造。

---

## 三、18 个现有工具速查

| 类别 | name（schema 名） | 类 | 必填参数 | 备注 |
|------|------------------|----|----------|------|
| 数据 | `list_data` | `DAAgentToolListData` | — | 列出工作区所有数据集 |
| 数据 | `get_data_info` | `DAAgentToolDataInfo` | `data_name` | 数据形状/列名/dtype |
| 数据 | `query_data` | `DAAgentToolQueryData` | `data_name` + 查询条件 | 过滤/选择/聚合 |
| 数据 | `get_column_stats` | `DAAgentToolColumnStats` | `data_name`, `column` | describe() + 缺失值计数 |
| 数据 | `export_data` | `DAAgentToolExportData` | `data_name`, `file_path` | 导出 csv/xlsx |
| 绘图 | `create_chart` | `DAAgentToolCreateChart` | `type`, `data_name`, `x`, `y` | 如 figure_name 已存在则复用该 figure（支持子图）；返回 figure_id/figure_name |
| 绘图 | `add_curve` | `DAAgentToolAddCurve` | `data_name`, `x_column`, `y_column` | 经 figure_name+chart_id 定位；支持 color/width/style |
| 绘图 | `set_chart_style` | `DAAgentToolSetChartStyle` | — | 标题/标签/网格(布尔)/图例(布尔) + 背景色/边框色/网格样式/图例位置/图例样式/figure 背景色 |
| 绘图 | `set_axis` | `DAAgentToolSetAxis` | `axis` | 坐标轴类型(normal/datetime)/日期格式/范围(min/max)/颜色/标签旋转 |
| 绘图 | `update_curve_style` | `DAAgentToolUpdateCurveStyle` | `curve_name` | 修改已有曲线的 color/width/style/symbol/symbol_size/fill_color |
| 绘图 | `remove_chart_item` | `DAAgentToolRemoveChartItem` | `item_name` | 删除曲线/标注/区域，按标题或索引定位 |
| 绘图 | `add_annotation` | `DAAgentToolAddAnnotation` | — | 文本/箭头/点/区域标注（type: text/arrow/point/region） |
| 绘图 | `create_subplots` | `DAAgentToolCreateSubplots` | `layout` | 子图网格，返回 figure_id |
| 绘图 | `save_chart_image` | `DAAgentToolSaveChartImage` | `file_path` | png/pdf/svg；链接 Qt::Svg/PrintSupport |
| 绘图 | `list_figures` | `DAAgentToolListFigures` | — | 列出所有 figure 及内部 chart |
| 文件 | `read_file` | `DAAgentToolReadFile` | `file_path` | 含路径安全检查（禁系统目录） |
| 文件 | `write_file` | `DAAgentToolWriteFile` | `file_path`, `content` | 写文本文件 |
| 报告 | `save_report` | `DAAgentToolSaveReport` | `content`, `file_path` | md/pdf/docx；docx 仅 Win，链接 DAAxOfficeWrapper |

---

## 四、新增一个工具（标准流程）

以「新增一个 `get_column_unique` 工具，返回某列唯一值」为例。

### 步骤 1：选基类、新建文件

数据工具 → 继承 `DAAgentToolBase`。在 `tools/` 下建 `DAAgentToolColumnUnique.h` / `.cpp`。文件名/类名统一 `DA` 前缀（项目铁律），类名语义化。

### 步骤 2：写头文件（照现有工具抄）

```cpp
#pragma once
#include "DAAgentToolBase.h"   // 图表工具换成 "DAAgentChartToolBase.h"

namespace DA
{
/**
 * @brief get_column_unique 工具：返回某列的唯一值列表
 *
 * 参数：data_name（必填）、column（必填）、limit（可选，默认 100）
 */
class DAAgentToolColumnUnique : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;  // 继承构造，不要手写
    // 获取工具规格
    QJsonObject getToolSpec() const override;
    // 执行工具
    QJsonObject execute(const QJsonObject& params) override;
};
}  // namespace DA
```

**头文件规范**（项目铁律）：头文件**只写单行中文注释**或类级 Doxygen 注释；**禁止**在头文件写成员函数的 Doxygen 块注释（`/// @copydoc` 等也属于此类，应移到 `.cpp` 的 Doxygen 块中）。`Q_OBJECT` 宏必填（moc 需要它生成 staticMetaObject，跨 DLL 继承要用）。构造用 `using Base::Base;` 继承，不要手写。

### 步骤 3：实现 `getToolSpec()`（OpenAI function schema）

```cpp
QJsonObject DAAgentToolColumnUnique::getToolSpec() const
{
    return QJsonObject{
        {"name", "get_column_unique"},
        {"description", "Get unique values of a column in a dataset. Returns value list and count."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"column", QJsonObject{{"type", "string"}, {"description", "Column name"}}},
                {"limit", QJsonObject{{"type", "integer"}, {"description", "Max values to return (default 100)"}}}
            }},
            {"required", QJsonArray{"data_name", "column"}}
        }}
    };
}
```

**schema 约定**：
- `name`：**小写 snake_case，不翻译**（参与 LLM 工具调用，翻译会破坏匹配）。
- `description`：英文，写清楚工具做什么、何时用。LLM 据此决定是否调用，写得越具体越好（可参考 `create_chart` 的描述，会提示 figure_name 等关联参数用法）。
- `parameters.type` 固定 `"object"`；每个 property 给 `type` + `description`；`required` 列必填项。
- 没有 property 时也要写空对象 `QJsonObject{}` 和空数组 `QJsonArray{}`（见 `list_figures`）。

### 步骤 4：实现 `execute(params)`

```cpp
#include "DAAgentToolColumnUnique.h"
#include "DAPyGILGuard.h"        // 涉及 Python/pandas 必加（见陷阱 P2）
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"

namespace DA
{
QJsonObject DAAgentToolColumnUnique::execute(const QJsonObject& params)
{
    QString dataName = params["data_name"].toString();
    QString column   = params["column"].toString();
    int limit        = params["limit"].toInt(100);
    if (dataName.isEmpty()) {
        return errorResponse("data_name is required");
    }
    if (column.isEmpty()) {
        return errorResponse("column is required");
    }

    DAData data = findData(dataName);          // 基类提供
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;                          // 持 GIL 后再碰 Python 对象
    DAPyDataFrame df = data.toDataFrame();

    QList<QString> cols = df.columns();
    if (!cols.contains(column)) {
        return errorResponse(QString("Column '%1' not found").arg(column));
    }

    DAPySeries col = df[ column ];             // 用 operator[]，不要用 df.loc()（见陷阱 P3）
    QList<QString> unique = col.uniqueValuesAsString(limit);

    QJsonObject result;
    result["column"] = column;
    result["count"]  = unique.size();
    result["values"] = QJsonArray::fromStringList(unique);
    return successResponse(result);
}
}  // namespace DA
```

**execute 约定**：
- 第一步永远校验必填参数为空 → `errorResponse`。
- 数据/列不存在 → `errorResponse`（消息含具体名字，便于 LLM 自纠）。
- 成功 → `successResponse(data|message)`（基类提供，自动补 `success:true`）。**不要手写 `resp["success"]=true`**，用基类方法保持一致。
- 错误消息**英文**（会进 ToolMessage 回传给 LLM，不翻译）。
- 函数内不需要 try/catch（`DAAgentBridge::executeTool()` 已兜底；但工具内部逻辑分支要返回 errorResponse，不要让异常冒泡）。

### 步骤 5：在插件入口注册

`DAAgentToolsPlugin.cpp` 的 `initialize()` 里，`#include` 头文件 + `agent->registerTool(new DAAgentToolColumnUnique(c, this));`：

```cpp
#include "tools/DAAgentToolColumnUnique.h"
// ...
bool DAAgentToolsPlugin::initialize()
{
    auto* c = core();
    auto* agent = c->getAgentInterface();
    // ... 现有 16 个注册 ...
    agent->registerTool(new DAAgentToolColumnUnique(c, this));   // 新增这一行
    // ...
    return DAAbstractPlugin::initialize();
}
```

`this` 作为 QObject parent，工具随插件释放，不要手动 delete。

### 步骤 6：构建验证

```powershell
.\scripts\build.ps1 -Target DAAgentTools
```

CMakeLists.txt 用 `file(GLOB ... *.h *.cpp)` 自动收集 `tools/` 下的新文件，**通常无需改 CMakeLists.txt**。例外见 § 六（新增 Qt 模块/DA 库依赖时）。构建后运行程序，在 Agent 对话里让 LLM 调用新工具，或看 `da_log.log` 确认 `registerTool` 后工具被 `DAAgentBridge::setTools` 收录。

---

## 五、工具实现常用模式

### 5.1 数据访问（pandas DataFrame）

| 操作 | 写法 | 说明 |
|------|------|------|
| 取数据集 | `DAData data = findData(dataName);` | 基类 `DAAgentToolBase::findData` |
| 判空 | `if (data.isNull())` | 找不到数据集时 |
| 取 DataFrame | `DAPyDataFrame df = data.toDataFrame();` | 须在 `DAPyGILGuard` 之后 |
| 列名列表 | `QList<QString> cols = df.columns();` | 列存在性校验 |
| 取列 Series | `DAPySeries col = df[ column ];` | **用 operator[]，不要用 df.loc("col")**——`.loc` 是按行标签访问 |
| Series → QVector\<double> | `QVector<double> xs = toQVectorDouble(col);` | 非数值列返回空，据此报错 |
| 列统计 | `col.describe()` | 见 `get_column_stats` |
| 缺失值 | `col.isNull().sum()` | 两步：先 isNull 再 sum |

涉及任何 Python 对象（`DAPyDataFrame`/`DAPySeries` 等）前**必须** `DAPyGILGuard gil;` 拿 GIL，否则崩溃（见陷阱 P2）。

### 5.2 图表访问（仅图表工具）

`DAAgentChartToolBase` 提供的 7 个方法（实现在 `DAAgentChartToolBase.cpp`）：

| 方法 | 用途 |
|------|------|
| `chartOperateWidget()` | 取图表操作窗口（DAGui），是访问 figure/chart 的入口；内部链路 `mCore->getUiInterface()->getDockingArea()->getChartOperateWidget()` |
| `currentFigure()` | 当前活动 figure，无则 nullptr |
| `currentChart()` | 当前活动 chart，无则 nullptr |
| `findFigureByName(name)` | 按 tab 标题匹配 figure；空名/未找到返回 nullptr |
| `createFigure(name)` | 创建新 figure 并设为当前；**`create_chart` 每次调它建新图，不复用现有** |
| `findChart(chartId, figureName)` | 按 figure_name 定位 figure，再按 chart_id（标题/整数索引/空或"current"）定位 chart |
| `enableAutoScale(chart)` | 恢复 xBottom/yLeft 自动缩放——加数据后必调，否则 `createChart` 锁定坐标轴 `[0,800]×[0,500]` 会导致数据落在可见范围外 |

**`figure_name` + `chart_id` 定位模式**（`add_curve`/`set_chart_style`/`add_annotation`/`save_chart_image` 共用）：

```cpp
QString chartId    = params["chart_id"].toString();
QString figureName = params["figure_name"].toString();
DAChartWidget* chart = findChart(chartId, figureName);   // 基类方法
if (!chart) {
    return errorResponse(QString("Chart '%1' not found").arg(figureName.isEmpty() ? chartId : figureName + "/" + chartId));
}
// ... 在 chart 上操作 ...
enableAutoScale(chart);   // 加完数据后恢复自动缩放
chart->replot();          // 触发重绘
```

`create_chart`/`create_subplots` 不走 `findChart`，而是 `createFigure(figureName)` 建新 figure + `fig->createChart()` 建新 chart，返回 `figure_id`/`figure_name` 供后续工具精确定位 + 供 agent 在回复里用 `da-figure:` 超链接引用。

**`DAChartWidget` 常用 API**（`src/DAFigure/DAChartWidget.h`）：

| 方法 | 说明 |
|------|------|
| `addCurve(QVector<QPointF>, title)` / `addCurve(xData, yData, title)` | 折线，返回 `QwtPlotCurve*` |
| `addScatter(QVector<QPointF>, title)` | 散点 |
| `addBarChart(QVector<QPointF>, title)` / `addBarChart(QVector<double>, title)` | 柱状 |
| `addHistogram(QVector<QwtIntervalSample>, title)` | 直方图 |
| `addBoxChart(QVector<QwtBoxSample>, title)` | 箱线图 |
| `setChartTitle` / `getChartTitle` | 图表标题 |
| `setAxisLabel(axisId, label)` | 坐标轴标签（`QwtPlot::xBottom`/`yLeft`） |
| `setAxisAutoScale(axis, on)` / `replot()` | 缩放与重绘 |

**`DAFigureWidget` 常用 API**（`src/DAFigure/DAFigureWidget.h`）：

| 方法 | 说明 |
|------|------|
| `createChart()` / `createChart(QRectF)` | 在 figure 内创建新 chart |
| `setCurrentChart(QwtPlot*)` | 设当前 chart |
| `getCurrentChart()` / `getCharts()` | 取当前/全部 chart |
| `getFigureId()` | figure 唯一 id，回传给 agent 用于 `da-figure:id=` 精确引用 |

### 5.3 文件操作（read_file / write_file）

- 路径安全：`read_file` 用 `isPathSafe()` 屏蔽 `c:/windows`、`c:/program files` 等系统目录（见 `DAAgentToolReadFile.cpp`）。新增读文件类工具照抄。
- 写文件：`QFile` + `QIODevice::WriteOnly | QIODevice::Text`。
- 报告导出：`save_report` 的 `pdf` 走 `QPrinter`+`QTextDocument`（需 `Qt::PrintSupport`）；`docx` 走 `DAAxObjectWordWrapper` Word COM（仅 Windows，`.cpp` 已有 `#ifdef Q_OS_WIN` 守卫，CMakeLists.txt 里 `if(WIN32)` 才 link `DAAxOfficeWrapper`）。

### 5.4 注册系统提示词

`DAAgentToolsPlugin::initialize()` 末尾用 `agent->registerSystemPrompt("figure_reference", ...)` 注入绘图引用提示词，教 agent 用 `da-figure:<figure_name>` / `da-figure:id=<figure_id>` 超链接引用创建的绘图（用户点击可 raise 定位 figure）。新工具若引入新的交互约定（如类似超链接机制），在此追加提示词片段。

---

## 六、CMake 依赖变更（仅特定场景需改）

CMakeLists.txt 用 `file(GLOB ... CONFIGURE_DEPENDS)` 收集 `*.h/*.cpp`，新增工具文件**无需**改 CMakeLists.txt。只有以下情况要改：

| 场景 | 改动位置 |
|------|---------|
| 工具新用 Qt 模块（如 `Qt::Network`） | `find_package` 加组件 + `target_link_libraries PRIVATE` 加 |
| 工具新用 DA 库（如 `DAWorkbench::DAData` 已通过 DAAgent PUBLIC 传递，一般不用改） | `target_link_libraries` 按需加，PUBLIC/PRIVATE 参考现有注释 |
| 工具新用三方库 | 调 `damacro_import_*` 宏（参考 qwt/Python/ADS 的现有导入） |
| 平台专属（如 Win only Office） | `if(WIN32)` 内 link + `.cpp` 加 `#ifdef Q_OS_WIN` 守卫 |

现有依赖（已配好，新工具若用到同样库则无需重复加）：
- `Qt::Svg` / `Qt::PrintSupport`：`save_chart_image`（svg/pdf）+ `save_report`（pdf）
- `DAWorkbench::DAAxOfficeWrapper`（Win only）：`save_report`（docx）
- `DAAgent`（PUBLIC）：基类 `DAAgentToolBase` + `DAAbstractAgentTool` + `DAAgentInterface`
- `DAGui`（PUBLIC）：`DAAgentChartToolBase.h` 经 `DAAgentToolBase.h` 暴露 `DAChartOperateWidget` 等头
- `qwt` + `QtAdvancedDocking` + `Python3`：经 include 链需要，已 `damacro_import_*`

---

## 七、陷阱清单（改工具前必读）

### P1. 跨 DLL 继承必须 `Q_OBJECT` + 基类导出
工具在插件 DLL，基类 `DAAgentToolBase` 在 `DAAgent.dll`。跨 DLL 继承 QObject 子类**必须**在子类写 `Q_OBJECT` 宏（moc 生成 staticMetaObject），否则信号槽/moc 元信息缺失。基类已加 `DAAgent_API` 导出宏，子类无需再加。

### P2. 碰 Python 对象前必须 `DAPyGILGuard gil;`
`DAPyDataFrame`/`DAPySeries` 等封装了 Python 对象，没拿 GIL 就访问会崩溃。`DAPyGILGuard` 在作用域内持锁，析构释放。**写法**：在 `findData`/参数校验**之后**、`data.toDataFrame()` **之前**声明 `DAPyGILGuard gil;`（校验是纯 Qt 操作，不需要 GIL）。见 `get_column_stats.cpp:40`、`add_curve.cpp:70`。

### P3. 取列用 `df[column]`，**不要**用 `df.loc("col")`
`DAPyDataFrame::loc()` 是按行标签访问（pandas `.loc[label]`），`.iloc`/`loc` 误用会拿到行而非列。取列统一 `DAPySeries col = df[ column ];`（operator[] 重载）。每个现有数据/绘图工具都遵循此约定，源码注释也标了。

### P4. 图表加数据后必须 `enableAutoScale` + `replot`
`DAFigureWidget::createChart()` 用 `setAxisScale(0,800)/(0,500)` 锁定坐标轴范围（禁用 Qwt auto-scale），不恢复会导致数据落在可见范围外显示空白。凡是往 chart 加数据的工具（`create_chart`/`add_curve` 等），数据加完后调 `enableAutoScale(chart); chart->replot();`。

### P5. 执行不要抛异常冒泡
`DAAgentBridge::executeTool()` 有 try/catch 兜底，工具抛异常时返回 `{success:false, error:...}`。但工具内部**逻辑分支**应主动 `return errorResponse(...)`，不要依赖兜底——异常被吞后表现为后续业务静默失败，极难排查。

### P6. schema `name` 不翻译
`name` 参与 LLM 工具调用匹配，翻译会破坏已存对话/提示词对工具的引用。`description`/`parameters.*.description` 写英文即可（LLM 读英文 schema，不需要中文）。

### P7. pybind11↔Qt 类型转换要 include caster
工具 `.cpp` 里若把 `QString`/`QVariant` 等传给 Python 可调用对象、或从 Python 对象 cast 到 Qt 类型，**必须**在该 `.cpp` 顶部 `#include "DAPybind11QtCaster.hpp"`（per-translation-unit 生效，仅 include `DAPybind11InQt.h` 不够）。遗漏不报错但运行时抛 `Unable to convert call argument of type 'QString'`，被吞后静默失败。详见根 AGENTS.md「pybind11 类型转换铁律」与 `docs/zh/dev-guide/dapybind11-qt-caster.md`。现有工具多走 `DAPyDataFrame`/`DAPySeries` 的 Qt 友好 API，较少直接 cast，但扩展新操作时要留意。

### P8. 非数值列转 QVector\<double> 返回空
`toQVectorDouble(series)` 对非数值（字符串/分类）列返回空 vector。据此判空 + 返回 errorResponse 提示「列含非数值，请用数值列或先 query_data 预聚合」（见 `create_chart.cpp:141-150`），不要让 agent 反复尝试。

### P9. `create_chart` 可复用已有 figure
`create_chart` 先用 `findFigureByName(name)` 查找已有 figure——若存在则在该 figure 中添加 chart（支持子图布局），不存在才 `createFigure(name)` 新建。`create_subplots` 仍始终新建 figure。修改其他图走 `findChart(chartId, figureName)` 定位。

### P10. 工具归属模块别放错
工具属本插件 `plugins/DAAgentTools/`（L5 应用层/插件层），**不是** `src/DAAgent/`（`src/DAAgent/` 是纯框架库，不含具体工具实现，`tools/` 子目录已删除）。基类 `DAAgentToolBase` 在 `src/DAAgent/`，但具体工具实现全在本插件。详见 `src/DAAgent/AGENTS.md` § 七。

---

## 八、修改现有工具的检查清单

改一个已存在的工具时逐项确认：

- [ ] `name` 字段没改（改了会破坏已存对话/提示词对工具的引用；要改名等于新增工具 + 删旧工具，需同步 `registerTool` 调用）
- [ ] schema 改动（增/删参数、改 required）后，`description` 同步更新说明新参数用途
- [ ] `execute` 内参数读取用 `params["key"].toString()` / `.toInt(default)`，新参数有默认值兜底
- [ ] 新分支都有 `return errorResponse` 或 `successResponse`，无悬空 return
- [ ] 涉及 Python 的新操作加了 `DAPyGILGuard`、用 `df[]` 取列、数值转换后判空
- [ ] 图表工具加数据后有 `enableAutoScale` + `replot`
- [ ] 新依赖在 CMakeLists.txt 配好（Qt 模块/DA 库/三方/平台专属）
- [ ] `DAAgentToolsPlugin::initialize()` 的 `registerTool` 调用与工具类一致（新增了类就要加一行）
- [ ] 构建通过：`.\scripts\build.ps1 -Target DAAgentTools`，运行后 `da_log.log` 确认工具被 `setTools` 收录

---

## 九、WHERE TO LOOK（速查）

| 任务 | 位置 |
|------|------|
| 新增工具 | `tools/` 新建 `.h/.cpp` + `DAAgentToolsPlugin.cpp::initialize()` 注册（§ 四） |
| 改工具 schema/参数 | 对应 `tools/DAAgentToolXxx.cpp::getToolSpec()` |
| 改工具执行逻辑 | 对应 `tools/DAAgentToolXxx.cpp::execute()` |
| 改插件注册顺序/提示词 | `DAAgentToolsPlugin.cpp::initialize()` |
| 数据访问 API | `src/DAData/DAData.h` + `src/DAPyBindQt/pandas/DAPyDataFrame.h`/`DAPySeries.h` |
| 图表访问基类 | `plugins/DAAgentTools/DAAgentChartToolBase.h/.cpp` |
| 图表 widget API | `src/DAFigure/DAChartWidget.h` + `DAFigureWidget.h` |
| 工具抽象接口 | `src/DAAgent/DAAbstractAgentTool.h`（`getToolSpec`/`execute`/`getOwnerModule`） |
| 工具基类 | `src/DAAgent/DAAgentToolBase.h`（瘦基类，数据/响应方法） |
| 工具执行/协议 | `src/DAAgent/DAAgentBridge.cpp::executeTool()`（try/catch 兜底 + 回传 tool_result） |
| 插件框架 | `src/DAPluginSupport/DAAbstractPlugin.h`（`initialize`/`core`/IID） |
| Agent 框架全貌 | `src/DAAgent/AGENTS.md`（子进程/协议/会话/信号链，本文件不重复） |
| 构建问题 | 根 `build.md`（Windows 用 `scripts/build.ps1`，禁 Ninja） |

---

## 十、开发 Checklist（新增工具完整流程）

- [ ] 选对基类（数据/文件 → `DAAgentToolBase`；图表 → `DAAgentChartToolBase`）
- [ ] 头文件只单行注释，不写成员函数 Doxygen 块；`Q_OBJECT` + `using Base::Base;`
- [ ] `getToolSpec` 的 `name` 小写 snake_case 不翻译，`required` 完整，`description` 写清用途
- [ ] `execute` 先校验必填参数 → 数据/列存在性 → 业务逻辑 → `successResponse`/`errorResponse`
- [ ] Python 操作有 `DAPyGILGuard`；取列用 `df[col]`；数值转换判空
- [ ] 图表工具加数据后 `enableAutoScale` + `replot`；`create_chart` 先查找已有 figure 再新建
- [ ] `DAAgentToolsPlugin::initialize()` 加 `registerTool(new ToolXxx(c, this))`
- [ ] CMake 新依赖已配（Qt 模块/DA 库/三方/平台专属）
- [ ] `.\scripts\build.ps1 -Target DAAgentTools` 构建通过
- [ ] 运行验证：Agent 对话调用新工具，或 `da_log.log` 确认收录（18 个工具）
