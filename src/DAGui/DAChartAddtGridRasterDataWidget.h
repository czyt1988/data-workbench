#ifndef DACHARTADDTGRIDRASTERDATAWIDGET_H
#define DACHARTADDTGRIDRASTERDATAWIDGET_H
#include "DAGuiAPI.h"
#include "qwt_grid_raster_data.h"
#include "DAAbstractChartAddItemWidget.h"
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
class DAPyDataFrameTableModule;
class DAPySeriesTableModule;
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
	QwtGridRasterData* getSeries() const;
private slots:
	void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onComboBoxMatricsCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onDataManagerChanged(DADataManager* dmgr);
	void onCurrentDataChanged(const DAData& d);

protected:
	bool getToVectorPointFFromUI(QwtGridRasterData* res);

private:
	Ui::DAChartAddtGridRasterDataWidget* ui;
	//	DAPySeriesTableModule* mModel{ nullptr };
	DAPyDataFrameTableModule* mModel{ nullptr };
};
}

#endif  // DACHARTADDTGRIDRASTERDATAWIDGET_H
