#ifndef DADIALOGCHARTGUIDE_H
#define DADIALOGCHARTGUIDE_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include <QDialog>
#include <QList>
#include <QSet>
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
class DAChartAdd3DSurfaceWidget;
class DAChartAdd3DBarWidget;
class DAChartAdd3DLineWidget;
/**
 * @brief 把dataframe抽取两列转换为两个double-vector
 * @code
 * DADialogDataframeToPointVector dlg;
 * dlg.setDataManager(xx);
 * dlg.setCurrentData(xxx);
 * if(QDialog::Accept == dlg.exec()){
 *
 * }
 * @endcode
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
    DAAbstractChartAddItemWidget* getChartAddItemWidget(DA::DAChartTypes chartType) const;
    // 根据当前绘图类型设置item属性
    void initSetPlotItem(QwtPlotItem* item);
    // 创建3D绘图item，如果没有返回nullptr
    Qwt3DPlotItem* create3DPlotItem();
    // 获取当前的3D绘图指引窗口
    DAAbstractChart3DAddItemWidget* getCurrentChart3DAddItemWidget() const;
    // 根据chartType获取3D绘图指引窗口
    DAAbstractChart3DAddItemWidget* getChartAdd3DItemWidget(DA::DAChartTypes chartType) const;
    // 根据当前绘图类型设置3D item属性
    void initSet3DPlotItem(Qwt3DPlotItem* item);
    // 判断是否为3D图表类型
    static bool is3DChartType(DA::DAChartTypes t);
private Q_SLOTS:
    // 选择绘图类型改变
    void onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
    void initListWidget();
    void ensureWidgetDataManager(DAAbstractChartAddItemWidget* w);
    // 确保3D widget的dataManager已初始化
    void ensureWidget3DDataManager(DAAbstractChart3DAddItemWidget* w);

private:
    Ui::DADialogChartGuide* ui;
    DADataManager* mDataMgr { nullptr };
    QSet< DAAbstractChartAddItemWidget* > mInitializedWidgets;
    QSet< DAAbstractChart3DAddItemWidget* > mInitialized3DWidgets;
};
}  // end DA
#endif  // DADIALOGDATAFRAMEPLOT_H
