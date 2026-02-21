#ifndef DAPLOTOPERATEWIDGET_H
#define DAPLOTOPERATEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureScrollArea.h"
#include "DAAbstractOperateWidget.h"
namespace Ui
{
class DAPlotOperateWidget;
}

namespace QIM
{
class QImFigureWidget;
class QImPlotNode;
}

namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartOperateWidget)
class DAFigureFactory;
/**
 * @brief 绘图操作窗口
 *
 * 管理着DAFigureScrollArea的操作窗口，DAFigureScrollArea内部管理着一个或多个绘图
 */
class DAGUI_API DAPlotOperateWidget : public DAAbstractOperateWidget
{
    Q_OBJECT
    DA_IMPL(DAChartOperateWidget)
public:
    DAPlotOperateWidget(QWidget* parent = nullptr);
    ~DAPlotOperateWidget();
    // 安装FigureFactory，针对继承的DAFigureScrollArea需要安装新的工厂,DAPlotOperateWidget负责工厂的销毁
    void setupFigureFactory(DAFigureFactory* factory);
    // 拿出之前的工厂
    DAFigureFactory* takeFactory();
    // 获取工厂
    DAFigureFactory* getFigureFactory() const;
    // 添加一个Figure
    virtual DAFigureScrollArea* createFigure(const QString& name = QString());
    // 获取所有的绘图
    QList< DAFigureScrollArea* > getFigureList() const;
    // 获取当前的fig，如果没有返回nullptr
    DAFigureScrollArea* getCurrentFigure() const;
    DAFigureScrollArea* gcf() const;
    // 设置当前的figure
    void setCurrentFigure(int index);
    // 获取figure
    DAFigureScrollArea* getFigure(int index) const;
    // 获取figure的命名
    QString getFigureName(int index) const;
    QString getFigureName(DAFigureScrollArea* f) const;
    // 设置绘图名称
    void setFigureName(int index, const QString& name);
    void setFigureName(DAFigureScrollArea* f, const QString& name);
    // 获取fig在DAChartOperateWidget的索引
    int getFigureIndex(DAFigureScrollArea* f) const;
    // 删除窗口
    void removeFigure(DAFigureScrollArea* f, bool deleteFigure = true);
    // 获取当前的chart，如果没有返回nullptr
    QIM::QImPlotNode* getCurrentPlot() const;
    QIM::QImPlotNode* gca() const;
    QList< QIM::QImPlotNode* > getCurrentCharts() const;
    QList< QIM::QImPlotNode* > gcas() const;
    // 获取绘图的数量
    int getFigureCount() const;
    // 获取QUndoStack
    QUndoStack* getUndoStack() override;
public Q_SLOTS:
    // 清除所有绘图
    void clear();

protected:
    // 初始化figure的连接，这个函数用于重载createFigure函数时创建fig后绑定槽函数到DAChartOperateWidget用
    void initFigureConnect(DAFigureScrollArea* fig);
private slots:
    // tab窗口改变
    void onTabWidgetCurrentChanged(int index);
    // tab窗口关闭
    void onTabCloseRequested(int index);
    //
    void onFigureTitleChanged(const QString& t);
Q_SIGNALS:
    /**
     * @brief 绘图即将关闭
     * @param f
     */
    void figureRemoving(DA::DAFigureScrollArea* f);
    /**
     * @brief 创建了一个绘图
     * @param f
     */
    void figureCreated(DA::DAFigureScrollArea* f);
    /**
     * @brief 绘图的标题发生了改变
     * @param f
     * @param title
     */
    void figureTitleChanged(DA::DAFigureScrollArea* f, const QString& title);

    /**
     * @brief 当前的figure改变了
     * @param f 当前激活的figure
     * @param index 当前激活的figure对应的index
     */
    void currentFigureChanged(DA::DAFigureScrollArea* f, int index);
    /**
     * @brief 添加了chart
     * @param chart指针
     */
    void plotAdded(QIM::QImPlotNode* c);

    /**
     * @brief 绘图移除
     * @param plot 移除的绘图，此时指针还有效
     */
    void plotRemoved(QIM::QImPlotNode* c);

    // 当前选中的发生改变
    /**
     * @brief 当前的绘图发生了变更
     *
     * 当前窗口是figure的默认窗口，任何对figure的动作会作用于当前绘图
     * @param w
     */
    void currentPlotChanged(QIM::QImPlotNode* c);

private:
    Ui::DAPlotOperateWidget* ui;
};
}  // end of namespace DA
#endif  // DAPLOTOPERATEWIDGET_H
