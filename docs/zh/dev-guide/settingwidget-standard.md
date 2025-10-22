# 设置类窗口规范

设置类窗口规范主要是为了规范化本程序所有设置了窗口

适用场景：任何需要对单一目标对象进行可视化配置/修改的 QWidget 派生窗口。
设计目标：
- 统一生命周期管理
- 杜绝野指针与重复绑定
- 支持“多目标复用同一面板”场景
- 降低不同设置了窗体的函数学习成本

## 设置类窗体的函数约定

一个设置类窗体应该实现如下6种函数

| 步骤      | 函数签名                        |函数权限| 职责                                |
| ------- | --------------------------- |-------| --------------------------------- |
| 1. 设定目标 | `void setTarget(Target* t)` |public| 设定管理对象（使用`QPointer`），更换目标时**先解绑（unbindTarget）旧目标，再绑定（bindTarget）新目标**，最后会调用 `updateUI()` 刷新显示            |
| 2. 获取目标 | `Target* getTarget() const`  |public| 返回当前目标，空则返回 `nullptr`             |
| 3. 绑定   | `void bindTarget()`         |protected| 【非必须】建立**目标 → 窗体**与**窗体 → 目标**的所有信号槽连接 |
| 4. 解绑   | `void unbindTarget()`       |protected| 【非必须】断开上述全部连接            |
| 5. 刷新显示 | `void updateUI()`           |public| 把目标属性**只读**地同步到界面控件               |
| 6. 应用设置 | `void applySetting(Target* t)`       |public| 把界面控件的值**写回**目标并立即生效              |

## 设置目标的生命周期管理

设置的目标应该使用`QPointer`管理，避免对象删除了导致显示异常

`setTarget`应该支持传入`nullptr`，表示清空目标没有管理的设置对象

## 窗体复用

为例多个对象复用一个设置窗体，`setTarget`时，应该先解绑旧目标，再绑定新目标，具体类似如下代码：

```cpp
//header : QPointer<Target> mChartOpt;

void MySettingWidget::setTarget(Target* opt)
{
    //! 1.避免重复设置
    if (mChartOpt == opt) {
        return;
    }
    //! 2.旧目标信号槽断开连接
    if (mChartOpt) {
        unbindTarget();
    }
    //! 3.幅值新目标
    mChartOpt = opt;
    //! 4.绑定新目标信号槽
    if (opt) {
        bindTarget();
    }
    //! 5.更新界面
    updateUI();
}
```