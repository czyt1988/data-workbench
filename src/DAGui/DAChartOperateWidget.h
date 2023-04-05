#ifndef DACHARTOPERATEWIDGET_H
#define DACHARTOPERATEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"

namespace Ui
{
class DAChartOperateWidget;
}
namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartOperateWidget)
class DAFigureFactory;
/**
 * @brief 绘图操作窗口
 *
 * 管理着DAFigureWidget的操作窗口，DAFigureWidget内部管理着一个或多个DAChartWidget窗口
 *
 * 通过DAChartOperateWidget可以实现DAFigureWidget和DAChartWidget
 */
class DAGUI_API DAChartOperateWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DAChartOperateWidget)
public:
    DAChartOperateWidget(QWidget* parent = nullptr);
    ~DAChartOperateWidget();
    //安装FigureFactory，针对继承的DAFigureWidget需要安装新的工厂,DAChartOperateWidget负责工厂的销毁
    void setupFigureFactory(DAFigureFactory* factory);
    //添加一个Figure
    DAFigureWidget* createFigure();
    DAFigureWidget* createFigure(const QString& title);
    //获取当前的fig，如果没有返回nullptr
    DAFigureWidget* getCurrentFigure() const;
    //获取figure
    DAFigureWidget* getFigure(int index) const;
    //获取fig在DAChartOperateWidget的索引
    int getFigureIndex(DAFigureWidget* f);
    //获取当前的chart，如果没有返回nullptr
    DAChartWidget* getCurrentChart() const;
private slots:
    // tab窗口改变
    void onTabWidgetCurrentChanged(int index);
    // tab窗口关闭
    void onTabCloseRequested(int index);
signals:
    /**
     * @brief 绘图即将关闭
     * @param f
     */
    void figureCloseing(DA::DAFigureWidget* f);
    /**
     * @brief 创建了一个绘图
     * @param f
     */
    void figureCreated(DA::DAFigureWidget* f);

    /**
     * @brief 当前的figure改变了
     * @param f
     * @param index
     */
    void currentFigureChanged(DA::DAFigureWidget* f, int index);
    /**
     * @brief 添加了chart
     * @param chart指针
     */
    void chartAdded(DA::DAChartWidget* c);

    /**
     * @brief 绘图即将移除
     * @param plot 即将移除的绘图，此时指针还有效
     */
    void chartWillRemove(DA::DAChartWidget* c);

    //当前选中的发生改变
    /**
     * @brief 当前的绘图发生了变更
     *
     * 当前窗口是figure的默认窗口，任何对figure的动作会作用于当前绘图
     * @param w
     */
    void currentChartChanged(DA::DAChartWidget* c);

private:
    Ui::DAChartOperateWidget* ui;
};
}  // end of namespace DA
#endif  // DACHARTOPERATEWIDGET_H
