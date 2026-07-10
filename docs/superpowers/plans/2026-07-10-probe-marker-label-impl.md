# 数据探针标记 Label 显示 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为数据连接窗口创建的 `DADataProbeMarker` 探针分配 A/B/C 字母标识，以圆角矩形徽章形式绘制在 x 轴显示侧，并将表格列头改为字母以建立探针与列的直观对应。

**Architecture:** 方案 A（调用方驱动）。`DADataProbeMarker` 新增 `LabelStyle` 枚举（4 种徽章样式）并增强 `drawProbeName` 按样式绘制；`GDataLinkTableTab` 负责字母生成（Excel 风格、递增不复用）、x 轴可见性判断与 label 定位；`GDataLinkTableModel::addClickColumn` 列头改字母 + x 值 tooltip。`ClickRecord` 增 `label` 字段，删除不重排。

**Tech Stack:** C++17, Qt 5.14+/6, 定制 Qwt (qwt_plot_marker, qwt_text, qwt_plot, qwt_axis)

**Spec:** `docs/superpowers/specs/2026-07-10-probe-marker-label-design.md`

**构建命令（Windows）:**
- `DADataProbeMarker` 改动：`.\scripts\build.ps1 -Target DAFigure`（data-workbench 仓库）
- `GDataLinkTable*` 改动：父级 `gree-data-workbench` 构建为插件 SHARED 库，无独立 Target，用 `.\scripts\build.ps1 -Full` 或 IDE 编译 `GreeDataWorkbench` 项目
- 注意：两个代码树在不同仓库（`data-workbench/src/DAFigure/` vs 父级 `src/`），需分别在各自上下文修改

---

## 文件结构

### 修改文件（DAFigure 模块 — data-workbench 仓库）

| 文件 | 责任 | 改动 |
|------|------|------|
| `data-workbench/src/DAFigure/DADataProbeMarker.h` | 探针标记类声明 | 新增 `LabelStyle` 枚举、`setLabelStyle`/`labelStyle` 声明 |
| `data-workbench/src/DAFigure/DADataProbeMarker.cpp` | 探针标记实现 | PrivateData 增 `labelStyle`、实现 setter/getter、`drawProbeName` 按样式分支绘制、`updateLabel` 不设内置 label |

### 修改文件（GDataLink — 父级 gree-data-workbench 仓库）

| 文件 | 责任 | 改动 |
|------|------|------|
| `src/GDataLinkTableTab.h` | 数据连接 tab 声明 | `ClickRecord` 增 `label` 字段、`m_probeLabelCounter` 成员、`indexToLetter` 静态方法声明、`resolveProbeLabelByXAxis` 静态方法声明 |
| `src/GDataLinkTableTab.cpp` | 数据连接 tab 实现 | `indexToLetter` 实现、`resolveProbeLabelByXAxis` 实现、`onPickerClicked` 流程调整（字母 + label 定位）、`onRenameColumn` 同步 probeName、`onDeleteColumn` 无需改（计数器不回退已天然满足） |
| `src/GDataLinkTableModel.h` | 表格模型声明 | `addClickColumn` 增 `letter` 参数 |
| `src/GDataLinkTableModel.cpp` | 表格模型实现 | `addClickColumn` 列头字母 + tooltip |

---

## Task 1: DADataProbeMarker 新增 LabelStyle 枚举与 API

**Files:**
- Modify: `data-workbench/src/DAFigure/DADataProbeMarker.h`
- Modify: `data-workbench/src/DAFigure/DADataProbeMarker.cpp`

- [ ] **Step 1: 在 .h 新增 LabelStyle 枚举与 getter/setter 声明**

在 `DADataProbeMarker.h` 中，`LabelPosition` 枚举（`:62-66`）之后、`CapturedData` 结构（`:76`）之前插入：

