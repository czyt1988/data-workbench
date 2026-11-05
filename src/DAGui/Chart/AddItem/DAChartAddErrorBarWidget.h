#ifndef DACHARTADDERRORBARWIDGET_H
#define DACHARTADDERRORBARWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAChartAddXYESeriesWidget.h"
namespace DA
{
/**
 * @brief 添加误差棒图
 *
 * 基于 QwtPlotIntervalCurve 实现 error bar 语义，复用 DAChartAddXYESeriesWidget
 * 的 x/y/error 三列数据提取逻辑，区间表示 y±error。
 */
class DAGUI_API DAChartAddErrorBarWidget : public DAChartAddXYESeriesWidget
{
	Q_OBJECT
public:
	explicit DAChartAddErrorBarWidget(QWidget* parent = nullptr);
	~DAChartAddErrorBarWidget();
	virtual QwtPlotItem* createPlotItem() override;
	// 编程式入口（快速绘图等无界面调用方）：直接给 x + [y-error, y+error] 区间样本
	void setProgrammaticSeries(const QVector< QwtIntervalSample >& samples, const QString& title = QString());
	// 是否已设置编程式数据
	bool hasProgrammaticSeries() const;
private:
	QVector< QwtIntervalSample > mProgrammaticSeries;
	QString mProgrammaticTitle;
};
}  // namespace DA
#endif  // DACHARTADDERRORBARWIDGET_H
