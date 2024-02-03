#ifndef DAAPPCHARTOPERATEWIDGET_H
#define DAAPPCHARTOPERATEWIDGET_H
#include "DAChartOperateWidget.h"
#include "DAData.h"
#include "qwt_plot_item.h"
namespace DA
{
class DADialogChartGuide;

/**
 * @brief DAChartOperateWidget的app特化
 */
class DAAppChartOperateWidget : public DAChartOperateWidget
{
public:
    DAAppChartOperateWidget(QWidget* parent = nullptr);
    ~DAAppChartOperateWidget();
    // 设置data manager
    void setDataManager(DADataManager* mgr);
    // 添加一个Figure
    virtual DAFigureWidget* createFigure() override;

public:
    // 绘制,如果没成功，返回nullptr
    QwtPlotItem* plotWithGuideDialog(const DAData& data = DAData());

private:
    DADataManager* mDataMgr { nullptr };
    DADialogChartGuide* mChartGuideDlg { nullptr };
};
}

#endif  // DAAPPCHARTOPERATEWIDGET_H
