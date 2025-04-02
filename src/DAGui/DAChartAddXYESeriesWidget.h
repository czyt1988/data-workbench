#ifndef DACHARTADDXYESERIESWIDGET_H
#define DACHARTADDXYESERIESWIDGET_H
#include "DAGuiAPI.h"
#include "qwt_plot_intervalcurve.h"
#include <QWidget>
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

class DADataManager;

/**
 * @brief 添加xye series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddXYESeriesWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartAddXYESeriesWidget)
public:
	explicit DAChartAddXYESeriesWidget(QWidget* parent = nullptr);
	~DAChartAddXYESeriesWidget();
	//
	void setDataManager(DADataManager* dmgr);
	DADataManager* getDataManager() const;
	// 判断x是否是自增
	bool isXAutoincrement() const;
	// 判断y是否是自增
	bool isYAutoincrement() const;
	// 根据配置获取数据
	QVector< QwtIntervalSample > getSeries() const;
	// 设置dataframe
	void setCurrentData(const DAData& d);
private slots:
	void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYECurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onGroupBoxXAutoincrementClicked(bool on);
	void onGroupBoxYAutoincrementClicked(bool on);

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
};
}

#endif  // DACHARTADDXYESERIESWIDGET_H
