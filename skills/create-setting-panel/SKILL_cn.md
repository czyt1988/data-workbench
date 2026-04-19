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

1. **定义 PropertyId** — 枚举从 `1` 开始，面板内唯一：`enum PropertyId { PID_Name = 1, PID_Color = 2, ... }`
2. **创建面板** — 继承合适的基类（`DAChartItemSettingPanel` / `QWidget`），构造 `DAPropertyPanelWidget` 实例
3. **buildPropertyPanel** — 调用 `addXxxProperty()` 构建UI。ChartItem 为纯虚函数（子类 ctor 末尾调用），独立面板为 `protected slot`（ctor 中直接调用）
   - 用 `addCollapsibleGroup(title)` → `addXxxProperty(...)` → `endGroup()` 组织分组属性
   - 用 `addSubPanel(id, groupName)` 嵌套独立属性组子面板
4. **连接信号链** — 见下方信号链规范
5. **updateUI** — 用 `QSignalBlocker` 阻止回环，从目标读取状态写入面板
6. **onPropertyValueChanged** — switch/case 读取新值，写回目标，触发刷新
7. **即时应用** — 写完即生效（图表 `replot()`，独立面板调用自身刷新/通知机制）

### 分组与嵌套规则

- 调用 `addCollapsibleGroup(title)` 后，后续所有 `addXxxProperty()` 自动归入该分组
- 调用 `endGroup()` 退出当前分组，回到根布局
- 分组支持嵌套：在分组内部再次调用 `addCollapsibleGroup()` 即可
- `addSubPanel(id, groupName)` 创建嵌套的 `DAPropertyPanelWidget`，拥有独立属性 ID 空间
- 子面板信号自动冒泡：子面板的 `propertyValueChanged` 自动转发到父面板，无需手动连接

!!! danger "强制约定：addCollapsibleGroup 后必须立即调用 endGroup"
    不调用 endGroup 会导致后续所有 addXxxProperty 和 addCollapsibleGroup 都归入当前分组，形成错误的嵌套层次，严重破坏面板布局。
    必须严格遵循：`addCollapsibleGroup(title)` → 添加属性 → `endGroup()`

### 信号链规范

| 类别 | 信号链 |
|------|--------|
| ChartItem（场景A） | mPanel → 基类转发 → `propertyValueChanged` → 子类槽 |
| 独立面板（场景B） | 3跳：`mPanel`→`onPanelPropertyValueChanged`→`emit`→`onPropertyValueChanged` ←P0关键 |
| 嵌套子面板 | 子面板 `propertyValueChanged` → 自动冒泡转发到父面板 `propertyValueChanged` → 无需额外连接 |

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
    auto* pp = propertyPanel();
    // 可折叠分组：外观属性
    pp->addCollapsibleGroup(tr("外观"));
    pp->addStringProperty(PID_Title, tr("标题"));
    pp->addPenProperty(PID_Pen, tr("画笔"));
    pp->endGroup();
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
    // 可折叠分组：节点信息
    mPanel->addCollapsibleGroup(tr("节点信息"));
    mPanel->addStringProperty(PID_Name, tr("名称"));
    mPanel->addColorProperty(PID_Color, tr("颜色"));
    mPanel->endGroup();
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

## 嵌套子面板模式

当一组属性需要独立的属性ID命名空间或单独的更新周期时，使用 `addSubPanel()` 创建嵌套子面板。

```cpp
void MyPanel::buildPropertyPanel() {
    auto* pp = propertyPanel();

    // 根级分组
    pp->addCollapsibleGroup(tr("常规"));
    pp->addStringProperty(PID_Title, tr("标题"));
    pp->endGroup();

    // 嵌套子面板：拥有独立的属性ID空间
    auto* subPanel = pp->addSubPanel(SID_AxisSettings, tr("坐标轴设置"));
    subPanel->addCollapsibleGroup(tr("X 轴"));
    subPanel->addDoubleProperty(PID_XMin, tr("最小值"), 0.0);
    subPanel->addDoubleProperty(PID_XMax, tr("最大值"), 100.0);
    subPanel->endGroup();

    subPanel->addCollapsibleGroup(tr("Y 轴"));
    subPanel->addDoubleProperty(PID_YMin, tr("最小值"), 0.0);
    subPanel->addDoubleProperty(PID_YMax, tr("最大值"), 100.0);
    subPanel->endGroup();
    // 无需额外信号连接 — 子面板 propertyValueChanged 自动冒泡到父面板
}

// 后续通过 getSubPanel 访问子面板更新状态：
void MyPanel::updateUI() {
    auto* subPanel = mPanel->getSubPanel(SID_AxisSettings);
    QSignalBlocker blocker(subPanel);
    subPanel->setDoubleValue(PID_XMin, ...);
    // ...
}
```

**关键规则**：
- 子面板ID（`SID_*`）与根面板ID（`PID_*`）独立，互不冲突
- `getSubPanel(id)` 返回 `DAPropertyPanelWidget*` 可直接读写
- `getSubPanelId(subPanel)` 反向查找子面板ID
- 子面板 `propertyValueChanged` 自动冒泡转发到父面板，无需手动 `connect`