```cpp
    /**
     * \if ENGLISH
     * @brief Label style enumeration for probe name rendering
     * \endif
     * \if CHINESE
     * @brief 探针名称标签样式枚举
     * \endif
     */
    enum LabelStyle
    {
        PlainTextBadge = 0,   ///< Plain text only, no background
        RoundedRectBadge,     ///< Rounded rectangle badge with background
        RectBadge,            ///< Rectangle badge with background (no rounded corners)
        EllipseBadge          ///< Ellipse/circle badge with background
    };
```

在 `setLabelVisible`/`isLabelVisible`（`:110-111`）之后、`setProbeColor`（`:114`）之前插入声明：

```cpp
    // Label style (badge rendering style)
    void setLabelStyle(LabelStyle style);
    LabelStyle labelStyle() const;
```

- [ ] **Step 2: 在 .cpp PrivateData 增 labelStyle 成员**

在 `DADataProbeMarker.cpp` 的 `PrivateData`（`:24-29`）中，`labelVisible` 之后新增成员：

```cpp
    bool labelVisible { true };
    DADataProbeMarker::LabelStyle labelStyle { DADataProbeMarker::RoundedRectBadge };
```

- [ ] **Step 3: 在 .cpp 实现 setLabelStyle/labelStyle**

在 `isLabelVisible()`（`:263-266`）之后、`setProbeColor()`（`:279`）之前插入：

```cpp
void DADataProbeMarker::setLabelStyle(LabelStyle style)
{
    d_ptr->labelStyle = style;
}

DADataProbeMarker::LabelStyle DADataProbeMarker::labelStyle() const
{
    return d_ptr->labelStyle;
}
```

- [ ] **Step 4: 编译验证 DAFigure 模块**

Run: `.\scripts\build.ps1 -Target DAFigure`
Expected: 编译通过，无错误（新增的枚举和 getter/setter 尚未被使用，但应编译通过）

- [ ] **Step 5: Commit**

```bash
cd data-workbench
git add src/DAFigure/DADataProbeMarker.h src/DAFigure/DADataProbeMarker.cpp
git commit -m "feat(DAFigure): add LabelStyle enum and getter/setter to DADataProbeMarker

新增 LabelStyle 枚举（PlainText/RoundedRect/Rect/Ellipse）及
setLabelStyle/labelStyle API，为探针标签徽章样式提供配置能力。
默认 RoundedRectBadge。"
```

---

## Task 2: 增强 drawProbeName 按样式分支绘制徽章

**Files:**
- Modify: `data-workbench/src/DAFigure/DADataProbeMarker.cpp`

- [ ] **Step 1: 重写 drawProbeName 方法**

将 `DADataProbeMarker.cpp:398-429` 的 `drawProbeName` 整体替换为：

