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
	//	QVector< double > data1            = { 0, 12.8, 25.6 };
	//	QVector< double > data2            = { 0, 0.0390625, 0.078125 };
	//	QVector< QVector< double > > data3 = { { 92.3322, 90.3358, 93.1646 },
	//										   { 95.8303, 97.5136, 108.795 },
	//										   { 70.7386, 97.5136, 109.925 } };
	//	QwtGridRasterData* raster          = new QwtGridRasterData();

	//	if (data1.size() != data3[ 0 ].size() || data2.size() != data3.size()) {
	//		qWarning() << "Dimension mismatch";
	//		delete raster;
	//		return nullptr;
	//	}
	//	raster->setValue(data1, data2, data3);
	QwtGridRasterData* raster = getSeries();
	QwtPlotSpectrogram* item  = new QwtPlotSpectrogram();
	item->setData(raster);

	return item;
}
}
