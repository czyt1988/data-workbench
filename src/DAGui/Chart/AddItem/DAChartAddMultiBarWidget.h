#ifndef DACHARTADDMULTIBARWIDGET_H
#define DACHARTADDMULTIBARWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractChartAddItemWidget.h"
#include "DAData.h"
#include "qwt_samples.h"
#include <QVector>
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
	// 编程式入口（快速绘图等无界面调用方）：samples 的 x 为类别序号（0 起），
	// set 为各数值列在该类别上的聚合值；barTitles 为各序列名，categoryLabels 为类别轴文字
	void setProgrammaticSamples(const QVector< QwtSetSample >& samples,
	                            const QStringList& barTitles,
	                            const QStringList& categoryLabels = QStringList(),
	                            const QString& title              = QString());
	// 是否已设置编程式数据
	bool hasProgrammaticSamples() const;
	// 编程式类别轴文字（无则空）
	QStringList programmaticCategoryLabels() const;
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
	QwtPlotItem* buildProgrammaticItem() const;
private:
	Ui::DAChartAddMultiBarWidget* ui;
	QVector< QwtSetSample > mProgrammaticSamples;
	QStringList mProgrammaticBarTitles;
	QStringList mProgrammaticCategoryLabels;
	QString mProgrammaticTitle;
};
}  // namespace DA
#endif  // DACHARTADDMULTIBARWIDGET_H