```cpp
void DA::DADataProbeMarker::drawProbeName(
    QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, const QwtText& name
) const
{
    DA_DC(d);
    QPointF pos     = value();
    double x        = xMap.transform(pos.x());
    double y        = yMap.transform(pos.y());
    QSizeF textSize = name.textSize();
    // padding for badge styles
    const double padX = 4.0;
    const double padY = 2.0;
    QSizeF borderSize = textSize + QSizeF(2 * padX, 2 * padY);

    QPointF labelPos;
    if (d->probeType == VerticalProbe) {
        if (d->labelPosition == LabelAtTop) {
            labelPos = QPointF(x - borderSize.width() / 2, canvasRect.top());
        } else {
            labelPos = QPointF(x - borderSize.width() / 2, canvasRect.bottom() - borderSize.height());
        }
    } else {
        if (d->labelPosition == LabelAtTop) {
            labelPos = QPointF(canvasRect.left(), y - borderSize.height() / 2);
        } else {
            labelPos = QPointF(canvasRect.right() - borderSize.width(), y - borderSize.height() / 2);
        }
    }

    // clamp to canvas bounds
    labelPos.setX(qBound(canvasRect.left(), labelPos.x(), canvasRect.right() - borderSize.width()));
    labelPos.setY(qBound(canvasRect.top(), labelPos.y(), canvasRect.bottom() - borderSize.height()));

    painter->save();

    QRectF badgeRect(labelPos.x(), labelPos.y(), borderSize.width(), borderSize.height());

    if (d->labelStyle == PlainTextBadge) {
        // plain text: draw directly, use name's own color
        QRectF drawTextRect(labelPos.x() + padX, labelPos.y() + padY, textSize.width(), textSize.height());
        name.draw(painter, drawTextRect);
    } else {
        // badge styles: draw background shape first, then white text centered
        painter->setBrush(d->probeColor);
        painter->setPen(Qt::NoPen);

        if (d->labelStyle == RoundedRectBadge) {
            double radius = qMin(borderSize.height() / 2.0, 4.0);
            painter->drawRoundedRect(badgeRect, radius, radius);
        } else if (d->labelStyle == RectBadge) {
            painter->drawRect(badgeRect);
        } else {  // EllipseBadge
            painter->drawEllipse(badgeRect);
        }

        // draw white text centered in badge
        QwtText badgeText = name;
        badgeText.setColor(Qt::white);
        QRectF drawTextRect(labelPos.x() + padX, labelPos.y() + padY, textSize.width(), textSize.height());
        badgeText.draw(painter, drawTextRect);
    }

    painter->restore();
}
```

- [ ] **Step 2: 修改 updateLabel 避免重复绘制**

将 `DADataProbeMarker.cpp:595-598` 的 `updateLabel` 方法替换为（不再向 QwtPlotMarker 设内置 label，标签绘制全权由 drawProbeName 负责）：

```cpp
void DADataProbeMarker::updateLabel()
{
    // Label rendering is fully handled by drawProbeName().
    // Do not set QwtPlotMarker's built-in label to avoid duplicate drawing.
}
```

- [ ] **Step 3: 编译验证 DAFigure 模块**

Run: `.\scripts\build.ps1 -Target DAFigure`
Expected: 编译通过，无错误

- [ ] **Step 4: Commit**

```bash
cd data-workbench
git add src/DAFigure/DADataProbeMarker.cpp
git commit -m "feat(DAFigure): enhance drawProbeName with badge styles and fix duplicate label

drawProbeName 按四种 LabelStyle 分支绘制：
- PlainTextBadge: 纯文字无背景
- RoundedRectBadge: 圆角矩形徽章（默认）
- RectBadge: 直角矩形徽章
- EllipseBadge: 椭圆徽章
徽章样式背景色取 probeColor，文字白色。
updateLabel 不再设 QwtPlotMarker 内置 label，避免与 drawProbeName 重复绘制。"
```

---

## Task 3: GDataLinkTableTab 新增 indexToLetter 与 resolveProbeLabelByXAxis

**Files:**
- Modify: `src/GDataLinkTableTab.h`（父级仓库）
- Modify: `src/GDataLinkTableTab.cpp`（父级仓库）

- [ ] **Step 1: 在 .h 新增成员与方法声明**

在 `GDataLinkTableTab.h` 中做三处修改：

(a) `ClickRecord` 结构（`:62-66`）增加 `label` 字段：

```cpp
    struct ClickRecord
    {
        double xValue;
        QString label;  // letter identifier (A, B, C, ... AA, AB, ...)
        QList< DA::DADataProbeMarker* > probes;
    };
```

(b) 在 `cleanupAllProbes();`（`:48`）之后新增私有方法声明：

```cpp
    static QString indexToLetter(int index);
    // resolve label visibility & position by x-axis visibility
    // outAtTop: true=label at top (xtop only), false=label at bottom (xbottom visible)
    // outVisible: false if neither x axis is visible
    static void resolveProbeLabelByXAxis(QwtPlot* plot, bool& outVisible, bool& outAtTop);
```