## 使用 DAPropertyPanelContainerWidget

`DAPropertyPanelContainerWidget` 将 `DAPropertyPanelWidget` 封装在 `QScrollArea` 内，并代理其**全部**公共API。当面板内容较多需要滚动时使用此容器。

```cpp
// 容器代理所有 DAPropertyPanelWidget 方法：
DAPropertyPanelContainerWidget* container = new DAPropertyPanelContainerWidget(this);
container->addCollapsibleGroup(tr("设置"));
container->addStringProperty(PID_Name, tr("名称"));
container->endGroup();
connect(container, &DAPropertyPanelContainerWidget::propertyValueChanged, ...);
```

**参考文件**：`src/DACommonWidgets/DAPropertyPanelContainerWidget.h`

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
| **子面板信号** | 自动冒泡到父面板 | 自动冒泡到父面板 | N/A |
| **刷新机制** | `replot()` | 目标自身刷新/通知机制 | `apply()` 在用户点确定时调用 |
| **Qwt专有方法** | 有（addCurveStyleProperty 等） | 无 | 无 |
| **分组方式** | `addCollapsibleGroup` + `endGroup` | `addCollapsibleGroup` + `endGroup` | N/A |

## DAPropertyPanelWidget API速查

### 属性添加与值读写

| 方法 | 用途 | 读写 |
|------|------|------|
| `addColorProperty` / `addFontProperty` / `addBrushProperty` | 颜色/字体/画刷 | `getColorValue`/`setColorValue` 等 |
| `addPenProperty` | 画笔 | `getPenValue`/`setPenValue` |
| `addIntProperty` / `addDoubleProperty` / `addBoolProperty` | 数值/布尔 | `getIntValue`/`setIntValue` 等 |
| `addStringProperty` / `addEnumProperty` | 字符串/枚举 | `getStringValue`/`setStringValue` 等 |
| `addAlignmentProperty` / `addFilePathProperty` | 对齐/路径 | `getAlignmentValue`/`setAlignmentValue` 等 |
| `addProperty` | 自定义 Widget | `getPropertyItem(id)` |

### 分组与嵌套

| 方法 | 用途 | 备注 |
|------|------|------|
| `addCollapsibleGroup(title)` | 开始可折叠分组 | 返回分组ID（从1递增）。后续 addXxxProperty 自动归入此分组 |
| `endGroup()` | 结束当前分组 | 后续 addXxxProperty 回到根布局 |
| `addSubPanel(id, groupName)` | 创建嵌套子面板 | 返回 `DAPropertyPanelWidget*`，信号自动冒泡 |
| `getSubPanel(id)` | 根据ID获取子面板 | 不存在返回 nullptr |
| `getSubPanelId(subPanel)` | 反向查找子面板ID | 不存在返回 -1 |
| `getGroupPanel(groupId)` | 获取分组内部面板 | 返回分组的 `DAPropertyPanelWidget*` |
| `isGroupExpanded(groupId)` | 查询分组展开状态 | 返回 true/false |
| `setGroupExpanded(groupId, expanded)` | 设置分组展开状态 | 程序化控制折叠/展开 |
| `addGroupLabel(text)` | **已弃用**装饰性标签 | 建议使用 `addCollapsibleGroup` 替代，不具备折叠功能 |

### DAPropertyPanelContainerWidget

`DAPropertyPanelContainerWidget` 代理**全部**上述 `DAPropertyPanelWidget` 方法。如需直接访问底层面板，可通过 `rootPanel()` 获取 `DAPropertyPanelWidget*`。

## 参考文件

| 文件 | 用途 |
|------|------|
| `src/DACommonWidgets/DAPropertyPanelWidget.h` | 全部 addXxxProperty + getXxxValue/setXxxValue + 分组/子面板API |
| `src/DACommonWidgets/DACollapsiblePanel.h` | 可折叠面板控件：展开/收起、标题、信号 |
| `src/DACommonWidgets/DAPropertyPanelContainerWidget.h` | 属性面板容器：滚动区域 + API代理 |
| `src/DAGui/ChartSetting/DAChartItemSettingPanel.h` | ChartItem 基类：纯虚 buildPropertyPanel、Qwt 专有方法 |
| `src/DAGui/ChartSetting/DAChartAxisSettingPanel.h/cpp` | 独立面板完整示例：3跳信号链、QButtonGroup 兼容 |
| `src/DAGui/ChartSetting/DAChartItemSettingPanelFactory.h/cpp` | 注册工厂：registerAllKnownPanels() |
| `src/DACommonWidgets/DAAbstractSettingPage.h` | 应用级页面基类：apply、settingChanged、settingApplyed |
| `src/DAGui/DANodeSettingWidget.h` | 现有工作流节点设置参考（非 DAPropertyPanelWidget） |
| `src/DAGui/ChartSetting/DAAbstractChartItemSettingWidget.h` | ReturnWhenItemNull 宏、d_cast/s_cast、replot |