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
	/**
	 * @brief Y 轴显示方案
	 */
	enum YAxisMode
	{
		Count,    ///< 累计值，每个 bin 的计数
		Density   ///< 概率密度，count/(total*binWidth)，积分为1
	};

	explicit DAChartAddHistogramWidget(QWidget* parent = nullptr);
	~DAChartAddHistogramWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
	// 获取 Y 轴显示方案
	YAxisMode getYAxisMode() const;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);
private:
	Ui::DAChartAddHistogramWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDHISTOGRAMWIDGET_H
