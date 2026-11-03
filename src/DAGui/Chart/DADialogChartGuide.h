#ifndef DADIALOGCHARTGUIDE_H
#define DADIALOGCHARTGUIDE_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include <QDialog>
#include <QList>
#include "DAData.h"
#include "qwt_plot_item.h"
#include "qwt3d_plotitem.h"
namespace Ui
{
class DADialogChartGuide;
}

class QListWidgetItem;
namespace DA
{
class DADataManager;
class DAAbstractChartAddItemWidget;
class DAAbstractChart3DAddItemWidget;
class DAAbstractChartAddWidgetBase;

/**
 * @brief 绘图引导窗口
 *
 * 左侧列表 + 右侧页面栈：列表内容与页面创建方式全部来自
 * DAChartAddItemWidgetFactory 注册表，本类只负责容器与路由。
 */
class DAGUI_API DADialogChartGuide : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADialogChartGuide)
public:
    explicit DADialogChartGuide(QWidget* parent = nullptr);
    ~DADialogChartGuide();
    // 设置datamanager,会把combox填入所有的dataframe
    void setDataManager(DADataManager* dmgr);
    // 获取当前的绘图类型
    DA::DAChartTypes getCurrentChartType() const;
    // 设置当前的绘图类型
    void setCurrentChartType(DA::DAChartTypes t);
    // 获取绘图item，如果没有返回nullptr
    QwtPlotItem* createPlotItem();
    // 批量获取绘图item（如曲线的 1 个 x + 多个 y），无法创建返回空列表
    QList< QwtPlotItem* > createPlotItems();
    // 获取当前添加部件提供的坐标轴标题推荐（x/y 轴标题），不提供返回 false
    bool getAxisTitleHint(QString& xTitle, QString& yTitle) const;
    // 获取当前的绘图指引窗口
    DAAbstractChartAddItemWidget* getCurrentChartAddItemWidget() const;
    // 根据类型获取二维绘图指引窗口（页面未创建时按需创建）
    DAAbstractChartAddItemWidget* getChartAddItemWidget(DA::DAChartTypes chartType);
    // 根据当前绘图类型设置item属性
    void initSetPlotItem(QwtPlotItem* item);
    // 创建3D绘图item，如果没有返回nullptr
    Qwt3DPlotItem* create3DPlotItem();
    // 获取当前的3D绘图指引窗口
    DAAbstractChart3DAddItemWidget* getCurrentChart3DAddItemWidget() const;
    // 根据chartType获取3D绘图指引窗口（页面未创建时按需创建）
    DAAbstractChart3DAddItemWidget* getChartAdd3DItemWidget(DA::DAChartTypes chartType);
    // 根据当前绘图类型设置3D item属性
    void initSet3DPlotItem(Qwt3DPlotItem* item);
    // 判断是否为3D图表类型
    static bool is3DChartType(DA::DAChartTypes t);

private Q_SLOTS:
    // 选择绘图类型改变
    void onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
    void initListWidget();
    // 切换到指定类型的页面（按需创建）
    void selectPage(DA::DAChartTypes type);
    // 按注册表创建并缓存页面，未注册返回 nullptr
    QWidget* ensurePage(DA::DAChartTypes type);
    void ensureWidgetDataManager(DAAbstractChartAddWidgetBase* w);

private:
    Ui::DADialogChartGuide* ui;
    DADataManager* mDataMgr { nullptr };
};
}  // end DA
#endif  // DADIALOGDATAFRAMEPLOT_H
