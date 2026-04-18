---
name: create-setting-panel-cn
description: 使用当需要在data-workbench Qt/C++项目中创建属性设置面板时。触发条件：为新的Qwt图表项添加设置面板、为工作流节点/数据过滤器创建属性面板（非绘图目标）、为坐标轴/图表全局级别创建面板、扩展DAChartItemSettingPanelFactory、在DAAbstractSettingPage中创建应用级配置页面。
---

# Create Setting Panel (Chinese)

## 概述

本技能指导在 data-workbench 项目中创建基于 `DAPropertyPanelWidget` 的属性设置面板。采用**即时应用模式**——属性变化直接写回目标对象 + 刷新机制，无需单独 `apply()` 步骤。

## 决策树：哪个类别？

```
需要创建设置面板？
  │
  ├─ 目标是否为 QwtPlotItem 子类（曲线、柱状图、图例等）？
  │   └─ YES → 场景A：ChartItem 面板（继承 DAChartItemSettingPanel）
  │
  ├─ 目标是否为任意非绘图对象（工作流节点、数据过滤器等）？
  │   └─ YES → 场景B：独立属性面板（QWidget + 组合 DAPropertyPanelWidget）
  │
  └─ 是否为应用级设置（如DASettingWidget侧边栏项）？
      └─ YES → 场景C：应用级设置页面（继承 DAAbstractSettingPage）
```

## 核心模式：7步通用流程

所有基于 `DAPropertyPanelWidget` 的面板遵循以下统一流程：

1. **创建面板** — 继承合适的基类（`DAChartItemSettingPanel` / `QWidget`），构造 `DAPropertyPanelWidget` 实例
2. **定义 PropertyId** — 枚举从 `1` 开始，面板内唯一：`enum PropertyId { PID_Name = 1, PID_Color = 2, ... }`
3. **buildPropertyPanel** — 调用 `addXxxProperty()` 构建UI。ChartItem 为纯虚函数（子类 ctor 末尾调用），独立面板为 `protected slot`（ctor 中直接调用）
4. **连接信号链** — 见下方信号链规范
5. **updateUI** — 用 `QSignalBlocker` 阻止回环，从目标读取状态写入面板
6. **onPropertyValueChanged** — switch/case 读取新值，写回目标，触发刷新
7. **即时应用** — 写完即生效（图表 `replot()`，独立面板调用自身刷新/通知机制）

### 信号链规范

| 类别 | 信号链 |
|------|--------|
| ChartItem（场景A） | mPanel → 基类转发 → `propertyValueChanged` → 子类槽 |
| 独立面板（场景B） | 3跳：`mPanel`→`onPanelPropertyValueChanged`→`emit`→`onPropertyValueChanged` ←P0关键 |

### QButtonGroup Qt5/Qt6 兼容

```cpp
mGroup = new QButtonGroup(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mGroup->addButton(rbA, IdA); mGroup->addButton(rbB, IdB);
#else
    mGroup->setId(rbA, IdA); mGroup->addButton(rbA); mGroup->setId(rbB, IdB); mGroup->addButton(rbB);
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(mGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int) { onPanelPropertyValueChanged(PID_XXX); });
#else
    connect(mGroup, &QButtonGroup::idClicked, this, [this](int) { onPanelPropertyValueChanged(PID_XXX); });
#endif
```

## 场景A：ChartItem 设置面板（绘图示例）

**适用于：** QwtPlotItem 子类（曲线、柱状图、图例等）。

### 骨架

```cpp
class MyCurveSettingPanel : public DAChartItemSettingPanel { Q_OBJECT
public:
    enum PropertyId { PID_Title = 1, PID_Pen = 2 };
    explicit MyCurveSettingPanel(QWidget* parent = nullptr);
    void updateUI(QwtPlotItem* item) override;
private: void buildPropertyPanel() override;
private Q_SLOTS: void onMyPropertyChanged(int propertyId); };
```

### 关键实现

```cpp
MyCurveSettingPanel::MyCurveSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &MyCurveSettingPanel::onMyPropertyChanged);
    buildPropertyPanel();
}
void MyCurveSettingPanel::buildPropertyPanel() {
    auto* pp = propertyPanel(); pp->addGroupLabel(tr("General"));
    pp->addStringProperty(PID_Title, tr("Title")); pp->addPenProperty(PID_Pen, tr("Pen"));
}
void MyCurveSettingPanel::onMyPropertyChanged(int propertyId) {
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    auto* curve = d_cast<QwtPlotCurve*>(); if (!curve) return; auto* pp = propertyPanel();
    switch (propertyId) { case PID_Title: curve->setTitle(pp->getStringValue(PID_Title)); break; case PID_Pen: curve->setPen(pp->getPenValue(PID_Pen)); break; }
    replot();
}
void MyCurveSettingPanel::updateUI(QwtPlotItem* item) {
    if (!item) { DAChartItemSettingPanel::updateUI(item); return; }
    auto* curve = d_cast<QwtPlotCurve*>(); DAChartItemSettingPanel::updateUI(item);
    QSignalBlocker blocker(propertyPanel());
    propertyPanel()->setStringValue(PID_Title, curve->title().text()); propertyPanel()->setPenValue(PID_Pen, curve->pen());
}
```

## 场景B：独立属性面板（非绘图目标，如工作流节点）

**适用于：** `setTarget()` 设置任意对象。当前工作流节点设置（`DANodeSettingWidget` 等）不使用 `DAPropertyPanelWidget` — 本场景展示如何用 `DAPropertyPanelWidget` 创建新面板。

### 骨架（DANodePropertyPanel 示例，编辑 DAWorkFlowNode）

