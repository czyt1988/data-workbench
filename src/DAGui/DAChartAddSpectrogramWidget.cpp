#include "DAChartAddSpectrogramWidget.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_grid_raster_data.h"
namespace DA
{
DAChartAddSpectrogramWidget::DAChartAddSpectrogramWidget(QWidget* parent) : DAChartAddtGridRasterDataWidget(parent)
{
}

DAChartAddSpectrogramWidget::~DAChartAddSpectrogramWidget()
{
}

QwtPlotItem* DAChartAddSpectrogramWidget::createPlotItem()
{
	QwtGridRasterData* raster = makeSeries();
	QwtPlotSpectrogram* item  = new QwtPlotSpectrogram();
	item->setData(raster);

	return item;
}
}
