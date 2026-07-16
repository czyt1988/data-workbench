#ifndef DACHARTADDBOXCHARTWIDGET_H
#define DACHARTADDBOXCHARTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
namespace Ui
{
class DAChartAddBoxChartWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加箱线图
 *
 * 用户选择一个 DataFrame 并勾选多个数值列，每列自动计算五数（min/Q1/median/Q3/max）
 * 生成一个箱体，基于 QwtPlotBoxChart 渲染。
 */
class DAGUI_API DAChartAddBoxChartWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddBoxChartWidget(QWidget* parent = nullptr);
	~DAChartAddBoxChartWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
private Q_SLOTS:
	void onComboBoxCurrentDataChanged(const DAData& d);
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);
private:
	void refreshColumns();
private:
	Ui::DAChartAddBoxChartWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDBOXCHARTWIDGET_H
