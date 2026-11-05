#ifndef DACHARTADDVECTORFIELDWIDGET_H
#define DACHARTADDVECTORFIELDWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractChartAddItemWidget.h"
#include "qwt_samples.h"
#include <QVector>
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
	// 编程式入口（快速绘图等无界面调用方）：直接给采样后的 (x, y, u, v) 向量样本
	void setProgrammaticSamples(const QVector< QwtVectorFieldSample >& samples, const QString& title = QString());
	// 是否已设置编程式数据
	bool hasProgrammaticSamples() const;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
private:
	QwtPlotItem* buildItem(const QVector< QwtVectorFieldSample >& samples, const QString& title);
private:
	Ui::DAChartAddVectorFieldWidget* ui;
	QVector< QwtVectorFieldSample > mProgrammaticSamples;
	QString mProgrammaticTitle;
};
}  // namespace DA
#endif  // DACHARTADDVECTORFIELDWIDGET_H