> **注意**：不使用 `DA::DADataProbeMarker::LabelPosition` 作为参数类型，因为 `GDataLinkTableTab.h` 对 `DADataProbeMarker` 只有前向声明（`:20`），嵌套类型需要完整类定义。改用 `bool& outAtTop`，在 .cpp 调用处转成 `LabelPosition`，保持头文件前向声明的简洁风格。

(c) 在 `int m_contextMenuColumn { -1 };`（`:60`）之后新增成员：

```cpp
    int m_probeLabelCounter { 0 };
```

- [ ] **Step 2: 确认 .cpp 头文件包含**

确认 `GDataLinkTableTab.cpp` 顶部已包含 `qwt_plot.h`（`:17` 已有）和 `DADataProbeMarker.h`（`:15` 已有）。

新增 `qwt_axis.h` 包含（用于 `QwtAxis::XBottom`/`XTop`）。在 `#include "qwt_plot.h"`（`:17`）之后插入：

```cpp
#include "qwt_axis.h"
```

- [ ] **Step 3: 在 .cpp 实现 indexToLetter**

在 `extractXValue`（`:133-145`）之后插入：

```cpp
QString GDataLinkTableTab::indexToLetter(int index)
{
    // Excel-style column letter: 0->A, 1->B, ..., 25->Z, 26->AA, 27->AB, ...
    if (index < 0)
        return QString();
    QString result;
    int n = index;
    do {
        result.prepend(QChar('A' + (n % 26)));
        n = n / 26 - 1;
    } while (n >= 0);
    return result;
}
```

- [ ] **Step 4: 在 .cpp 实现 resolveProbeLabelByXAxis**

在 `indexToLetter` 之后插入：

```cpp
void GDataLinkTableTab::resolveProbeLabelByXAxis(QwtPlot* plot, bool& outVisible, bool& outAtTop)
{
    if (!plot) {
        outVisible = false;
        outAtTop = false;
        return;
    }
    bool xBottomVisible = plot->isAxisVisible(QwtAxis::XBottom);
    bool xTopVisible = plot->isAxisVisible(QwtAxis::XTop);

    if (xBottomVisible) {
        // xbottom visible (with or without xtop): label at bottom
        outVisible = true;
        outAtTop = false;
    } else if (xTopVisible) {
        // only xtop visible: label at top
        outVisible = true;
        outAtTop = true;
    } else {
        // neither x axis visible: no label
        outVisible = false;
        outAtTop = false;
    }
}
```

- [ ] **Step 5: 编译验证（父级插件库）**

Run: 在 IDE 中编译 `GreeDataWorkbench` 项目，或 `.\scripts\build.ps1 -Full`
Expected: 编译通过（新方法尚未被调用，但应编译通过）

- [ ] **Step 6: Commit**

```bash
cd ..   # 回到父级 gree-data-workbench 仓库
git add src/GDataLinkTableTab.h src/GDataLinkTableTab.cpp
git commit -m "feat(GDataLink): add indexToLetter and resolveProbeLabelByXAxis helpers

- indexToLetter: Excel 风格列号转字母（0->A, 26->AA）
- resolveProbeLabelByXAxis: 根据 plot 的 x 轴可见性决定 label 位置
  (xbottom->底部, xtop->顶部, 都不可见->不显示)
- ClickRecord 增 label 字段
- 新增 m_probeLabelCounter 成员（递增不复用）"
```

---

## Task 4: GDataLinkTableModel::addClickColumn 增字母参数

**Files:**
- Modify: `src/GDataLinkTableModel.h`（父级仓库）
- Modify: `src/GDataLinkTableModel.cpp`（父级仓库）

- [ ] **Step 1: 修改 .h 的 addClickColumn 签名**

将 `GDataLinkTableModel.h:25` 的声明改为：

```cpp
    int addClickColumn(double xValue, const QString& headerText = QString(), const QString& letter = QString());
```

