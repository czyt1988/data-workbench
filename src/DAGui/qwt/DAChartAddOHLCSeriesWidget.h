#ifndef DACHARTADDOHLCSERIESWIDGET_H
#define DACHARTADDOHLCSERIESWIDGET_H
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
class DAChartAddOHLCSeriesWidget;
}

namespace DA
{

class DADataManager;

/**
 * @brief 添加OHLC series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddOHLCSeriesWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartAddOHLCSeriesWidget)
public:
	explicit DAChartAddOHLCSeriesWidget(QWidget* parent = nullptr);
	~DAChartAddOHLCSeriesWidget();
	// 判断t是否是自增
	bool isTAutoincrement() const;
	// 根据配置获取数据
	QVector< QwtOHLCSample > getSeries() const;

private slots:
	void onComboBoxTCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxOCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxHCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxLCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxCCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onGroupBoxTAutoincrementClicked(bool on);
    void onDataManagerChanged(DADataManager* dmgr);
    void onCurrentDataChanged(const DAData& d);

protected:
	// 获取x自增
	bool getTAutoIncFromUI(DAAutoincrementSeries< double >& v);
	// 获取为vector pointf
	bool getToVectorPointFFromUI(QVector< QwtOHLCSample >& res);
	// 尝试获取t值得自增内容
	bool tryGetTSelfInc(double& base, double& step);

private:
	Ui::DAChartAddOHLCSeriesWidget* ui;
};
}

#endif  // DACHARTADDOHLCSERIESWIDGET_H
