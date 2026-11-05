#ifndef DACHARTADDCONTOURWIDGET_H
#define DACHARTADDCONTOURWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractChartAddItemWidget.h"
#include "qwt_point_3d.h"
#include <QVector>
namespace Ui
{
class DAChartAddContourWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加等高线图
 *
 * 用户选择 x/y/value 三列数据，基于 QwtPlotSpectroCurve 渲染等高线。
 */
class DAGUI_API DAChartAddContourWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddContourWidget(QWidget* parent = nullptr);
	~DAChartAddContourWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
	// 编程式入口（快速绘图等无界面调用方）：直接给采样后的 (x, y, value) 点集，
	// createPlotItem 将使用这份数据而不再读取界面选择
	void setProgrammaticSamples(const QVector<QwtPoint3D>& samples, const QString& title = QString());
	// 是否已设置编程式数据
	bool hasProgrammaticSamples() const;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
private:
	QwtPlotItem* buildItem(const QVector<QwtPoint3D>& samples, const QString& title);
private:
	Ui::DAChartAddContourWidget* ui;
	QVector<QwtPoint3D> mProgrammaticSamples;
	QString mProgrammaticTitle;
};
}  // namespace DA
#endif  // DACHARTADDCONTOURWIDGET_H
