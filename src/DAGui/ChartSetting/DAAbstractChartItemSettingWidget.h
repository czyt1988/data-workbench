#ifndef DAABSTRACTCHARTITEMSETTINGWIDGET_H
#define DAABSTRACTCHARTITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
// qwt
class QwtPlotItem;
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
	virtual void setPlotItem(QwtPlotItem* item);
	QwtPlotItem* getPlotItem() const;

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
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

protected:
	QwtPlotItem* mPlotItem { nullptr };
	QPointer< QwtPlot > mPlot { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTITEMSETTINGWIDGET_H
