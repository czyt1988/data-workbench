#ifndef DACHARTADDHISTOGRAMWIDGET_H
#define DACHARTADDHISTOGRAMWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
namespace Ui
{
class DAChartAddHistogramWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加直方图
 *
 * 用户选择一个数值序列并指定 bin 数量，自动进行等宽分箱统计，
 * 基于 QwtPlotHistogram 渲染。
 */
class DAGUI_API DAChartAddHistogramWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddHistogramWidget(QWidget* parent = nullptr);
	~DAChartAddHistogramWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);
private:
	Ui::DAChartAddHistogramWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDHISTOGRAMWIDGET_H
