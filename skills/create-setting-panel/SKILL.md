---
name: create-setting-panel
description: Use when creating setting panels, property panels, or configuration UI in the data-workbench Qt/C++ project. Triggers include "add a setting panel", "create property panel", "chart item settings", "axis settings", or "app settings page".
---

# Create Setting Panel

## Overview

Three categories of setting panels exist in data-workbench. Identify which category before implementing. All use `DAPropertyPanelWidget` for property UI and follow an immediate-apply pattern (write to target + `replot()` on each change).

## Decision Tree

```
Do you need to configure a QwtPlotItem (curve, bar, grid, legend)?
  YES → Category 1: ChartItemSettingPanel (inherit DAChartItemSettingPanel)
  NO → Is the target a QwtScaleWidget/axis (NOT a QwtPlotItem)?
    YES → Category 2: Non-Item Panel (inherit QWidget directly)
    NO → Is this an app-level configuration (preferences, options)?
      YES → Category 3: App Setting Page (inherit DAAbstractSettingPage)
```

## Category 1: ChartItemSettingPanel

**Target**: QwtPlotItem subclasses (QwtPlotCurve, QwtPlotBarChart, etc.)

**Inheritance**: `DAChartItemSettingPanel` → `DAAbstractChartItemSettingWidget` → `QWidget`

**Key conventions**:
- `buildPropertyPanel()` is **pure virtual**. Subclass calls it at constructor end.
- Define `PropertyID` enum starting from 1.
- `updateUI(QwtPlotItem*)` reads item state into panel (use `QSignalBlocker`).
- `onXxxPropertyValueChanged(int)` writes changes back + calls `replot()`.
- Register via `DAChartItemSettingPanelFactory::registerPanel(rtti, creator)`.

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

// Constructor: buildPropertyPanel() called LAST
MyPanel::MyPanel(QWidget* parent) : DAChartItemSettingPanel(parent) {
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &MyPanel::onPropertyValueChanged);
    buildPropertyPanel();
}

// Signal chain is automatic: propertyPanel → onPanelPropertyValueChanged → propertyValueChanged
```

**Reference files**:
- `src/DAGui/ChartSetting/DAChartItemSettingPanel.h` — base class, pure virtual + Qwt methods
- `src/DAGui/ChartSetting/DAChartCurveSettingPanel.{h,cpp}` — complete implementation example
- `src/DAGui/ChartSetting/DAChartItemSettingPanelFactory.{h,cpp}` — singleton factory with 7 built-in registrations

## Category 2: Non-Item Panel

**Target**: QwtPlot-level objects (axes, scales) — NOT QwtPlotItem subclasses.

**Inheritance**: `QWidget` directly (NOT DAAbstractChartItemSettingWidget)

**Key conventions**:
- `buildPropertyPanel()` is a **protected slot**, called directly in constructor (NOT pure virtual).
- Constructor requires an identifier parameter (e.g., `QwtAxis::Position axisId`).
- **Signal chain MUST be manually wired** in constructor:
  1. `mPanel->propertyValueChanged → this->onPanelPropertyValueChanged(int)`
  2. `this->propertyValueChanged(int) → this->onPropertyValueChanged(int)`
- Uses `QPointer<QwtPlot>` for target management.
- Call `replot()` on `mPlot` after each property change.

**Skeleton**:
```cpp
class MyAxisPanel : public QWidget {
    Q_OBJECT
public:
    enum PropertyId { PID_Enable = 1, PID_Label = 2 };
    explicit MyAxisPanel(QwtAxis::Position axisId, QWidget* parent = nullptr);
protected Q_SLOTS:
    void buildPropertyPanel();
    void onPanelPropertyValueChanged(int propertyId);
    void onPropertyValueChanged(int propertyId);
Q_SIGNALS:
    void propertyValueChanged(int propertyId);
};

// Constructor: connect BOTH links in signal chain
MyAxisPanel::MyAxisPanel(QwtAxis::Position axisId, QWidget* parent)
    : QWidget(parent), mAxisId(axisId) {
    mPanel = new DAPropertyPanelWidget(this);
    connect(mPanel, &DAPropertyPanelWidget::propertyValueChanged,
            this, &MyAxisPanel::onPanelPropertyValueChanged);
    connect(this, &MyAxisPanel::propertyValueChanged,
            this, &MyAxisPanel::onPropertyValueChanged);
    buildPropertyPanel();  // called directly, NOT pure virtual
}
```

**Reference files**:
- `src/DAGui/ChartSetting/DAChartAxisSettingPanel.{h,cpp}` — complete implementation example
- `src/DACommonWidgets/DAPropertyPanelWidget.h` — all `addXxxProperty` + `getXxxValue`/`setXxxValue` methods

## Category 3: App Setting Page

**Target**: Application-level configuration (preferences, options, file paths).

**Inheritance**: `DAAbstractSettingPage` → `QWidget`

**Key conventions**:
- `apply()` is **pure virtual** — called when user clicks Apply/OK.
- `getSettingPageTitle()` and `getSettingPageIcon()` define sidebar display.
- Emit `settingChanged()` when any property changes, otherwise `apply()` is never called.
- Emit `settingApplyed()` after apply completes.

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

**Reference files**:
- `src/DACommonWidgets/DAAbstractSettingPage.h` — base class interface

## Key Differences

| Aspect | ChartItemSettingPanel | Non-Item Panel | App Setting Page |
|--------|----------------------|----------------|------------------|
| **Inherits** | DAChartItemSettingPanel | QWidget | DAAbstractSettingPage |
| **buildPropertyPanel()** | Pure virtual, called by subclass | Protected slot, called by base | N/A |
| **Signal chain** | Automatic via base class | Manual: 2 connections required | `settingChanged()` signal |
| **Apply style** | Immediate (write + replot) | Immediate (write + replot) | Deferred (`apply()` on OK) |
| **Target** | QwtPlotItem via `setPlotItem()` | QwtPlot + axisId via `setTarget()` | N/A |
| **Factory** | DAChartItemSettingPanelFactory | Manually created | DASettingWidget manages |

## Common Utilities

**DAPropertyPanelWidget methods** (available to all categories):
- `addXxxProperty(int id, name)` — addBoolProperty, addStringProperty, addDoubleProperty, addIntProperty, addColorProperty, addFontProperty, addPenProperty, addBrushProperty, addEnumProperty, addAlignmentProperty, addFilePathProperty
- `getXxxValue(id)` / `setXxxValue(id, value)` — matching getters/setters per type
- `setPropertyEnabled(id, bool)`, `setPropertyVisible(id, bool)`
- `addGroupLabel(text)` — section headers

**PropertyId convention**: Enum values start from 1. Used in `addProperty()` calls and `onPropertyValueChanged()` switch dispatch.

**Immediate-apply pattern**: Property changes write directly to target object + call `replot()`. No separate `apply()` step needed for Categories 1 and 2.

**QButtonGroup Qt5/Qt6 compat**:
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(group, QOverload<int>::of(&QButtonGroup::buttonClicked), ...);
#else
    connect(group, &QButtonGroup::idClicked, ...);
#endif
```

## What NOT to Duplicate

The 6-function lifecycle (`setTarget`, `getTarget`, `bindTarget`, `unbindTarget`, `updateUI`, `applySetting`) is documented in `docs/zh/dev-guide/settingwidget-standard.md`. Reference that document for general setting widget conventions — do not re-explain it here.
