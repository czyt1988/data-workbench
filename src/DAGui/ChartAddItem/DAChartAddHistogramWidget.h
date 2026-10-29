#ifndef DACHARTADDHISTOGRAMWIDGET_H
#define DACHARTADDHISTOGRAMWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
#include <QVector>
#include <QString>
#include <vector>
namespace Ui
{
class DAChartAddHistogramWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 添加直方图
 *
 * 用户选择一个数值序列并指定 bin 数量，自动进行等宽分箱统计，
 * 基于 QwtPlotHistogram 渲染。
 */
class DAGUI_API DAChartAddHistogramWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	/**
	 * @brief Y 轴显示方案
	 */
	enum YAxisMode
	{
		Count,    ///< 累计值，每个 bin 的计数
		Density   ///< 概率密度，count/(total*binWidth)，积分为1
	};

	explicit DAChartAddHistogramWidget(QWidget* parent = nullptr);
	~DAChartAddHistogramWidget();
	virtual QwtPlotItem* createPlotItem() override;
	virtual void setDataManager(DADataManager* dmgr) override;
	// 获取 Y 轴显示方案
	YAxisMode getYAxisMode() const;
	// 编程式数据入口：直接给定数值序列与分箱数，createPlotItem 优先使用它，
	// 不再要求先在界面上选 DataFrame（供快速绘图等无界面路径复用同一套分箱实现）
	void setProgrammaticValues(const QVector< double >& values, int binCount, const QString& title = QString());
	// 清除编程式数据，回到界面取数路径
	void clearProgrammaticValues();
	// 是否处于编程式数据模式
	bool hasProgrammaticValues() const;
private Q_SLOTS:
	void onDataManagerChanged(DADataManager* dmgr);
private:
	// 按当前 Y 轴方案把数值序列分箱成直方图图元（唯一的图元构建实现）
	QwtPlotItem* buildHistogramItem(const std::vector< double >& raw, const QString& title, int binCount) const;
private:
	Ui::DAChartAddHistogramWidget* ui;
	QVector< double > mProgrammaticValues;
	QString mProgrammaticTitle;
	int mProgrammaticBins { 10 };
	bool mHasProgrammatic { false };
};
}  // namespace DA
#endif  // DACHARTADDHISTOGRAMWIDGET_H
