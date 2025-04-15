#ifndef DACHARTADDSPECTROGRAMWIDGET_H
#define DACHARTADDSPECTROGRAMWIDGET_H
#include <QWidget>
#include "DAAbstractChartAddItemWidget.h"
#include "DAData.h"
#include "DADataManager.h"
class QAbstractButton;
namespace Ui
{
class DAChartAddSpectrogramWidget;
}

namespace DA
{
/**
 * @brief 添加曲线
 */
class DAChartAddSpectrogramWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartAddSpectrogramWidget)
	void init();

public:
	explicit DAChartAddSpectrogramWidget(QWidget* parent = nullptr);
	~DAChartAddSpectrogramWidget();
	virtual QwtPlotItem* createPlotItem() override;
	// 设置当前的datafram，这个仅仅会影响初始显示
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
	Ui::DAChartAddSpectrogramWidget* ui;
};
}

#endif  // DACHARTADDSPECTROGRAMWIDGET_H
