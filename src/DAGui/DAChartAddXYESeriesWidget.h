#ifndef DACHARTADDXYESERIESWIDGET_H
#define DACHARTADDXYESERIESWIDGET_H
#include "DAGuiAPI.h"
#include "qwt_samples.h"
#include "DAAbstractChartAddItemWidget.h"
// DAData
#include "DAData.h"
// DAUtil
#include "DAAutoincrementSeries.hpp"
// DAGui
namespace Ui
{
class DAChartAddXYESeriesWidget;
}

namespace DA
{
#if DA_ENABLE_PYTHON
class DAPySeriesTableModule;
#endif
class DADataManager;

/**
 * @brief 添加xye series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddXYESeriesWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddXYESeriesWidget(QWidget* parent = nullptr);
	~DAChartAddXYESeriesWidget();
	// 判断x是否是自增
	bool isXAutoincrement() const;
	// 判断y是否是自增
	bool isYAutoincrement() const;
	// 根据配置获取数据
	QVector< QwtIntervalSample > getSeries() const;
private slots:
	void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYECurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
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
	bool getToVectorPointFFromUI(QVector< QwtIntervalSample >& res);
	// 尝试获取x值得自增内容
	bool tryGetXSelfInc(double& base, double& step);
	bool tryGetYSelfInc(double& base, double& step);

private:
	Ui::DAChartAddXYESeriesWidget* ui;
	DAPySeriesTableModule* mModel { nullptr };
};
}

#endif  // DACHARTADDXYESERIESWIDGET_H
