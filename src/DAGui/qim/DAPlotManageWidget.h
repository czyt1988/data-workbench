#ifndef DAPLOTMANAGEWIDGET_H
#define DAPLOTMANAGEWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include "DAFigureElementSelection.h"

namespace Ui
{
class DAPlotManageWidget;
}
// qt
class QStandardItem;
// qim
namespace QIM
{
class QImPlotNode;
}

namespace DA
{
class DAPlotOperateWidget;
class DAFigureScrollArea;
class DAFigureTreeView;

/**
 * @brief 绘图管理窗口
 */
class DAGUI_API DAPlotManageWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPlotManageWidget)
public:
    DAPlotManageWidget(QWidget* parent = nullptr);
    ~DAPlotManageWidget();
    // 设置绘图操作窗口，只有设置了绘图操作窗口，管理窗口才可以工作
    void setPlotOperateWidget(DAPlotOperateWidget* cow);
    // 设置item点击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemClicked(bool on);
    bool isSetCurrentChartOnItemClicked() const;
    // 设置item双击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemDoubleClicked(bool on);
    bool isSetCurrentChartOnItemDoubleClicked() const;
    // 通过plot获取figure
    DAFigureScrollArea* plotToFigureWidget(QIM::QImPlotNode* plot) const;
    // 获取当前的figure
    DAFigureScrollArea* getCurrentFigure() const;
public Q_SLOTS:
    // 把管理树展开
    void expandCurrentTree();
    // 把管理树收起
    void collapseCurrentTree();
Q_SIGNALS:
    /**
     * @brief 绘图元素选中信号
     * @param selection 选中的内容
     * @sa DAFigureElementSelection
     */
    void figureElementClicked(const DA::DAFigureElementSelection& selection);
    void figureElementDbClicked(const DA::DAFigureElementSelection& selection);
    /**
     * @brief 请求绘图的设置
     * @param fig
     */
    void requestFigureSetting(DA::DAFigureScrollArea* fig);

    /**
     * @brief 当前选中的绘图发生了改变
     * @param fig
     */
    void selectFigureChanged(DA::DAFigureScrollArea* fig);

protected:
    // 获取当前的tree
    DAFigureTreeView* currentTreeView() const;

private:
    // 设置当前显示的fig对应的view
    void setCurrentDisplayView(DA::DAFigureScrollArea* fig);
    DAFigureScrollArea* getComboboxFigure(int index) const;
    void setStackCurrentFigure(DA::DAFigureScrollArea* fig);
    void setComboboxCurrentFigure(DA::DAFigureScrollArea* fig);
private slots:
    void onFigureCreated(DA::DAFigureScrollArea* fig);
    void onFigureCloseing(DA::DAFigureScrollArea* fig);
    void onCurrentFigureChanged(DA::DAFigureScrollArea* fig, int index);
    // 点击了plotitem，这里要把信号转发出去
    // 绘图设置窗口点击
    void onToolButtonFigureSettingClicked();
    // combobox
    void onComboboxCurrentIndexChanged(int index);

private:
    Ui::DAPlotManageWidget* ui;
};
}  // end of namespace DA
#endif  // DAPLOTMANAGEWIDGET_H
