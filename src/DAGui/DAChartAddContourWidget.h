#ifndef DACHARTADDCONTOURWIDGET_H
#define DACHARTADDCONTOURWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
namespace Ui
{
class DAChartAddContourWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加等高线图
 *
 * 用户选择 x/y/value 三列数据，基于 QwtPlotSpectroCurve 渲染等高线。
 */
class DAGUI_API DAChartAddContourWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddContourWidget(QWidget* parent = nullptr);
	~DAChartAddContourWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);
private:
	Ui::DAChartAddContourWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDCONTOURWIDGET_H