```cpp
class DANodePropertyPanel : public QWidget { Q_OBJECT
public:
    enum PropertyId { PID_Name = 1, PID_Color = 2 };
    explicit DANodePropertyPanel(QWidget* parent = nullptr);
    void setTarget(DAAbstractNode::SharedPointer node); void updateUI();
private: void buildPropertyPanel(); void onPropertyValueChanged(int); void onPanelPropertyValueChanged(int);
    DAPropertyPanelWidget* mPanel; DAAbstractNode::WeakPointer mNode; };
```

### 关键实现

```cpp
DANodePropertyPanel::DANodePropertyPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    mPanel = new DAPropertyPanelWidget(this);
    auto* l = new QVBoxLayout(this); l->addWidget(mPanel); setLayout(l);
    connect(mPanel, &DAPropertyPanelWidget::propertyValueChanged, this, &DANodePropertyPanel::onPanelPropertyValueChanged);
    connect(this, &DANodePropertyPanel::propertyValueChanged, this, &DANodePropertyPanel::onPropertyValueChanged);
    buildPropertyPanel();
}
void DANodePropertyPanel::buildPropertyPanel() {
    mPanel->addGroupLabel(tr("Node Info")); mPanel->addStringProperty(PID_Name, tr("Name")); mPanel->addColorProperty(PID_Color, tr("Color"));
}
void DANodePropertyPanel::onPanelPropertyValueChanged(int propertyId) { emit propertyValueChanged(propertyId); }
void DANodePropertyPanel::onPropertyValueChanged(int propertyId) {
    auto node = mNode.toStrongRef(); if (!node) return;
    switch (propertyId) { case PID_Name: node->setName(mPanel->getStringValue(PID_Name)); break; case PID_Color: node->setColor(mPanel->getColorValue(PID_Color)); break; }
    // 非绘图目标：不调用 replot()，调用目标自身刷新/通知机制
    // 例: node->notifyChanged(); 或 emit dataChanged();
}
void DANodePropertyPanel::setTarget(DAAbstractNode::SharedPointer node) { mNode = node; updateUI(); }
void DANodePropertyPanel::updateUI() {
    auto node = mNode.toStrongRef(); DASignalBlockers block(mPanel); if (!node) return;
    mPanel->setStringValue(PID_Name, node->name()); mPanel->setColorValue(PID_Color, node->color());
}
```

## 场景C：应用级设置页面

继承 `DAAbstractSettingPage`，实现 `apply()`、`getSettingPageTitle()`、`getSettingPageIcon()`。**必须发射** `settingChanged()` 通知变更（否则顶层不调用 `apply()`），成功后 `settingApplyed()`。

```cpp
class MySettingPage : public DAAbstractSettingPage { Q_OBJECT public:
    void apply() override; QString getSettingPageTitle() const override; QIcon getSettingPageIcon() const override;
protected: void onValueChanged() { emit settingChanged(); } };
```

## 关键差异对比

| 差异点 | 场景A: ChartItem | 场景B: 独立面板 | 场景C: 应用级页面 |
|--------|-----------------|----------------|-----------------|
| **基类** | `DAChartItemSettingPanel` | `QWidget` | `DAAbstractSettingPage` |
| **Target** | `QwtPlotItem` via `setItem()` | 任意对象 via `setTarget()` | N/A |
| **buildPropertyPanel** | 纯虚函数，子类 ctor 末尾调用 | protected slot，ctor 中直接调用 | 不适用 |
| **信号链** | 基类已转发 `propertyValueChanged` | 3跳链，ctor 必须自行连接 | `settingChanged` / `settingApplyed` |
| **刷新机制** | `replot()` | 目标自身刷新/通知机制 | `apply()` 在用户点确定时调用 |
| **Qwt专有方法** | 有（addCurveStyleProperty 等） | 无 | 无 |

## DAPropertyPanelWidget API速查

| 方法 | 用途 | 读写 |
|------|------|------|
| `addColorProperty` / `addFontProperty` / `addBrushProperty` | 颜色/字体/画刷 | `getColorValue`/`setColorValue` 等 |
| `addPenProperty` | 画笔 | `getPenValue`/`setPenValue` |
| `addIntProperty` / `addDoubleProperty` / `addBoolProperty` | 数值/布尔 | `getIntValue`/`setIntValue` 等 |
| `addStringProperty` / `addEnumProperty` | 字符串/枚举 | `getStringValue`/`setStringValue` 等 |
| `addAlignmentProperty` / `addFilePathProperty` | 对齐/路径 | `getAlignmentValue`/`setAlignmentValue` 等 |
| `addGroupLabel` | 分组标签 | — |
| `addProperty` | 自定义 Widget | `getPropertyItem(id)` |

## 参考文件

| 文件 | 用途 |
|------|------|
| `src/DACommonWidgets/DAPropertyPanelWidget.h` | 全部 addXxxProperty + getXxxValue/setXxxValue |
| `src/DAGui/ChartSetting/DAChartItemSettingPanel.h` | ChartItem 基类：纯虚 buildPropertyPanel、Qwt 专有方法 |
| `src/DAGui/ChartSetting/DAChartAxisSettingPanel.h/cpp` | 独立面板完整示例：3跳信号链、QButtonGroup 兼容 |
| `src/DAGui/ChartSetting/DAChartItemSettingPanelFactory.h/cpp` | 注册工厂：registerAllKnownPanels() |
| `src/DACommonWidgets/DAAbstractSettingPage.h` | 应用级页面基类：apply、settingChanged、settingApplyed |
| `src/DAGui/DANodeSettingWidget.h` | 现有工作流节点设置参考（非 DAPropertyPanelWidget） |
| `src/DAGui/ChartSetting/DAAbstractChartItemSettingWidget.h` | ReturnWhenItemNull 宏、d_cast/s_cast、replot |
