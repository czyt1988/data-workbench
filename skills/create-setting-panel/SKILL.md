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
4. **Connect signal chain** — `mPanel->propertyValueChanged` to handler slot
5. **Dispatch in handler** — `switch(propertyId)` with `case PID_*:` blocks
6. **Read values** — use `getxxxValue(PID_*)` in handler
7. **Write back** — apply to target + call target's refresh mechanism

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
    enum PropertyID { PID_Title = 1, PID_Color = 2 };
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
```

**Note**: Non-chart targets do not call `replot()`. Emit the target object's own change signal or call its refresh method instead.

**References**: `src/DAGui/ChartSetting/DAChartAxisSettingPanel.{h,cpp}` (composition pattern), `src/DAGui/DAWorkFlowNodeItemSettingWidget.h` (target domain)

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
| **Apply style** | Immediate (write + replot) | Immediate (write + notify) | Deferred (`apply()` on OK) |
| **Qwt extras** | addCurveStyle/Axis/SymbolProperty | Not available | N/A |
| **Factory** | DAChartItemSettingPanelFactory | Manually created | DASettingWidget manages |

## Compatibility Notes

**DAPropertyPanelWidget API** (all scenarios):
- `addXxxProperty` — addBool/String/Double/Int/Color/Font/Pen/Brush/Enum/Alignment/FilePathProperty
- `getXxxValue(id)` / `setXxxValue(id, value)` — matching getters/setters
- `setPropertyEnabled/Visible`, `addGroupLabel`

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
