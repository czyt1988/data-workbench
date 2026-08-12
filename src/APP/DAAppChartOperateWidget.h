#ifndef DAAPPCHARTOPERATEWIDGET_H
#define DAAPPCHARTOPERATEWIDGET_H
#include "DAChartOperateWidget.h"
#include "DAData.h"
#include "DAFigureAPI.h"

#include "qwt_plot_item.h"
#if DA_ENABLE_PYTHON
#include "qwt3d_plotitem.h"
#endif
namespace DA
{
#if DA_ENABLE_PYTHON
class DADialogChartGuide;
class DAChart3DWidget;
#endif
class DAEvenFilterDragPlotWithGuide;
/**
 * @brief DAChartOperateWidget的app特化
 */
class DAAppChartOperateWidget : public DAChartOperateWidget
{
    Q_OBJECT
public:
    DAAppChartOperateWidget(QWidget* parent = nullptr);
    ~DAAppChartOperateWidget() override;
    // 设置data manager
    void setDataManager(DADataManager* mgr);
    // 添加一个Figure
    virtual DAFigureWidget* createFigure(const QString& name = QString()) override;
    // 绘制,如果没成功，返回nullptr
    // int execPlotGuideDialog(DA::DAChartTypes t = DA::DAChartTypes::Curve);
#if DA_ENABLE_PYTHON
    // 通过open打开引导对话框来创建item，这时将通过createItem信号来接收创建好的item
    void showPlotGuideDialog(DA::DAChartTypes t = DA::DAChartTypes::Curve);
    // 获取绘图指引对话框
    DADialogChartGuide* getChartGuideDlg();
#endif
Q_SIGNALS:
    /**
     * @brief createItem
     * @param fig
     * @param plot
     * @param item
     */
    void plotItemCreated(DAFigureWidget* fig, DAChartWidget* plot, QwtPlotItem* item);
#if DA_ENABLE_PYTHON
    /**
     * @brief 3D绘图项创建完成
     * @param fig figure窗口
     * @param plot 3D图表窗口
     * @param item 3D绘图item
     */
    void plot3DItemCreated(DAFigureWidget* fig, DAChart3DWidget* plot, Qwt3DPlotItem* item);
#endif
private Q_SLOTS:
    void onChartGuideAccept();

private:
    void initChartGuideDialog();

private:
    DADataManager* mDataMgr { nullptr };
#if DA_ENABLE_PYTHON
    DADialogChartGuide* mChartGuideDlg { nullptr };
#endif
    DAEvenFilterDragPlotWithGuide* mFigEventFilter { nullptr };  ///< 给DAFigureWidget的eventfilter
};
}

#endif  // DAAPPCHARTOPERATEWIDGET_H
