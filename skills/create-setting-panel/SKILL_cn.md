---
name: create-setting-panel-cn
description: 使用当需要在data-workbench Qt/C++项目中创建属性设置面板时。症状/触发条件：为新的Qwt图表项（QwtPlotItem子类）添加设置面板、为坐标轴/图表全局级别创建属性面板、扩展DAChartItemSettingPanelFactory注册自定义面板、在DAAbstractSettingPage中创建应用级配置页面。
---

# Create Setting Panel (Chinese)

## 概述

本技能指导AI Agent在 data-workbench 项目中创建属性设置面板。框架基于 `DAPropertyPanelWidget` 提供统一的属性编辑能力，采用**即时应用模式**——属性变化直接写回目标对象 + `replot()`，无需单独的 `apply()` 步骤。

## 决策树：哪个类别？

```
需要创建设置面板？
  │
  ├─ 目标是否为 QwtPlotItem 子类（曲线、柱状图、图例等）？
  │   └─ YES → 类别1：ChartItem 面板（继承 DAChartItemSettingPanel）
  │
  ├─ 目标是否为坐标轴/图表全局级别？
  │   └─ YES → 类别2：非Item级面板（继承 QWidget + 组合 DAPropertyPanelWidget）
  │
  └─ 是否为应用级设置（如DASettingWidget侧边栏项）？
      └─ YES → 类别3：应用级设置页面（继承 DAAbstractSettingPage）
```

## 类别1：ChartItem 设置面板

### 继承关系

```
DAChartCurveSettingPanel → DAChartItemSettingPanel → DAAbstractChartItemSettingWidget
```

### 关键约定

- `buildPropertyPanel()` 是**纯虚函数**，子类构造函数末尾**自行调用**
- 使用 `DAChartItemSettingPanelFactory` 注册：`instance().registerPanel(RTTI, []{ return new MyPanel(); })`
- 已有7个内置注册：`Rtti_PlotCurve`, `Rtti_PlotBarChart`, `Rtti_PlotIntervalCurve`, `Rtti_PlotSpectrogram`, `Rtti_PlotTradingCurve`, `Rtti_PlotGrid`, `Rtti_PlotLegend`

### 最小骨架（.h）

```cpp
class MyCurveSettingPanel : public DAChartItemSettingPanel
{
    Q_OBJECT
public:
    enum PropertyID { PID_Title = 1, PID_Pen = 2, PID_EnableMarker = 3 };
    explicit MyCurveSettingPanel(QWidget* parent = nullptr);
    void updateUI(QwtPlotItem* item) override;
private:
    void buildPropertyPanel() override;
private Q_SLOTS:
    void onMyPropertyChanged(int propertyId);
};
```

### 构造函数

```cpp
MyCurveSettingPanel::MyCurveSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &MyCurveSettingPanel::onMyPropertyChanged);
    buildPropertyPanel();   // 纯虚函数，子类末尾自行调用
}
```

### buildPropertyPanel + onPropertyChanged 合一示例

```cpp
void MyCurveSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelWidget* pp = propertyPanel();
    pp->addGroupLabel(tr("General"));
    pp->addStringProperty(PID_Title, tr("Title"));
    pp->addPenProperty(PID_Pen, tr("Pen"));
}

void MyCurveSettingPanel::onMyPropertyChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotCurve)) return;
    QwtPlotCurve* curve = d_cast<QwtPlotCurve*>();
    if (!curve) return;
    DAPropertyPanelWidget* pp = propertyPanel();
    switch (propertyId) {
    case PID_Title: curve->setTitle(pp->getStringValue(PID_Title)); break;
    case PID_Pen:   curve->setPen(pp->getPenValue(PID_Pen)); break;
    default: break;
    }
    replot();  // 即时应用：写回后立即重绘
}
```

### updateUI（使用QSignalBlocker）

```cpp
void MyCurveSettingPanel::updateUI(QwtPlotItem* item)
{
    if (!item || !checkItemRTTI(QwtPlotItem::Rtti_PlotCurve)) {
        DAChartItemSettingPanel::updateUI(item); return;
    }
    QwtPlotCurve* curve = d_cast<QwtPlotCurve*>();
    DAChartItemSettingPanel::updateUI(item);
    DAPropertyPanelWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);
    pp->setStringValue(PID_Title, curve->title().text());
    pp->setPenValue(PID_Pen, curve->pen());
}
```

## 类别2：非Item级设置面板

### 继承关系

```
DAChartAxisSettingPanel → QWidget（组合 DAPropertyPanelWidget）
```

### 关键约定（与类别1不同）

| 差异点 | ChartItem（类别1） | 非Item（类别2） |
|--------|-------------------|-----------------|
| `buildPropertyPanel()` | 纯虚函数，子类构造函数末尾调用 | `protected slot`，构造函数中**直接调用** |
| 信号链 | `mPanel → onPanelPropertyValueChanged → emit propertyValueChanged → 子类槽` | **3跳**：`mPanel → onPanelPropertyValueChanged → emit propertyValueChanged → onPropertyValueChanged` |
| 构造函数参数 | 仅 `QWidget* parent` | 需要额外业务参数（如 `QwtAxis::Position axisId`） |

### 构造函数：3跳信号链 + QButtonGroup

