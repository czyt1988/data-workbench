#ifndef DACHARTMANAGEWIDGET_H
#define DACHARTMANAGEWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
namespace Ui
{
class DAChartManageWidget;
}
namespace DA
{
class DAChartOperateWidget;
class DAFigureWidget;
DA_IMPL_FORWARD_DECL(DAChartManageWidget)
/**
 * @brief 绘图管理窗口
 */
class DAGUI_API DAChartManageWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DAChartManageWidget)
public:
    DAChartManageWidget(QWidget* parent = nullptr);
    ~DAChartManageWidget();
    //设置绘图炒作窗口，只有设置了绘图操作窗口，管理窗口才可以工作
    void setChartOperateWidget(DAChartOperateWidget* cow);
private slots:
    void onFigureCreated(DA::DAFigureWidget* fig);
    void onFigureCloseing(DA::DAFigureWidget* fig);
    void onCurrentFigureChanged(DA::DAFigureWidget* fig, int index);

private:
    Ui::DAChartManageWidget* ui;
};
}  // end of namespace DA
#endif  // DACHARTMANAGEWIDGET_H
