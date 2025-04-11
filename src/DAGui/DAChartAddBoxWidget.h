#ifndef DACHARTADDBOXWIDGET_H
#define DACHARTADDBOXWIDGET_H
#include <QWidget>
#include "DAAbstractChartAddItemWidget.h"
#include "DAData.h"
#include "DADataManager.h"
class QAbstractButton;
namespace Ui
{
class DAChartAddBoxWidget;
}

namespace DA
{
/**
 * @brief 添加曲线
 */
class DAChartAddBoxWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartAddBoxWidget)
	void init();

public:
	explicit DAChartAddBoxWidget(QWidget* parent = nullptr);
	~DAChartAddBoxWidget();
	virtual QwtPlotItem* createPlotItem() override;
	// 设置当前的dataframe，这个仅仅会影响初始显示
	void setCurrentData(const DAData& d);
	// 设置datamanager,会把combox填入所有的dataframe
	void setDataManager(DADataManager* dmgr);
	// 下一步
	virtual void next() override;
	// 上一步
	virtual void previous() override;
	// 获取步骤总数
	virtual int getStepCount() const override;
	// 获取步骤总数
	virtual int getCurrentStep() const override;
	//
	void updateNavButtonState();
private slots:
	// 顶部导航按钮点击槽
	void onNavButtonClicked(QAbstractButton* button);
	//
	void onStackWidgetCurrentChanged(int i);

private:
	Ui::DAChartAddBoxWidget* ui;
};
}
#endif  // DACHARTADDBOXWIDGET_H
