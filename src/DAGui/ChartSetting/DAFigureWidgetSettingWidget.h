#ifndef DAFIGUREWIDGETSETTINGWIDGET_H
#define DAFIGUREWIDGETSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
namespace Ui
{
class DAFigureWidgetSettingWidget;
}

namespace DA
{
class DAFigureWidget;
/**
 * @brief figure设置窗口
 *
 * 一个设置窗口应该由以下方法组成：
 * 1. 设定设置目标 setXXX、getXXX
 * 2. 目标的信号和此设置窗口的信号和槽的绑定函数bindXXX
 * 3. 目标的信号和此设置窗口的信号和槽的解除绑定函数unbindXXX(解除绑定是为了多个目标，能共用一个设置窗口，设置新目标时，解除旧目标的信号槽绑定关系)
 * 4. 把目标的属性更新到界面中显示：updateUI();
 * 5. 把设置界面的内容应用到目标窗口：applySetting();
 */
class DAGUI_API DAFigureWidgetSettingWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureWidgetSettingWidget)
public:
    explicit DAFigureWidgetSettingWidget(QWidget* parent = nullptr);
    ~DAFigureWidgetSettingWidget();
    // 设置figure
    void setFigure(DAFigureWidget* fig);
    // 获取当前绑定的figure
    DAFigureWidget* getFigure() const;
    // 更新界面
    void updateUI();
    // 把设置界面的内容应用到目标窗口
    void applySetting();

protected:
    void changeEvent(QEvent* e);
    // 相关信号和槽的绑定
    void bindFigure(DAFigureWidget* fig);
    void unbindFigure(DAFigureWidget* fig);

private:
    Ui::DAFigureWidgetSettingWidget* ui;
};

}
#endif  // DAFIGUREWIDGETSETTINGWIDGET_H
