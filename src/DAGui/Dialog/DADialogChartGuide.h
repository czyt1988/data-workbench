#ifndef DADIALOGCHARTGUIDE_H
#define DADIALOGCHARTGUIDE_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include <QDialog>
#include "DAData.h"
#include "qwt_plot_item.h"
namespace Ui
{
class DADialogChartGuide;
}

class QListWidgetItem;
namespace DA
{
class DADataManager;
class DAAbstractChartAddItemWidget;
/**
 * @brief 把dataframe抽取两列转换为两个double-vector
 * @code
 * DADialogDataframeToPointVector dlg;
 * dlg.setDataManager(xx);
 * dlg.setCurrentData(xxx);
 * if(QDialog::Accept == dlg.exec()){
 *
 * }
 * @endcode
 */
class DAGUI_API DADialogChartGuide : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADialogChartGuide)
public:
	explicit DADialogChartGuide(QWidget* parent = nullptr);
	~DADialogChartGuide();
	// 设置datamanager,会把combox填入所有的dataframe
	void setDataManager(DADataManager* dmgr);
	// 设置dataframe
	void setCurrentData(const DAData& d);
	// 获取当前的绘图类型
	DA::ChartTypes getCurrentChartType() const;
	// 设置当前的绘图类型
	void setCurrentChartType(DA::ChartTypes t);
	// 获取绘图item，如果没有返回nullptr
	QwtPlotItem* createPlotItem();
	// 更新数据
	void updateData();
	// 获取当前的绘图指引窗口
	DAAbstractChartAddItemWidget* getCurrentChartAddItemWidget() const;

private slots:
	// 选择绘图类型改变
	void onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
    void initListWidget();

private:
    Ui::DADialogChartGuide* ui;
};
}  // end DA
#endif  // DADIALOGDATAFRAMEPLOT_H
