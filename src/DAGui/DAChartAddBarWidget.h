#ifndef DACHARTADDBARWIDGET_H
#define DACHARTADDBARWIDGET_H
#include <QWidget>
#include "DAAbstractChartAddItemWidget.h"
#include "DAData.h"
#include "DADataManager.h"
class QAbstractButton;
namespace Ui
{
class DAChartAddBarWidget;
}

namespace DA
{
/**
 * @brief 添加曲线
 */
class DAChartAddBarWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartAddBarWidget)
	void init();

public:
	explicit DAChartAddBarWidget(QWidget* parent = nullptr);
	~DAChartAddBarWidget();
	virtual QwtPlotItem* createPlotItem() override;
	// 设置当前的datafram，这个仅仅会影响初始显示
	void setCurrentData(const DAData& d);
	// 设置datamanager,会把combox填入所有的dataframe
	void setDataManager(DADataManager* dmgr);
	// 设置仅仅只有symbol，此时会把plot类型设置为no curve，把symbol 勾上
	void setBarMode(bool on);
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
	Ui::DAChartAddBarWidget* ui;
};
}

#endif  // DACHARTADDBARWIDGET_H
