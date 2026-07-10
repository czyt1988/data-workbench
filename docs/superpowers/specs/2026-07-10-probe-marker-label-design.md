# 数据探针标记 Label 显示设计

**Created:** 2026-07-10
**Status:** Design (pending implementation)
**Related:** `src/DAFigure/DADataProbeMarker`, `GDataLinkTableTab`, `GDataLinkTableWidget`, `GDataLinkTableModel`

---

## 1. 背景与动机

数据连接窗口通过 `GDataLinkTableWidget` 监听 figure 的 picker group 点击事件。用户在绘图上点击某 x 位置后，`GDataLinkTableTab::onPickerClicked`（`GDataLinkTableTab.cpp:147-202`）会为 figure 内每个子绘图（`QwtPlot`）创建一个 `DA::DADataProbeMarker`（垂直探针），并在数据连接表格中新增一列记录各曲线在该 x 处的 y 值。

当前存在两个体验缺口：

1. **探针无标识**：`DADataProbeMarker` 已具备 `probeName`（QwtText）与 `drawProbeName()` 绘制能力，但所有创建点（`GDataLinkTableTab.cpp:192-197`、`DAChartItemCreatInteractor.cpp:148-166`）均传入空名字，导致探针在绘图上仅显示一条虚线，无法与表格中的列对应。
2. **标签位置未适配 x 轴**：部分子绘图不显示 x 轴（寄生轴模式），此时探针标签若一律画在固定位置会与布局冲突；标签应跟随 x 轴显示侧（xbottom→底部，xtop→顶部），x 轴完全不显示时则不画标签。

本次目标：为数据连接窗口创建的探针分配 A/B/C 字母标识并以圆角矩形徽章形式绘制在 x 轴显示侧，同时将表格列头改为字母以建立探针与列的直观对应。

---

## 2. 目标与非目标

### 2.1 目标

1. `DADataProbeMarker` 新增 `LabelStyle` 枚举，支持纯文字、圆角矩形、直角矩形、椭圆四种徽章样式（仅 API 可配置，无最终用户 UI 入口）
2. 数据连接窗口为每次点击分配字母标识（A、B、C…AA、AB…），同一点击产生的所有探针共享同一字母
3. 探针标签位置由所挂绘图的 x 轴可见性决定：xbottom 显示→底部，xtop 显示→顶部，两者都显示→默认底部，都不显示→不显示标签
4. 数据连接表格列头改为字母，原 x 值文本作为列头 tooltip
5. 删除列后剩余探针字母保持不变，计数器只增不减

### 2.2 非目标

- 不为最终用户提供徽章样式选择 UI（仅 API 层可配置）
- 不改动 `DAChartItemCreatInteractor` 交互式创建路径（该路径不设 probeName，不在数据连接表格管理范围内）
- 不改变探针线本身的绘制样式（虚线/颜色保持现有逻辑）
- 不改变表格单元格内容（仍为 y 值文本），仅改列头
- 不持久化字母到工程文件（探针随会话存在，关闭数据连接窗口即销毁，与现状一致）

---

## 3. 设计决策

### 3.1 决策一：x 轴判断逻辑放在调用方（方案 A）

**选择**：由 `GDataLinkTableTab::onPickerClicked` 负责根据每个 plot 的 x 轴可见性设置探针的 label 属性（`setProbeName`/`setLabelPosition`/`setLabelVisible`），`DADataProbeMarker` 不增加"根据轴决定显示策略"的业务逻辑，仅增强 `drawProbeName` 的绘制能力。

**理由**：`DADataProbeMarker` 的职责是"绘制探针与标签"，显示策略属于使用场景。本次范围只有数据连接窗口一个调用点，marker 保持纯粹可避免耦合，改动最小。

**否决项**：
- marker 自感知（方案 B）：给 marker 加 `autoAdjustLabelByXAxis()` 内部判断轴可见性。逻辑封装但让 marker 耦合使用场景业务，且本次无第二调用点，属提前抽象。
- 工具函数（方案 C）：`resolveProbeLabelByXAxis(QwtPlot*)` 返回结构体。多一层间接，对单调用点略重。

### 3.2 决策二：字母递增不复用

