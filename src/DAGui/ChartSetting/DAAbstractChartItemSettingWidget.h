﻿#ifndef DAABSTRACTCHARTITEMSETTINGWIDGET_H
#define DAABSTRACTCHARTITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
// qwt
#include "qwt_plot.h"
#include "qwt_plot_item.h"
class QwtPlot;

/**
 * @def DAAbstractChartItemSettingWidget_ReturnWhenItemNull
 * @brief 如果getPlotItem返回空则return，此宏经常用于DAAbstractChartItemSettingWidget之类的槽函数进行判断是否设置了item
 *
 */
#ifndef DAAbstractChartItemSettingWidget_ReturnWhenItemNull
#define DAAbstractChartItemSettingWidget_ReturnWhenItemNull                                                            \
	do {                                                                                                               \
		if (nullptr == getPlotItem()) {                                                                                \
			return;                                                                                                    \
		}                                                                                                              \
	} while (0)

#endif

namespace DA
{
/**
 * @brief chart设置的基类封装了基本操作
 */
class DAGUI_API DAAbstractChartItemSettingWidget : public QWidget
{
	Q_OBJECT
public:
	DAAbstractChartItemSettingWidget(QWidget* parent = nullptr);
	~DAAbstractChartItemSettingWidget();
	// 设置plotitem
	void setPlotItem(QwtPlotItem* item);
	QwtPlotItem* getPlotItem() const;
	// 判断是否有item
	bool isHaveItem() const;
	// 判断当前item是否是对应的rtti，如果没有item也返回false
	bool checkItemRTTI(QwtPlotItem::RttiValues rtti) const;
	// 获取plot
	QwtPlot* getPlot() const;
	/**
	 * @brief 快捷转换为别的item
	 * @return
	 */
	template< typename T >
	T d_cast()
	{
		return dynamic_cast< T >(mPlotItem);
	}
	/**
	 * @brief 快捷转换为别的item
	 * @return
	 */
	template< typename T >
	T s_cast()
	{
		return static_cast< T >(mPlotItem);
	}
    // setPlotItem之后调用的虚函数,通过重写此函数可以执行一些设置item之后的操作例如一些界面的初始化
	virtual void plotItemSet(QwtPlotItem* item);
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

protected:
	QwtPlotItem* mPlotItem { nullptr };
	QPointer< QwtPlot > mPlot { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTITEMSETTINGWIDGET_H
