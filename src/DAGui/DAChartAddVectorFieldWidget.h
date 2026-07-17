#ifndef DACHARTADDVECTORFIELDWIDGET_H
#define DACHARTADDVECTORFIELDWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
namespace Ui
{
class DAChartAddVectorFieldWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加向量场图
 *
 * 用户选择 x/y/u/v 四列数据，基于 QwtPlotVectorField 渲染向量场。
 */
class DAGUI_API DAChartAddVectorFieldWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddVectorFieldWidget(QWidget* parent = nullptr);
	~DAChartAddVectorFieldWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
private:
	Ui::DAChartAddVectorFieldWidget* ui;
};
}  // namespace DA
#endif  // DACHARTADDVECTORFIELDWIDGET_H