```cpp
DAChartAxisSettingPanel::DAChartAxisSettingPanel(QwtAxis::Position axisId, QWidget* parent)
    : QWidget(parent), mAxisId(axisId)
{
    mPanel = new DAPropertyPanelWidget(this);
    auto* layout = new QVBoxLayout(this); layout->addWidget(mPanel); setLayout(layout);
    // 第1跳：面板 → 转发槽    第2→3跳：本类信号 → 处理槽（必须连）
    connect(mPanel, &DAPropertyPanelWidget::propertyValueChanged, this, &DAChartAxisSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChartAxisSettingPanel::propertyValueChanged, this, &DAChartAxisSettingPanel::onPropertyValueChanged);
    buildPropertyPanel();  // protected slot，构造函数中直接调用
}

// 第2跳：转发
void DAChartAxisSettingPanel::onPanelPropertyValueChanged(int propertyId) { emit propertyValueChanged(propertyId); }
// 第3跳：处理
void DAChartAxisSettingPanel::onPropertyValueChanged(int propertyId) { /* switch/case + replot(); */ }
```

**QButtonGroup 兼容写法**（`clicked`传buttonId而非propertyId，lambda捕获propertyId）：

```cpp
mScaleStyleButtonGroup = new QButtonGroup(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mScaleStyleButtonGroup->addButton(rbNormal, NormalScale);
    mScaleStyleButtonGroup->addButton(rbDateTime, DateTimeScale);
#else
    mScaleStyleButtonGroup->setId(rbNormal, NormalScale); mScaleStyleButtonGroup->addButton(rbNormal);
    mScaleStyleButtonGroup->setId(rbDateTime, DateTimeScale); mScaleStyleButtonGroup->addButton(rbDateTime);
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(mScaleStyleButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int) { onPanelPropertyValueChanged(PID_ScaleStyle); });
#else
    connect(mScaleStyleButtonGroup, &QButtonGroup::idClicked, this, [this](int) { onPanelPropertyValueChanged(PID_ScaleStyle); });
#endif
panel->addProperty(PID_ScaleStyle, tr("Scale Style"), container);
```

## 类别3：应用级设置页面

继承 `DAAbstractSettingPage`，必须实现：`apply()`（用户点应用/确定调用）、`getSettingPageTitle()`（侧边栏标题）、`getSettingPageIcon()`（侧边栏图标）。

**关键约定**：属性变化时**必须发射** `settingChanged()` 信号，否则顶层设置管理窗口不会调用 `apply()`。`apply()` 成功后发射 `settingApplyed()`。

```cpp
class MySettingPage : public DAAbstractSettingPage {
    Q_OBJECT
public:
    void apply() override;
    QString getSettingPageTitle() const override;
    QIcon getSettingPageIcon() const override;
protected:
    void onValueChanged() { emit settingChanged(); }
};
```

## 关键差异汇总

### DAPropertyPanelWidget API速查

| 方法 | 用途 | 方法 | 用途 |
|------|------|------|------|
| `addColorProperty` | 颜色 | `getColorValue/setColorValue` | 读写 |
| `addFontProperty` | 字体 | `getFontValue/setFontValue` | 读写 |
| `addBrushProperty` | 画刷 | `getBrushValue/setBrushValue` | 读写 |
| `addPenProperty` | 画笔 | `getPenValue/setPenValue` | 读写 |
| `addIntProperty` | 整数 | `getIntValue/setIntValue` | 读写 |
| `addDoubleProperty` | 浮点 | `getDoubleValue/setDoubleValue` | 读写 |
| `addBoolProperty` | 布尔 | `getBoolValue/setBoolValue` | 读写 |
| `addStringProperty` | 字符串 | `getStringValue/setStringValue` | 读写 |
| `addEnumProperty` | 枚举 | `getEnumValue/setEnumValue` | 读写 |
| `addAlignmentProperty` | 对齐 | `getAlignmentValue/setAlignmentValue` | 读写 |
| `addFilePathProperty` | 路径 | `getFilePathValue/setFilePathValue` | 读写 |
| `addGroupLabel` | 分组 | `addProperty` | 自定义Widget |

### PropertyId + 即时应用

- PropertyId从1开始，用于`addProperty()`和`onPropertyValueChanged()`的switch分派，面板内不重复
- 即时应用模式：`propertyValueChanged` → 读取新值 → 写回目标 → `replot()`，不需单独`apply()`（类别3除外）

## 参考文件

| 文件 | 用途 |
|------|------|
| `src/DAGui/ChartSetting/DAChartItemSettingPanel.h` | ChartItem 基类：纯虚 buildPropertyPanel、Qwt 专有方法 |
| `src/DAGui/ChartSetting/DAChartCurveSettingPanel.h/cpp` | ChartItem 完整示例：PropertyId、构造函数、updateUI switch |
| `src/DAGui/ChartSetting/DAChartAxisSettingPanel.h/cpp` | 非Item 面板完整示例：3跳信号链、QButtonGroup 兼容写法 |
| `src/DAGui/ChartSetting/DAChartItemSettingPanelFactory.h/cpp` | 注册工厂：registerAllKnownPanels()、7个内置注册 |
| `src/DACommonWidgets/DAPropertyPanelWidget.h` | 全部 addXxxProperty + getXxxValue/setXxxValue 方法 |
| `src/DACommonWidgets/DAAbstractSettingPage.h` | 应用级页面基类：apply、settingChanged、settingApplyed |
| `src/DAGui/ChartSetting/DAAbstractChartItemSettingWidget.h` | ReturnWhenItemNull 宏、d_cast/s_cast、replot |