- [ ] **Step 2: 修改 .cpp 的 addClickColumn 实现**

将 `GDataLinkTableModel.cpp:24-32` 的方法替换为：

```cpp
int GDataLinkTableModel::addClickColumn(double xValue, const QString& headerText, const QString& letter)
{
    int col = columnCount();
    setColumnCount(col + 1);
    // header text: letter if provided, otherwise formatted x value
    QString displayText = letter.isEmpty() ? (headerText.isEmpty() ? formatXValue(xValue) : headerText) : letter;
    auto* headerItem = new QStandardItem(displayText);
    // tooltip: original x value text for reference
    QString tooltip = headerText.isEmpty() ? formatXValue(xValue) : headerText;
    headerItem->setToolTip(tooltip);
    setHorizontalHeaderItem(col, headerItem);
    m_columnXValues.append(xValue);
    return col;
}
```

- [ ] **Step 3: 编译验证**

Run: 编译 `GreeDataWorkbench` 项目
Expected: 编译通过（`onPickerClicked` 尚未传 letter 参数，用默认空值，行为不变）

- [ ] **Step 4: Commit**

```bash
git add src/GDataLinkTableModel.h src/GDataLinkTableModel.cpp
git commit -m "feat(GDataLink): addClickColumn supports letter header with x-value tooltip

addClickColumn 新增 letter 参数：列头显示字母，原 x 值文本作为
列头 tooltip。letter 为空时保持原有行为（兼容）。"
```

---

## Task 5: onPickerClicked 集成字母分配与 label 定位

**Files:**
- Modify: `src/GDataLinkTableTab.cpp`（父级仓库）

- [ ] **Step 1: 修改 onPickerClicked 方法**

将 `GDataLinkTableTab.cpp:147-202` 的 `onPickerClicked` 整体替换为：

```cpp
void GDataLinkTableTab::onPickerClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos)
{
    if (!m_figureWidget)
        return;
    QwtPlotSeriesDataPickerGroup* group = m_figureWidget->getDataPickerGroup();
    if (!group)
        return;

    double xValue = extractXValue(group);
    QString headerText = formatXValueForHeader(xValue);
    // generate letter identifier (A, B, C, ... AA, AB, ...)
    QString letter = indexToLetter(m_probeLabelCounter++);
    int newCol = m_model->addClickColumn(xValue, headerText, letter);
    qDebug() << "[DataLinkTable] onPickerClicked: xValue=" << xValue
             << "headerText=" << headerText << "letter=" << letter << "newCol=" << newCol;

    QList< QwtPlotSeriesDataPicker* > pickers = group->pickers();
    ClickRecord record;
    record.xValue = xValue;
    record.label = letter;

    for (QwtPlotSeriesDataPicker* p : pickers) {
        QwtPlot* plot = p->plot();
        if (!plot)
            continue;

        QString plotTitle = plot->title().text();
        if (plotTitle.isEmpty())
            plotTitle = QString::fromUtf8("绘图 %1").arg(plot->plotId());
        QStandardItem* plotRow = m_model->ensurePlotRow(plot, plotTitle);

        QList< QwtPlotSeriesDataPicker::FeaturePoint > fps = p->featurePoints();
        qDebug() << "[DataLinkTable] onPickerClicked: plot=" << plotTitle
                 << "featurePoints count=" << fps.size();
        for (const auto& fp : fps) {
            if (!fp.item)
                continue;
            QString curveTitle = fp.item->title().text();
            if (curveTitle.isEmpty())
                curveTitle = QString::fromUtf8("曲线");
            QColor color = QwtPlotStyling::color(fp.item);
            QStandardItem* curveRow = m_model->ensureCurveRow(plotRow, fp.item, curveTitle, color);

            QString yStr = QString::number(fp.feature.y(), 'g', 6);
            m_model->setCellData(curveRow, newCol, yStr);
            qDebug() << "[DataLinkTable]   curve=" << curveTitle << "y=" << yStr;
        }

        DA::DADataProbeMarker* probe = new DA::DADataProbeMarker(DA::DADataProbeMarker::VerticalProbe);
        probe->setXValue(xValue);
        probe->setProbeName(QwtText(letter));
        // determine label position by x-axis visibility
        bool labelVisible = true;
        bool labelAtTop = false;
        resolveProbeLabelByXAxis(plot, labelVisible, labelAtTop);
        probe->setLabelPosition(labelAtTop ? DA::DADataProbeMarker::LabelAtTop
                                           : DA::DADataProbeMarker::LabelAtBottom);
        probe->setLabelVisible(labelVisible);
        probe->setLabelStyle(DA::DADataProbeMarker::RoundedRectBadge);
        probe->attach(plot);
        probe->captureData(true);
        plot->replot();
        record.probes.append(probe);
    }

    m_clickRecords.append(record);
    m_model->updateVisibility();
}
```

