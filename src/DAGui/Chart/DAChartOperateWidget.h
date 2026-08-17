#ifndef DACHARTOPERATEWIDGET_H
#define DACHARTOPERATEWIDGET_H
#include <QWidget>
#include <QByteArray>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"
#include "DAAbstractOperateWidget.h"
namespace ads
{
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}
namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartOperateWidget)
class DAFigureFactory;
class DAFigureDockWidget;
/**
 * @brief 绘图操作窗口
 *
 * 管理着DAFigureWidget的操作窗口，DAFigureWidget内部管理着一个或多个DAChartWidget窗口
 *
 * 本窗口基于 Qt-Advanced-Docking-System (ADS) 的 dockindock 嵌套模式实现：
 * 内部持有一个嵌套的 ads::CDockManager，每个 DAFigureWidget 包成 DAFigureDockWidget
 * （纯 QWidget）后由 ads::CDockWidget 包装加入停靠区。figure 的 dock widget 无法
 * 逃逸到 app 顶层管理器，满足"只能在 DAChartOperateWidget 里布局"。默认以标签形式
 * 加入当前聚焦 dock area，用户可自由拖拽分屏/并栏（禁浮动，保留 movable）。
 *
 * 通过DAChartOperateWidget可以实现DAFigureWidget和DAChartWidget
 */
class DAGUI_API DAChartOperateWidget : public DAAbstractOperateWidget
{
    Q_OBJECT
    DA_IMPL(DAChartOperateWidget)
public:
    DAChartOperateWidget(QWidget* parent = nullptr);
    ~DAChartOperateWidget();
    // 安装FigureFactory，针对继承的DAFigureWidget需要安装新的工厂,DAChartOperateWidget负责工厂的销毁
    void setupFigureFactory(DAFigureFactory* factory);
    // 拿出之前的工厂
    DAFigureFactory* takeFactory();
    // 获取工厂
    DAFigureFactory* getFigureFactory() const;
    // 添加一个Figure，id 非空时用作 figure 持久 id 与 dock objectName（供工程反序列化恢复布局）
    virtual DAFigureWidget* createFigure(const QString& name = QString(), const QString& id = QString());
    // 获取所有的绘图
    QList< DAFigureWidget* > getFigureList() const;
    // 获取当前的fig，如果没有返回nullptr
    DAFigureWidget* getCurrentFigure() const;
    DAFigureWidget* gcf() const;
    // 设置当前的figure
    void setCurrentFigure(int index);
    void setCurrentFigure(DAFigureWidget* fig);
    // 获取figure
    DAFigureWidget* getFigure(int index) const;
    DAFigureWidget* findFigure(const QString& id) const;
    // 获取figure的命名
    QString getFigureName(int index) const;
    QString getFigureName(DAFigureWidget* f) const;
    // 设置绘图名称
    void setFigureName(int index, const QString& name);
    void setFigureName(DAFigureWidget* f, const QString& name);
    // 获取fig在DAChartOperateWidget的索引
    int getFigureIndex(DAFigureWidget* f) const;
    // 删除窗口
    void removeFigure(DAFigureWidget* f, bool deleteFigure = true);
    // 获取当前的chart，如果没有返回nullptr
    DAChartWidget* getCurrentChart() const;
    DAChartWidget* gca() const;
    QList< DAChartWidget* > getAllCharts() const;
    QList< DAChartWidget* > gcas() const;
    // 获取绘图的数量
    int getFigureCount() const;
    // 获取QUndoStack
    QUndoStack* getUndoStack() override;
    // 保存嵌套停靠区布局（供工程序列化），无停靠区时返回空
    QByteArray saveChartLayout() const;
    // 恢复嵌套停靠区布局（供工程反序列化），state 为空或失败返回 false
    bool restoreChartLayout(const QByteArray& state);
public Q_SLOTS:
    // 清除所有绘图
    void clear();

protected:
    // 初始化figure的连接，这个函数用于重载createFigure函数时创建fig后绑定槽函数到DAChartOperateWidget用
    void initFigureConnect(DAFigureWidget* fig);
private Q_SLOTS:
    // 嵌套停靠区聚焦 dock 改变（过滤掉非本管理器的顶层 dock）
    void onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock);
    // 绘图的标题改变槽函数
    void onFigureTitleChanged(const QString& t);
private:
    // dock 关闭请求处理（经 closeRequested 信号触发，传入对应的 figure）
    void onFigureCloseRequested(DAFigureWidget* fig);
Q_SIGNALS:
    /**
     * @brief 绘图即将关闭
     * @param f
     */
    void figureRemoving(DA::DAFigureWidget* f);
    /**
     * @brief 创建了一个绘图
     * @param f
     */
    void figureCreated(DA::DAFigureWidget* f);
    /**
     * @brief 绘图的标题发生了改变
     * @param f
     * @param title
     */
    void figureTitleChanged(DA::DAFigureWidget* f, const QString& title);

    /**
     * @brief 当前的figure改变了
     * @param f 当前激活的figure
     * @param index 当前激活的figure对应的index
     */
    void currentFigureChanged(DA::DAFigureWidget* f, int index);
};
}  // end of namespace DA
#endif  // DACHARTOPERATEWIDGET_H