**选择**：新增计数器 `m_probeLabelCounter` 只增不减。删除某列后下一次新点击取下一个字母，不复用已删除的空缺。

**理由**：字母永远唯一不重复，避免与已删但可能仍存在于撤销栈/缓存中的记录混淆。会出现跳号（如 A、C、D），但可接受。

**否决项**：复用空缺字母——尽量连续但可能产生与已删记录同名的字母，虽不冲突却有歧义隐患。

### 3.3 决策三：字母固定到 ClickRecord，删除不重排

**选择**：给 `ClickRecord` 增加 `QString label` 字段存储字母，删除某条 record 时其余 record 的 label 不变。

**理由**：探针字母标识稳定，仍在显示的探针字母不会因删除操作突变。用户选择此行为。

### 3.4 决策四：徽章样式枚举（仅 API 可配置）

**选择**：`DADataProbeMarker` 新增 `LabelStyle` 枚举（四种样式）+ `setLabelStyle`/`labelStyle` API，默认 `RoundedRectBadge`。调用方在创建探针时选定样式。

**理由**：用户要求"让用户有选择权力"，但确认选择权给开发者/API 层而非最终用户，故只需枚举 + setter，无需 UI 入口。默认圆角矩形契合数据连接窗口需求。

---

## 4. 详细设计

### 4.1 DADataProbeMarker 增强（DAFigure 模块）

#### 4.1.1 新增 LabelStyle 枚举

在 `DADataProbeMarker.h` 紧邻 `LabelPosition` 枚举处新增：

```cpp
enum LabelStyle {
    PlainTextBadge = 0,    // 纯文字，无背景框
    RoundedRectBadge,      // 圆角矩形徽章
    RectBadge,             // 直角矩形徽章
    EllipseBadge           // 椭圆/圆形徽章
};
```

#### 4.1.2 PrivateData 新增成员

`DADataProbeMarker.cpp` 的 `PrivateData`（当前 `.cpp:24-29`）增加：

```cpp
LabelStyle labelStyle { RoundedRectBadge };
```

#### 4.1.3 公共 API

```cpp
void setLabelStyle(LabelStyle style);
LabelStyle labelStyle() const;
```

`setLabelStyle` 内部存储后无需触发重绘（`draw` 读取时实时取值），但调用方若改变已 attach 的探针样式应自行 `plot->replot()`。

#### 4.1.4 drawProbeName 按样式分支绘制

增强 `DADataProbeMarker::drawProbeName`（当前 `.cpp:398-429`）：

**统一调整**：
- 增加 padding：水平 4px、垂直 2px，`borderSize = textSize + 2*padding`
- 位置逻辑与边界 clamp 不变（垂直探针按 `LabelPosition` 贴 canvas 顶/底，水平探针贴左/右）

**按样式分支**：
- **PlainTextBadge**：仅画 `QwtText`（沿用现有纯文字绘制），文字颜色取 `probeName` 自身颜色，无背景。
- **RoundedRectBadge**：先 `QPainter::drawRoundedRect`（圆角半径 `min(borderSize.height()/2, 4)`，背景色 `probeColor`），再画白色文字居中。
- **RectBadge**：先 `QPainter::drawRect`（背景色 `probeColor`），再画白色文字居中。
- **EllipseBadge**：先 `QPainter::drawEllipse`（背景色 `probeColor`），再画白色文字居中。

**文字颜色规则**：徽章样式（RoundedRect/Rect/Ellipse）文字固定白色以保证与 `probeColor` 背景的对比度；PlainText 沿用 `probeName` 自身颜色。

#### 4.1.5 避免重复绘制

当前 `updateLabel()`（`.cpp:595-598`）会调用 `QwtPlotMarker::setLabel(d_ptr->probeName)`，而 `draw()`（`.cpp:379-386`）先调 `QwtPlotMarker::draw`（绘制内置 label）再调 `drawProbeName`，导致同一文字被画两次。

**处理**：`updateLabel()` 改为不再向 `QwtPlotMarker` 设内置 label（内置 label 保持空），标签绘制全权由 `drawProbeName` 负责。`setProbeName` 调 `updateLabel` 仅用于同步内部 `probeName` 成员，不触发 QwtPlotMarker 内置 label。

