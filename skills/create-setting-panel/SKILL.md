---
name: create-setting-panel
description: Use when creating property panels or configuration UI in the data-workbench Qt/C++ project. Triggers include "add a setting panel", "create property panel", "node settings", "workflow property editor", "chart item settings", or "app settings page".
---

# Create Setting Panel

## Overview

Setting panels in data-workbench use `DAPropertyPanelWidget` for property UI. Three patterns exist. Identify your target before implementing.

## Decision Tree

```
What are you configuring?
  QwtPlotItem (curve, bar, grid)?
    → Scenario A: ChartItem Panel (inherit DAChartItemSettingPanel)
  Arbitrary object (node, custom data model)?
    → Scenario B: Standalone Panel (inherit QWidget + DAPropertyPanelWidget)
  Application-level config (preferences, options)?
    → Scenario C: App Setting Page (inherit DAAbstractSettingPage)
```

## Core Pattern — 7 Universal Steps

These steps apply to **all** DAPropertyPanelWidget-based panels (Scenarios A and B):

1. **Define PropertyId enum** — values start from 1
2. **Create DAPropertyPanelWidget** in constructor
3. **Add properties** — call `addXxxProperty(PID_*, name, ...)` in `buildPropertyPanel()`
   - Group properties with `addCollapsibleGroup(title)` → `addXxxProperty(...)` → `endGroup()`
   - Nest sub-panels with `addSubPanel(id, groupName)` for independent property groups
4. **Connect signal chain** — `mPanel->propertyValueChanged` to handler slot
5. **Dispatch in handler** — `switch(propertyId)` with `case PID_*:` blocks
6. **Read values** — use `getxxxValue(PID_*)` in handler
7. **Write back** — apply to target + call target's refresh mechanism

### Grouping & Nesting Rules

- After `addCollapsibleGroup(title)`, all subsequent `addXxxProperty()` calls go into that group automatically
- Call `endGroup()` to exit the current group and return to root-level layout
- Groups can be nested: call `addCollapsibleGroup()` inside another group
- `addSubPanel(id, groupName)` creates a nested `DAPropertyPanelWidget` with its own property IDs
- Sub-panel signals bubble up: `propertyValueChanged` from sub-panel forwards to parent panel automatically

### Signal Chain

| Scenario | Signal Chain |
|----------|-------------|
| ChartItem (A) | mPanel → base class forwards → `propertyValueChanged` → subclass slot |
| Standalone (B) | 3-hop: `mPanel`→`onPanelPropertyValueChanged`→`emit`→`onPropertyValueChanged` ← P0 critical |
| Nested sub-panel | subPanel `propertyValueChanged` → auto forwards to parent `propertyValueChanged` → no extra wiring needed |

## Scenario A: ChartItem Panel (QwtPlotItem)

**Inheritance**: `DAChartItemSettingPanel` → `DAAbstractChartItemSettingWidget` → `QWidget`

**Key pattern**:
- `buildPropertyPanel()` is **pure virtual**, called at end of subclass constructor
- `updateUI(QwtPlotItem*)` reads state back (`QSignalBlocker` recommended)
- Signal chain is **automatic** via base class
- Refresh by calling `replot()`

**Skeleton**:
```cpp
class MyPanel : public DAChartItemSettingPanel {
    Q_OBJECT
public:
    enum PropertyID { PID_Title = 1, PID_Color = 2, PID_LineWidth = 3 };
    explicit MyPanel(QWidget* parent = nullptr);
    void updateUI(QwtPlotItem* item) override;
protected:
    void buildPropertyPanel() override;
private Q_SLOTS:
    void onPropertyValueChanged(int propertyId);
};

MyPanel::MyPanel(QWidget* parent) : DAChartItemSettingPanel(parent) {
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &MyPanel::onPropertyValueChanged);
    buildPropertyPanel();
}

void MyPanel::buildPropertyPanel() {
    auto* pp = propertyPanel();
    // Collapsible group for appearance properties
    pp->addCollapsibleGroup(tr("Appearance"));
    pp->addStringProperty(PID_Title, tr("Title"));
    pp->addColorProperty(PID_Color, tr("Color"));
    pp->addIntProperty(PID_LineWidth, tr("Line Width"), 1, 1, 100);
    pp->endGroup();
}
```

Register via `DAChartItemSettingPanelFactory::registerPanel(rtti, creator)`.

**References**: `src/DAGui/ChartSetting/DAChartItemSettingPanel.h`, `src/DAGui/ChartSetting/DAChartCurveSettingPanel.{h,cpp}`

## Scenario B: Standalone Panel (Any Target)

**Inheritance**: `QWidget` directly, owns `DAPropertyPanelWidget*`

