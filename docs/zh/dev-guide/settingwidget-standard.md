# 设置类窗口规范

设置类窗口规范定义了 DAWorkBench 中所有配置窗口的标准接口和生命周期管理方式，通过 QPointer 避免野指针问题，支持多目标复用。

## 主要功能特性

**特性**

- ✅ **统一生命周期管理**：通过 `QPointer` 管理目标对象，避免野指针问题
- ✅ **多目标复用支持**：同一面板可切换管理不同目标对象
- ✅ **信号槽绑定机制**：提供 `bindTarget`/`unbindTarget` 方法管理信号连接
- ✅ **标准化接口**：6 个标准函数覆盖完整生命周期

## 适用场景

设置类窗口规范适用于任何需要对单一目标对象进行可视化配置/修改的 QWidget 派生窗口。

**设计目标**

- 统一生命周期管理
- 杜绝野指针与重复绑定
- 支持"多目标复用同一面板"场景
- 降低不同设置窗体的函数学习成本

## 设置类窗体的函数约定

一个设置类窗体应该实现如下 6 种函数：

| 步骤 | 函数签名 | 函数权限 | 职责 |
|-----|---------|---------|------|
| 1. 设定目标 | `void setTarget(Target* t)` | public | 设定管理对象（使用 `QPointer`），更换目标时**先解绑旧目标，再绑定新目标**，最后调用 `updateUI()` 刷新显示 |
| 2. 获取目标 | `Target* getTarget() const` | public | 返回当前目标，空则返回 `nullptr` |
| 3. 绑定 | `void bindTarget()` | protected | 【非必须】建立**目标 → 窗体**与**窗体 → 目标**的所有信号槽连接 |
| 4. 解绑 | `void unbindTarget()` | protected | 【非必须】断开上述全部连接 |
| 5. 刷新显示 | `void updateUI()` | public | 把目标属性**只读**地同步到界面控件 |
| 6. 应用设置 | `void applySetting(Target* t)` | public | 把界面控件的值**写回**目标并立即生效 |

## 设置目标的生命周期管理

设置的目标应该使用 `QPointer` 管理，避免对象删除后导致显示异常。以下代码展示了如何在类中声明 QPointer 成员：

```cpp
// header file
#include <QPointer>

class MySettingWidget : public QWidget
{
private:
    // 使用 QPointer 管理目标，对象删除时自动变为 nullptr
    QPointer<Target> m_target;
};
```

执行上述代码后，当目标对象被删除时，`m_target` 自动变为 `nullptr`，避免野指针访问导致的崩溃。

`setTarget` 应该支持传入 `nullptr`，表示清空目标没有管理的设置对象。

## 窗体复用实现

为了多个对象复用一个设置窗体，`setTarget` 时应该先解绑旧目标，再绑定新目标。以下代码展示了完整的 `setTarget` 实现流程：

```cpp
// header: QPointer<Target> m_target;

void MySettingWidget::setTarget(Target* opt)
{
    // 1. 避免重复设置，提高效率
    if (m_target == opt) {
        return;
    }
    
    // 2. 旧目标信号槽断开连接，避免内存泄漏
    if (m_target) {
        unbindTarget();
    }
    
    // 3. 设置新目标指针
    m_target = opt;
    
    // 4. 绑定新目标信号槽，建立双向通信
    if (opt) {
        bindTarget();
    }
    
    // 5. 更新界面，显示新目标的属性
    updateUI();
}
```

执行上述代码后，设置窗口成功切换到新目标，旧目标的信号槽被正确解绑，界面显示新目标的属性值。

### 绑定与解绑实现示例

以下代码展示 `bindTarget` 和 `unbindTarget` 的实现，建立目标与窗体的双向信号连接：

```cpp
void MySettingWidget::bindTarget()
{
    if (!m_target) {
        return;
    }
    
    // 建立目标 → 窗体的信号连接
    // 当目标属性变化时，窗体自动更新显示
    connect(m_target, &Target::propertyChanged,
            this, &MySettingWidget::onTargetPropertyChanged);
    
    // 建立窗体 → 目标的信号连接（如需要）
    // 当窗体设置变化时，目标自动应用
    // connect(this, &MySettingWidget::settingChanged,
    //         m_target, &Target::applySetting);
}

void MySettingWidget::unbindTarget()
{
    if (!m_target) {
        return;
    }
    
    // 断开所有连接，防止信号继续触发
    disconnect(m_target, nullptr, this, nullptr);
}
```

执行上述代码后，目标与窗体建立信号连接，属性变化自动同步，解绑时所有连接被正确断开。

## 刷新显示与应用设置

### updateUI 实现

以下代码展示 `updateUI` 的实现，从目标读取属性并更新界面控件：