`DAChartItemCreatInteractor` 路径不设 probeName，内置 label 本就为空，无影响。

### 4.2 调用方逻辑（GDataLinkTableTab，方案 A）

#### 4.2.1 字母生成

`GDataLinkTableTab` 新增私有成员：

```cpp
int m_probeLabelCounter { 0 };
```

新增私有静态函数（Excel 风格列号转换）：

```cpp
static QString indexToLetter(int index);
// 0→A, 1→B, ..., 25→Z, 26→AA, 27→AB, ..., 51→AZ, 52→BA, ...
```

实现：标准除 26 取余递进算法（与 Excel 列名相同）。

#### 4.2.2 ClickRecord 增字段

`GDataLinkTableTab.h` 的 `ClickRecord` 结构（当前 `:62-66`）增加：

```cpp
struct ClickRecord {
    double xValue;
    QString label;                          // 新增：字母标识
    QList<DA::DADataProbeMarker*> probes;
};
```

#### 4.2.3 onPickerClicked 流程调整

在 `GDataLinkTableTab::onPickerClicked`（`GDataLinkTableTab.cpp:147-202`）中：

1. 计算 `QString letter = indexToLetter(m_probeLabelCounter++);`
2. 创建 `ClickRecord record; record.xValue = xValue; record.label = letter;`
3. 遍历 `group->pickers()`，对每个 plot 创建 probe 后：
   - `probe->setProbeName(QwtText(letter));`
   - 按 x 轴可见性设置 label 位置与可见性（见 4.2.4）
   - `probe->setLabelStyle(DA::DADataProbeMarker::RoundedRectBadge);`
4. `record.probes.append(probe);`
5. `m_clickRecords.append(record);`

#### 4.2.4 x 轴可见性 → label 定位规则

对每个 probe，根据其 `probe->plot()` 的 x 轴可见性判断（使用定制 QwtPlot 的 `isAxisVisible(QwtAxis::XBottom)` / `isAxisVisible(QwtAxis::XTop)`）：

| XBottom 可见 | XTop 可见 | 结果 |
|:---:|:---:|---|
| ✓ | ✓ | `setLabelPosition(LabelAtBottom)` + `setLabelVisible(true)` |
| ✓ | ✗ | `setLabelPosition(LabelAtBottom)` + `setLabelVisible(true)` |
| ✗ | ✓ | `setLabelPosition(LabelAtTop)` + `setLabelVisible(true)` |
| ✗ | ✗ | `setLabelVisible(false)`（不显示徽章，探针线仍画） |

设置顺序：`setProbeName` → `setLabelPosition` → `setLabelVisible` → `setLabelStyle`。

`probe->plot()` 为空时跳过该 probe 的 label 设置。

### 4.3 表格列头变更（GDataLinkTableModel）

#### 4.3.1 addClickColumn 增字母参数

`GDataLinkTableModel::addClickColumn`（`GDataLinkTableModel.cpp:24-32`）增加字母参数：

```cpp
int addClickColumn(double xValue, const QString& headerText, const QString& letter);
```

- 列头文本设为字母（`letter`）
- 列头 `QStandardItem::setToolTip(headerText)`（原 x 值格式化文本）
- 其余逻辑不变

> **实现注意**：`addClickColumn` 签名变更后，需检查所有调用点（`onPickerClicked`、`refillColumn`、`onRefreshAllColumns` 等）。若 `refillColumn`/`onRefreshAllColumns` 也调用 `addClickColumn` 新增列，需同步传入对应 record 的字母（从 `m_clickRecords` 取 `label`）。若它们仅刷新已有列数据而不新增列，则不受影响。

#### 4.3.2 onPickerClicked 调用调整

`onPickerClicked` 中 `m_model->addClickColumn(xValue, headerText, letter)` 传入字母。`headerText` 仍为 `formatXValueForHeader(xValue)` 的产出，移至 tooltip。

#### 4.3.3 onRenameColumn 同步 probeName