**Key pattern**:
- `buildPropertyPanel()` is a **protected slot**, called directly in constructor
- Constructor takes a target identifier (e.g., node type, object ID)
- **Signal chain MUST be manually wired** — two connections (see skeleton)
- Refresh by calling target's own notify/redraw mechanism (NOT `replot()`)

**Skeleton** (workflow node example):
```cpp
class DANodePropertyPanel : public QWidget {
    Q_OBJECT
public:
    enum PropertyId { PID_Name = 1, PID_Color = 2 };
    explicit DANodePropertyPanel(QWidget* parent = nullptr);
    void setTarget(DAWorkFlowNode* node);
protected Q_SLOTS:
    void buildPropertyPanel();
    void onPanelPropertyValueChanged(int propertyId);
    void onPropertyValueChanged(int propertyId);
Q_SIGNALS:
    void propertyValueChanged(int propertyId);
};

// Constructor: wire BOTH signal links
DANodePropertyPanel::DANodePropertyPanel(QWidget* parent)
    : QWidget(parent) {
    mPanel = new DAPropertyPanelWidget(this);
    // Link 1: panel → forwarding slot
    connect(mPanel, &DAPropertyPanelWidget::propertyValueChanged,
            this, &DANodePropertyPanel::onPanelPropertyValueChanged);
    // Link 2: self → handler slot (P0: often forgotten)
    connect(this, &DANodePropertyPanel::propertyValueChanged,
            this, &DANodePropertyPanel::onPropertyValueChanged);
    buildPropertyPanel();
}

void DANodePropertyPanel::buildPropertyPanel() {
    // Collapsible group for node info
    mPanel->addCollapsibleGroup(tr("Node Info"));
    mPanel->addStringProperty(PID_Name, tr("Name"));
    mPanel->addColorProperty(PID_Color, tr("Color"));
    mPanel->endGroup();
}
```

**Note**: Non-chart targets do not call `replot()`. Emit the target object's own change signal or call its refresh method instead.

**References**: `src/DAGui/ChartSetting/DAChartAxisSettingPanel.{h,cpp}` (composition pattern), `src/DAGui/DAWorkFlowNodeItemSettingWidget.h` (target domain)

## Nested Sub-Panel Pattern

Use `addSubPanel()` when a group of properties needs its own independent property ID namespace or separate update cycle.

```cpp
void MyPanel::buildPropertyPanel() {
    auto* pp = propertyPanel();

    // Root-level group
    pp->addCollapsibleGroup(tr("General"));
    pp->addStringProperty(PID_Title, tr("Title"));
    pp->endGroup();

    // Nested sub-panel with its own property IDs
    auto* subPanel = pp->addSubPanel(SID_AxisSettings, tr("Axis Settings"));
    subPanel->addCollapsibleGroup(tr("X Axis"));
    subPanel->addDoubleProperty(PID_XMin, tr("Min"), 0.0);
    subPanel->addDoubleProperty(PID_XMax, tr("Max"), 100.0);
    subPanel->endGroup();

    subPanel->addCollapsibleGroup(tr("Y Axis"));
    subPanel->addDoubleProperty(PID_YMin, tr("Min"), 0.0);
    subPanel->addDoubleProperty(PID_YMax, tr("Max"), 100.0);
    subPanel->endGroup();
    // No extra signal wiring needed — sub-panel propertyValueChanged
    // bubbles up to parent panel automatically
}

// Access sub-panel later for state updates:
void MyPanel::updateUI() {
    auto* subPanel = mPanel->getSubPanel(SID_AxisSettings);
    QSignalBlocker blocker(subPanel);
    subPanel->setDoubleValue(PID_XMin, ...);
    // ...
}
```

**Key rules**:
- Sub-panel IDs (`SID_*`) are separate from root panel IDs (`PID_*`), no collision
- `getSubPanel(id)` returns the `DAPropertyPanelWidget*` for direct read/write
- `getSubPanelId(subPanel)` reverse-lookups the ID from a pointer
- Sub-panel `propertyValueChanged` auto-forwards to parent, no manual `connect` needed

## Using DAPropertyPanelContainerWidget

`DAPropertyPanelContainerWidget` wraps `DAPropertyPanelWidget` inside a `QScrollArea` and proxies **all** its public API. Use it when the panel may grow tall and needs scrolling.

```cpp
// Container delegates all DAPropertyPanelWidget methods:
DAPropertyPanelContainerWidget* container = new DAPropertyPanelContainerWidget(this);
container->addCollapsibleGroup(tr("Settings"));
container->addStringProperty(PID_Name, tr("Name"));
container->endGroup();
connect(container, &DAPropertyPanelContainerWidget::propertyValueChanged, ...);
```

**Reference**: `src/DACommonWidgets/DAPropertyPanelContainerWidget.h`