```cpp
void MySettingWidget::updateUI()
{
    if (!m_target) {
        // 清空界面或显示默认状态
        clearUI();
        return;
    }
    
    // 从目标读取属性，更新界面控件（只读操作）
    m_nameEdit->setText(m_target->getName());
    m_valueSlider->setValue(m_target->getValue());
    m_colorCombo->setCurrentColor(m_target->getColor());
}
```

执行上述代码后，界面控件显示目标当前的属性值，此操作是只读的，不修改目标状态。

### applySetting 实现

以下代码展示 `applySetting` 的实现，将界面控件的值写回目标：

```cpp
void MySettingWidget::applySetting(Target* t)
{
    // 如果传入目标，使用传入的；否则使用当前管理的目标
    Target* target = t ? t : m_target;
    if (!target) {
        return;
    }
    
    // 将界面控件的值写回目标（修改操作）
    target->setName(m_nameEdit->text());
    target->setValue(m_valueSlider->value());
    target->setColor(m_colorCombo->currentColor());
    
    // 触发目标更新通知
    target->notifyChanged();
}
```

执行上述代码后，界面控件的值被写入目标对象，目标状态被修改。

## 完整示例

```cpp
// MyChartSettingWidget.h
#ifndef MYCHARTSETTINGWIDGET_H
#define MYCHARTSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "DAChart.h"

class MyChartSettingWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit MyChartSettingWidget(QWidget* parent = nullptr);
    
    // 标准接口
    void setTarget(DAChart* chart);
    DAChart* getTarget() const;
    void updateUI();
    void applySetting(DAChart* chart = nullptr);
    
protected:
    void bindTarget();
    void unbindTarget();
    
private Q_SLOTS:
    void onChartPropertyChanged();
    
private:
    QPointer<DAChart> m_chart;
    
    // UI 组件
    QLineEdit* m_titleEdit;
    QSlider* m_opacitySlider;
};

#endif

// MyChartSettingWidget.cpp
#include "MyChartSettingWidget.h"

MyChartSettingWidget::MyChartSettingWidget(QWidget* parent)
    : QWidget(parent)
{
    // 初始化 UI 组件
    m_titleEdit = new QLineEdit(this);
    m_opacitySlider = new QSlider(Qt::Horizontal, this);
}

void MyChartSettingWidget::setTarget(DAChart* chart)
{
    if (m_chart == chart) {
        return;
    }
    
    if (m_chart) {
        unbindTarget();
    }
    
    m_chart = chart;
    
    if (chart) {
        bindTarget();
    }
    
    updateUI();
}

DAChart* MyChartSettingWidget::getTarget() const
{
    return m_chart;
}

void MyChartSettingWidget::bindTarget()
{
    if (!m_chart) {
        return;
    }
    
    connect(m_chart, &DAChart::titleChanged,
            this, &MyChartSettingWidget::onChartPropertyChanged);
    connect(m_chart, &DAChart::opacityChanged,
            this, &MyChartSettingWidget::onChartPropertyChanged);
}

void MyChartSettingWidget::unbindTarget()
{
    if (!m_chart) {
        return;
    }
    
    disconnect(m_chart, nullptr, this, nullptr);
}

void MyChartSettingWidget::updateUI()
{
    if (!m_chart) {
        m_titleEdit->clear();
        m_opacitySlider->setValue(100);
        return;
    }
    
    m_titleEdit->setText(m_chart->getTitle());
    m_opacitySlider->setValue(static_cast<int>(m_chart->getOpacity() * 100));
}

void MyChartSettingWidget::applySetting(DAChart* chart)
{
    DAChart* target = chart ? chart : m_chart;
    if (!target) {
        return;
    }
    
    target->setTitle(m_titleEdit->text());
    target->setOpacity(m_opacitySlider->value() / 100.0);
}

void MyChartSettingWidget::onChartPropertyChanged()
{
    updateUI();
}
```

## 注意事项

!!! warning "QPointer 的使用"
    必须使用 `QPointer` 而非裸指针管理目标对象。当目标对象被删除时，`QPointer` 会自动变为 `nullptr`，避免野指针问题。

!!! tip "信号槽连接"
    `bindTarget` 中建立的信号槽连接必须在 `unbindTarget` 中完全断开，否则可能导致内存泄漏或重复响应。

!!! note "界面刷新时机"
    `updateUI` 只负责读取目标属性显示到界面，不应修改目标状态。修改目标状态应在 `applySetting` 中进行。

## 参考资料

- [QPointer 使用指南](https://doc.qt.io/qt-5/qpointer.html)
- [Qt 信号槽最佳实践](https://doc.qt.io/qt-5/signalsandslots.html)