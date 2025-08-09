#ifndef DAAPPCHARTOPERATEWIDGET_H
#define DAAPPCHARTOPERATEWIDGET_H
#include "DAChartOperateWidget.h"
#include "DAData.h"
#include "DAFigureAPI.h"

#include "qwt_plot_item.h"
namespace DA
{
#if DA_ENABLE_PYTHON
class DADialogChartGuide;
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
	~DAAppChartOperateWidget();
	// 设置data manager
	void setDataManager(DADataManager* mgr);
	// 添加一个Figure
	virtual DAFigureWidget* createFigure() override;

public:
	// 绘制,如果没成功，返回nullptr
	QwtPlotItem* createPlotItemWithGuideDialog(const DAData& data = DAData(), DA::ChartTypes t = DA::ChartTypes::Curve);
	// 调用绘图引导窗口进行引导性绘图
	void plotWithGuideDialog(DA::ChartTypes t = DA::ChartTypes::Curve);

private:
	DADataManager* mDataMgr { nullptr };
#if DA_ENABLE_PYTHON
	DADialogChartGuide* mChartGuideDlg { nullptr };
#endif
	DAEvenFilterDragPlotWithGuide* mFigEventFilter;  ///< 给DAFigureWidget的eventfilter
};
}

#endif  // DAAPPCHARTOPERATEWIDGET_H