## Scenario C: App Setting Page

**Inheritance**: `DAAbstractSettingPage` → `QWidget`

**Key pattern**:
- `apply()` is **pure virtual** — called on Apply/OK click
- Emit `settingChanged()` on any property change (otherwise `apply()` is skipped)
- Emit `settingApplyed()` after apply completes

**Skeleton**:
```cpp
class MyAppSettings : public DAAbstractSettingPage {
    Q_OBJECT
public:
    explicit MyAppSettings(QWidget* parent = nullptr);
    void apply() override;
    QString getSettingPageTitle() const override;
    QIcon getSettingPageIcon() const override;
};
```

**Reference**: `src/DACommonWidgets/DAAbstractSettingPage.h`

## Key Differences

| Aspect | ChartItem (A) | Standalone (B) | App Page (C) |
|--------|--------------|----------------|--------------|
| **Inherits** | DAChartItemSettingPanel | QWidget | DAAbstractSettingPage |
| **Target** | QwtPlotItem via `setPlotItem()` | Any object via `setTarget()` | N/A |
| **buildPropertyPanel()** | Pure virtual, called by subclass | Protected slot, called directly | N/A |
| **Signal chain** | Automatic via base | Manual: 2 connections required | `settingChanged()` signal |
| **Sub-panel signals** | Auto bubble-up to parent | Auto bubble-up to parent | N/A |
| **Apply style** | Immediate (write + replot) | Immediate (write + notify) | Deferred (`apply()` on OK) |
| **Qwt extras** | addCurveStyle/Axis/SymbolProperty | Not available | N/A |
| **Factory** | DAChartItemSettingPanelFactory | Manually created | DASettingWidget manages |
| **Grouping** | addCollapsibleGroup + endGroup | addCollapsibleGroup + endGroup | N/A |

## DAPropertyPanelWidget API Quick Reference

### Property Adding & Value Access

| Method | Purpose | Read / Write |
|--------|---------|-------------|
| `addColorProperty` / `addFontProperty` / `addBrushProperty` | Color/Font/Brush | `getColorValue`/`setColorValue` etc. |
| `addPenProperty` | Pen | `getPenValue`/`setPenValue` |
| `addIntProperty` / `addDoubleProperty` / `addBoolProperty` | Numeric/Bool | `getIntValue`/`setIntValue` etc. |
| `addStringProperty` / `addEnumProperty` | String/Enum | `getStringValue`/`setStringValue` etc. |
| `addAlignmentProperty` / `addFilePathProperty` | Alignment/Path | `getAlignmentValue`/`setAlignmentValue` etc. |
| `addProperty` | Custom Widget | `getPropertyItem(id)` |

### Grouping & Nesting

| Method | Purpose | Notes |
|--------|---------|-------|
| `addCollapsibleGroup(title)` | Start collapsible group | Returns group ID (1-based). Subsequent addXxxProperty auto-tracked into this group |
| `endGroup()` | Exit current group | Next addXxxProperty goes to root layout |
| `addSubPanel(id, groupName)` | Create nested sub-panel | Returns `DAPropertyPanelWidget*` with signal bubble-up |
| `getSubPanel(id)` | Get sub-panel by ID | Returns nullptr if not found |
| `getSubPanelId(subPanel)` | Reverse-lookup sub-panel ID | Returns -1 if not found |
| `getGroupPanel(groupId)` | Get group's internal panel | Returns `DAPropertyPanelWidget*` for the group |
| `isGroupExpanded(groupId)` | Check group expand state | Returns true/false |
| `setGroupExpanded(groupId, expanded)` | Set group expand state | Programmatic control of collapse/expand |
| `addGroupLabel(text)` | **Deprecated** decorative label | Use `addCollapsibleGroup` instead. No collapse functionality |

### DAPropertyPanelContainerWidget

`DAPropertyPanelContainerWidget` proxies **all** `DAPropertyPanelWidget` methods listed above. Use `rootPanel()` to access the underlying `DAPropertyPanelWidget` directly if needed.

## Compatibility Notes

**QButtonGroup Qt5/Qt6 compat**:
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(group, QOverload<int>::of(&QButtonGroup::buttonClicked), ...);
#else
    connect(group, &QButtonGroup::idClicked, ...);
#endif
```

**Qt5/Qt6 QButtonGroup addButton**:
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    group->addButton(rb, id);
#else
    group->setId(rb, id);
    group->addButton(rb);
#endif
```

## What NOT to Duplicate

The 6-function lifecycle (`setTarget`, `getTarget`, `bindTarget`, `unbindTarget`, `updateUI`, `applySetting`) is documented in `docs/zh/dev-guide/settingwidget-standard.md`. Reference that document — do not re-explain it here.