- [ ] **Step 2: 编译验证**

Run: 编译 `GreeDataWorkbench` 项目
Expected: 编译通过

- [ ] **Step 3: 手动验证 — 字母分配与徽章显示**

1. 运行主程序，打开一个含多条曲线的 figure
2. 在数据连接窗口激活状态下，在绘图上点击 3 次不同 x 位置
3. 验证：每次点击在对应 plot 上出现红色虚线探针 + 圆角矩形徽章（字母 A、B、C）
4. 验证：表格列头显示 A、B、C，鼠标悬停列头显示 x 值 tooltip

- [ ] **Step 4: 手动验证 — x 轴可见性定位**

1. 使用含寄生轴的 figure（部分 plot 只显示 xtop 不显示 xbottom，或都不显示）
2. 点击探针
3. 验证：xbottom 显示的 plot 徽章在底部；只 xtop 显示的 plot 徽章在顶部；都不显示的 plot 无徽章（只有虚线）
4. 验证：xbottom + xtop 都显示的 plot 徽章在底部（默认）

- [ ] **Step 5: Commit**

```bash
git add src/GDataLinkTableTab.cpp
git commit -m "feat(GDataLink): integrate letter labels and x-axis-driven positioning

onPickerClicked 改动：
- 每次点击生成字母（indexToLetter, 递增不复用）
- 探针 setProbeName 设字母，setRoundedRectBadge 样式
- 根据每个 plot 的 x 轴可见性决定 label 位置/可见
- ClickRecord.label 存储字母
- addClickColumn 传字母，列头显示字母+x值tooltip"
```

---

## Task 6: onRenameColumn 同步探针 probeName

**Files:**
- Modify: `src/GDataLinkTableTab.cpp`（父级仓库）

- [ ] **Step 1: 修改 onRenameColumn 方法**

将 `GDataLinkTableTab.cpp:239-253` 的 `onRenameColumn` 整体替换为：

```cpp
void GDataLinkTableTab::onRenameColumn()
{
    if (m_contextMenuColumn <= 0)
        return;
    auto* header = m_model->horizontalHeaderItem(m_contextMenuColumn);
    if (!header)
        return;
    bool ok = false;
    QString newName = QInputDialog::getText(this, QString::fromUtf8("重命名"),
                                            QString::fromUtf8("新名称:"),
                                            QLineEdit::Normal, header->text(), &ok);
    if (ok && !newName.isEmpty()) {
        m_model->renameColumn(m_contextMenuColumn, newName);
        // sync probe name so the badge text matches the header
        int idx = m_contextMenuColumn - 1;
        if (idx >= 0 && idx < m_clickRecords.size()) {
            ClickRecord& record = m_clickRecords[idx];
            record.label = newName;
            for (DA::DADataProbeMarker* probe : record.probes) {
                probe->setProbeName(QwtText(newName));
                QwtPlot* plot = probe->plot();
                if (plot)
                    plot->replot();
            }
        }
    }
}
```

