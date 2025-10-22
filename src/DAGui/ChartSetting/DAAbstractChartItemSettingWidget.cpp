#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_plot_item.h"
#include "qwt_plot.h"
namespace DA
{
DAAbstractChartItemSettingWidget::DAAbstractChartItemSettingWidget(QWidget* parent) : QWidget(parent)
{
}

DAAbstractChartItemSettingWidget::~DAAbstractChartItemSettingWidget()
{
}

/**
 * @brief 设置plotitem
 *
 * 注意，如果重载，必须保证调用了DAAbstractChartItemSettingWidget::setPlotItem(item),
 * 否则getPlotItem会失效，同时无法自动处理item的detach
 * @param item
 */
void DAAbstractChartItemSettingWidget::setPlotItem(QwtPlotItem* item)
{
	mPlotItem = item;
	// 如果item有plot，则把plot设置进来，plot可以知道item是否被delete
	QwtPlot* oldPlot = mPlot.data();
	QwtPlot* newPlot = nullptr;
	if (item) {
		newPlot = mPlotItem->plot();
		mPlot   = newPlot;
	}
	if (oldPlot != newPlot) {
		if (oldPlot) {
			disconnect(oldPlot, &QwtPlot::itemAttached, this, &DAAbstractChartItemSettingWidget::plotItemAttached);
		}
		if (newPlot) {
			connect(newPlot, &QwtPlot::itemAttached, this, &DAAbstractChartItemSettingWidget::plotItemAttached);
		}
	}
    updateUI(item);
}

/**
 * @brief setPlotItem之后调用的虚函数
 * @param item
 * @return
 */
QwtPlotItem* DAAbstractChartItemSettingWidget::getPlotItem() const
{
	return mPlotItem;
}

/**
 * @brief 判断是否有item
 * @return
 */
bool DAAbstractChartItemSettingWidget::isHaveItem() const
{
    return (mPlotItem != nullptr);
}

/**
 * @brief 判断当前item是否是对应的rtti，如果没有item也返回false
 * @param rtti
 * @return
 */
bool DAAbstractChartItemSettingWidget::checkItemRTTI(QwtPlotItem::RttiValues rtti) const
{
	if (!mPlotItem) {
		return false;
	}
	return (mPlotItem->rtti() == rtti);
}

/**
 * @brief 获取绘图
 * @return
 */
QwtPlot* DAAbstractChartItemSettingWidget::getPlot() const
{
    return mPlot;
}

void DAAbstractChartItemSettingWidget::updateUI(QwtPlotItem* item)
{
	Q_UNUSED(item);
}

void DAAbstractChartItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
{
	if (!on && plotItem == mPlotItem) {
		// item脱离plot，有可能会被delete
		setPlotItem(nullptr);
	}
}

}
