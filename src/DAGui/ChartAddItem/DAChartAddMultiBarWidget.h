#ifndef DACHARTADDMULTIBARWIDGET_H
#define DACHARTADDMULTIBARWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAData.h"
namespace Ui
{
class DAChartAddMultiBarWidget;
}
namespace DA
{
class DADataManager;
class DAPySeries;

/**
 * @brief 添加多重柱状图
 *
 * 用户选择一个 X 列（或自增）和多个 Y 列，每个 X 位置对应一组柱状图，
 * 基于 QwtPlotMultiBarChart 渲染。
 */
class DAGUI_API DAChartAddMultiBarWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddMultiBarWidget(QWidget* parent = nullptr);
	~DAChartAddMultiBarWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
private Q_SLOTS:
	void onXSeriesChanged();
	void onYSeriesChanged();
	void onGroupBoxXAutoincrementClicked(bool on);
	void onButtonXRemoveClicked();
	void onButtonYRemoveClicked();
private:
	bool tryGetXSelfInc(double& base, double& step);
	bool extractXSeries(std::vector< double >& res);
	bool extractYSeriesList(QVector< std::vector< double > >& res, QStringList& names);
private:
	Ui::DAChartAddMultiBarWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDMULTIBARWIDGET_H
