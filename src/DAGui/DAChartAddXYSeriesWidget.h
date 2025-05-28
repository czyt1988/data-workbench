#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
// DAData
#include "DAData.h"
// DAUtil
#include "DAAutoincrementSeries.hpp"
// DAGui

namespace Ui
{
class DAChartAddXYSeriesWidget;
}

namespace DA
{
#if DA_ENABLE_PYTHON
class DAPySeriesTableModel;
#endif
class DADataManager;

/**
 * @brief 添加xy series，适用二维数据绘图的系列获取
 * 这个是一个abstract类，需要重写@sa createPlotItem
 */
class DAGUI_API DAChartAddXYSeriesWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddXYSeriesWidget(QWidget* parent = nullptr);
	~DAChartAddXYSeriesWidget();
	// 判断x是否是自增
	bool isXAutoincrement() const;
	// 判断y是否是自增
	bool isYAutoincrement() const;
	// 根据配置获取数据
	QVector< QPointF > getSeries() const;

private slots:
	void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onGroupBoxXAutoincrementClicked(bool on);
	void onGroupBoxYAutoincrementClicked(bool on);
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);

protected:
	// 获取x自增
	bool getXAutoIncFromUI(DAAutoincrementSeries< double >& v);
	// 获取y自增
	bool getYAutoIncFromUI(DAAutoincrementSeries< double >& v);
	// 获取为vector pointf
	bool getToVectorPointFFromUI(QVector< QPointF >& res);
	// 尝试获取x值得自增内容
	bool tryGetXSelfInc(double& base, double& step);
	bool tryGetYSelfInc(double& base, double& step);

private:
	Ui::DAChartAddXYSeriesWidget* ui;
};
}
#endif  // DACHARTADDXYSERIESWIDGET_H