- [ ] **Step 2: 编译验证**

Run: 编译 `GreeDataWorkbench` 项目
Expected: 编译通过

- [ ] **Step 3: 手动验证 — 重命名同步**

1. 点击产生探针 A
2. 右键列头 A → 重命名 → 输入 "X"
3. 验证：表格列头变为 "X"，绘图上探针徽章文字也变为 "X"

- [ ] **Step 4: Commit**

```bash
git add src/GDataLinkTableTab.cpp
git commit -m "feat(GDataLink): sync probe name on column rename

onRenameColumn 改表头后同步更新对应 record.label 和所有 probes
的 setProbeName + replot，保持探针徽章与列头一致。"
```

---

## Task 7: 验证删除列不重排与边界情况

**Files:**
- 无文件修改（验证现有删除逻辑 + 计数器不回退已天然满足）

- [ ] **Step 1: 手动验证 — 删除列后字母保持**

1. 点击 3 次产生探针 A、B、C
2. 右键列头 B → 删除
3. 验证：剩余探针显示 A、C（字母不重排）
4. 验证：表格剩余列头 A、C
5. 再次点击产生新探针
6. 验证：新探针字母为 D（计数器不回退，不复用 B）

- [ ] **Step 2: 手动验证 — 字母超 Z**

1. 连续点击 27 次（或模拟高频点击）
2. 验证：第 27 个探针字母为 AA（`indexToLetter(26)` = "AA"）

> 若手动点击 27 次不现实，可临时在 `onPickerClicked` 开头加 `qDebug() << indexToLetter(m_probeLabelCounter);` 观察输出，或信任 `indexToLetter` 算法的正确性（标准 Excel 列名算法）。

- [ ] **Step 3: 手动验证 — 四种 LabelStyle**

1. 临时修改 `onPickerClicked` 中 `probe->setLabelStyle(...)` 为各样式，分别编译运行验证：
   - `PlainTextBadge`：纯文字无背景框
   - `RoundedRectBadge`（默认）：圆角矩形红底白字
   - `RectBadge`：直角矩形红底白字
   - `EllipseBadge`：椭圆红底白字
2. 验证完成后恢复为 `RoundedRectBadge`

- [ ] **Step 4: 无需 commit（纯验证步骤）**

如果 Step 3 临时修改了代码，确保恢复为 `RoundedRectBadge` 后：

```bash
git diff --stat  # 确认无残留改动
```

---

## 实现顺序总结

| Task | 内容 | 仓库 |
|------|------|------|
| 1 | DADataProbeMarker LabelStyle 枚举 + API | data-workbench |
| 2 | drawProbeName 徽章绘制 + updateLabel 去重 | data-workbench |
| 3 | GDataLinkTableTab indexToLetter + resolveProbeLabelByXAxis + ClickRecord.label | 父级 |
| 4 | GDataLinkTableModel addClickColumn 字母参数 | 父级 |
| 5 | onPickerClicked 集成字母 + label 定位 | 父级 |
| 6 | onRenameColumn 同步 probeName | 父级 |
| 7 | 删除/边界/样式验证 | 无改动 |

**关键依赖**：Task 1→2（同文件递进）；Task 3→5（resolveProbeLabelByXAxis 被 onPickerClicked 调用）；Task 4→5（addClickColumn 签名变更被 onPickerClicked 调用）。Task 1-2 必须先于 Task 5 编译（DADataProbeMarker 新 API 被 onPickerClicked 调用，需 data-workbench 先安装）。

**跨仓库注意**：Task 1-2 在 `data-workbench` 仓库完成并 `cmake --install` 后，父级仓库才能引用新的 `setLabelStyle` 等 API。若 data-workbench 未重新安装，Task 5 编译会报 `setLabelStyle` 未定义。确保 Task 2 完成后执行安装步骤（`.\scripts\build.ps1 -Target DAFigure` 会触发安装）。
