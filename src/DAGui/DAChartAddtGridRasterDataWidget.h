#ifndef DACHARTADDTGRIDRASTERDATAWIDGET_H
#define DACHARTADDTGRIDRASTERDATAWIDGET_H
#include "DAGuiAPI.h"
#include "qwt_grid_raster_data.h"
#include "DAAbstractChartAddItemWidget.h"

class QwtMatrixRasterData;
// DAData
#include "DAData.h"
// DAGui
namespace Ui
{
class DAChartAddtGridRasterDataWidget;
}

namespace DA
{
#if DA_ENABLE_PYTHON
class DAPyGridDataTableModule;
#endif
class DADataManager;

/**
 * @brief 添加xymatrics series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddtGridRasterDataWidget : public DAAbstractChartAddItemWidget
{
	Q_OBJECT
public:
	explicit DAChartAddtGridRasterDataWidget(QWidget* parent = nullptr);
	~DAChartAddtGridRasterDataWidget();
	QwtGridRasterData* makeSeries() const;
	// 判断当前的维度是否正确
	bool isCorrectDim() const;
#if DA_ENABLE_PYTHON
	static QVector< QVector< double > > dataframeToMatrix(const DAPyDataFrame& df);
#endif
private slots:
	void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onComboBoxMatricsCurrentDataChanged(const DA::DAData& data);
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);

protected:
	QwtGridRasterData* makeGridDataFromUI();

private:
	Ui::DAChartAddtGridRasterDataWidget* ui;
	DAPyGridDataTableModule* mModel { nullptr };
};
}

#endif  // DACHARTADDTGRIDRASTERDATAWIDGET_H