`onRenameColumn`（`GDataLinkTableTab.cpp:239-253`）当前只改表格表头文本。改表头后需同步对应 record 的 `label` 字段及所有 probes 的 `setProbeName`：

```cpp
record.label = newHeaderText;
for (auto* probe : record.probes) {
    probe->setProbeName(QwtText(newHeaderText));
    if (probe->plot()) probe->plot()->replot();
}
```

### 4.4 删除列联动（onDeleteColumn）

`onDeleteColumn`（`GDataLinkTableTab.cpp:255-279`）现有逻辑：按 `idx = col-1` 移除 record、detach+delete 其 probes、replot。

**调整**：保持现有删除逻辑不变。`m_probeLabelCounter` 不回退，其余 record 的 `label` 不变（字母保持原样）。

---

## 5. 边界情况

| 场景 | 处理 |
|------|------|
| 某 plot 的 XBottom、XTop 都不可见 | 该 probe `setLabelVisible(false)`，不画徽章（探针线仍画） |
| 字母超过 Z（第 27 次点击） | Excel 风格 `AA, AB, …`，`indexToLetter` 覆盖 |
| `probe->plot()` 为空 | 跳过该 probe 的 label 设置 |
| 同一 figure 内不同 plot 的 x 轴可见性不同 | 每个 probe 独立判断，互不影响 |
| 删除列后新建点击 | 计数器不回退，取下一个字母（可能出现 A、C、D 跳号） |
| 重命名列 | 同步更新 record.label 与所有 probes 的 probeName + replot |
| 探针未设 probeName | drawProbeName 收到空 QwtText，`textSize` 为零，不绘制（与现状一致） |

---

## 6. 涉及文件清单

| 文件 | 模块 | 改动 |
|------|------|------|
| `data-workbench/src/DAFigure/DADataProbeMarker.h` | DAFigure | 新增 `LabelStyle` 枚举、`setLabelStyle`/`labelStyle` 声明 |
| `data-workbench/src/DAFigure/DADataProbeMarker.cpp` | DAFigure | PrivateData 增 `labelStyle`、`drawProbeName` 分支绘制、`updateLabel` 不设内置 label |
| `src/GDataLinkTableTab.h` | （父级 src） | `ClickRecord` 增 `label` 字段、`m_probeLabelCounter` 成员、`indexToLetter` 声明 |
| `src/GDataLinkTableTab.cpp` | （父级 src） | `onPickerClicked` 流程调整、x 轴定位逻辑、`onRenameColumn` 同步、`indexToLetter` 实现 |
| `src/GDataLinkTableModel.h` | （父级 src） | `addClickColumn` 增字母参数声明 |
| `src/GDataLinkTableModel.cpp` | （父级 src） | `addClickColumn` 列头字母 + tooltip |

> 注：`DADataProbeMarker` 位于 `data-workbench` 仓库；`GDataLinkTable*` 文件位于父级 `gree-data-workbench/src/`（独立路径）。两处改动分别在各自代码树进行。

---

## 7. 测试策略

### 7.1 单元测试（可自动化）

- **indexToLetter**：0→A、25→Z、26→AA、27→AB、51→AZ、52→BA、701→ZZ、702→AAA
- **x 轴 4 组合映射**：将"轴可见性 → label 位置/可见"逻辑提取为纯函数后单测 4 种组合
- **ClickRecord::label 分配**：连续点击 3 次 → A、B、C；删除中间项后新建 → D（计数器不回退）
- **表格列头**：`addClickColumn` 后列头文本为字母、tooltip 为 x 值文本

### 7.2 手动验证（涉及 QPainter，难单测）

- 四种 LabelStyle 徽章视觉效果（圆角矩形/直角矩形/椭圆/纯文字）
- x 轴在底部时标签在底部、在顶部时在顶部、都不可见时无标签
- 删除列后剩余探针字母保持、位置正确
- 重命名列后探针字母同步更新
- 字母超出 Z 后 AA/AB 显示正常

---

## 8. 不在范围内

- 最终用户徽章样式选择 UI（仅 API 可配置）
- `DAChartItemCreatInteractor` 交互式创建路径的 label 适配
- 探针字母持久化到工程文件
- 探针线样式变更
- 表格单元格内容变更
