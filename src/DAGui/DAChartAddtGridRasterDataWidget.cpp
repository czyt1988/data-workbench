#include "DAChartAddtGridRasterDataWidget.h"
#include "ui_DAChartAddtGridRasterDataWidget.h"
#include <QMessageBox>
#include <qwt_interval.h>
#include <qwt_matrix_raster_data.h>
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModule.h"
#include "Models/DAPyDataFrameTableModule.h"
#endif
namespace DA
{

//===================================================
// DAChartAddtGridRasterDataWidget
//===================================================

DAChartAddtGridRasterDataWidget::DAChartAddtGridRasterDataWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddtGridRasterDataWidget)
{
	ui->setupUi(this);
#if DA_ENABLE_PYTHON
	//	mModel = new DAPySeriesTableModule(this);
	mModel = new DAPyDataFrameTableModule(nullptr, this);
	//	mModel->setHeaderLabel({ tr("x"), tr("y"), tr("matrics") });
	ui->tableViewRaster->setModel(mModel);
#endif
	QFontMetrics fm = fontMetrics();
	ui->tableViewRaster->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
	connect(this,
			&DAChartAddtGridRasterDataWidget::dataManagerChanged,
			this,
			&DAChartAddtGridRasterDataWidget::onDataManagerChanged);
	connect(this,
			&DAChartAddtGridRasterDataWidget::currentDataChanged,
			this,
			&DAChartAddtGridRasterDataWidget::onCurrentDataChanged);
	connect(ui->comboBoxX,
			&DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DAChartAddtGridRasterDataWidget::onComboBoxXCurrentDataframeSeriesChanged);
	connect(ui->comboBoxY,
			&DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DAChartAddtGridRasterDataWidget::onComboBoxYCurrentDataframeSeriesChanged);
	connect(ui->comboBoxMatrics,
			&DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DAChartAddtGridRasterDataWidget::onComboBoxMatricsCurrentDataframeSeriesChanged);
}

DAChartAddtGridRasterDataWidget::~DAChartAddtGridRasterDataWidget()
{
    delete ui;
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QwtGridRasterData* DAChartAddtGridRasterDataWidget::getSeries() const
{
	DAChartAddtGridRasterDataWidget* that = const_cast< DAChartAddtGridRasterDataWidget* >(this);
	QwtGridRasterData* raster             = new QwtGridRasterData();
	that->getToVectorPointFFromUI(*raster);
	return raster;
}

/**
 * @brief DAChartAddtGridRasterDataWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddtGridRasterDataWidget::onComboBoxXCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
	if (seriesName.isEmpty()) {
		return;
	}
#if DA_ENABLE_PYTHON
	DAPySeries series;
	DAPyDataFrame df = data.toDataFrame();
	if (!df.isNone()) {
		series = df[ seriesName ];
	}
	//		mModel->setSeriesAt(0, series);
	mModel->setDataFrame(df);
#endif
}

/**
 * @brief DAChartAddtGridRasterDataWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddtGridRasterDataWidget::onComboBoxYCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
	if (seriesName.isEmpty()) {
		return;
	}
#if DA_ENABLE_PYTHON
	DAPySeries series;
	DAPyDataFrame df = data.toDataFrame();
	if (!df.isNone()) {
		series = df[ seriesName ];
	}
	//	mModel->setSeriesAt(1, series);
	mModel->setDataFrame(df);
#endif
}

/**
 * @brief DAChartAddtGridRasterDataWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddtGridRasterDataWidget::onComboBoxMatricsCurrentDataframeSeriesChanged(const DAData& data,
                                                                                     const QString& seriesName)
{
	if (seriesName.isEmpty()) {
		return;
	}
#if DA_ENABLE_PYTHON
	DAPySeries series;
	DAPyDataFrame df = data.toDataFrame();
	//	if (!df.isNone()) {
	//		series = df[ seriesName ];
	//	}
	mModel->setDataFrame(df);
#endif
}

void DAChartAddtGridRasterDataWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxX->setDataManager(dmgr);
	ui->comboBoxY->setDataManager(dmgr);
	ui->comboBoxMatrics->setDataManager(dmgr);
}

void DAChartAddtGridRasterDataWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxX->setCurrentDAData(d);
	ui->comboBoxY->setCurrentDAData(d);
	ui->comboBoxMatrics->setCurrentDAData(d);
}

/**
 * @brief 获取点序列
 * @param res
 * @return
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddtGridRasterDataWidget::getToVectorPointFFromUI(QwtGridRasterData& res)
{
#if DA_ENABLE_PYTHON
	try {
		// 确保数据维度匹配
		QVector< double > data1            = { 0, 12.8, 25.6 };
		QVector< double > data2            = { 0, 0.0390625, 0.078125 };
		QVector< QVector< double > > data3 = { { 92.3322, 90.3358, 93.1646 },
											   { 95.8303, 97.5136, 108.795 },
											   { 70.7386, 97.5136, 109.925 } };

		// 验证数据维度
		if (data1.size() != data3.size() || data2.size() != data3[ 0 ].size()) {
			QMessageBox::warning(this, tr("Warning"), tr("Data dimensions do not match"));
			return false;
		}

		// 将二维数据转换为一维数组
		QVector< double > values;
		values.reserve(data3.size() * data3[ 0 ].size());
		for (const auto& row : data3) {
			values.append(row);
		}

		// 创建 QwtMatrixRasterData 对象
		QwtMatrixRasterData* matrixData = new QwtMatrixRasterData();
		matrixData->setValueMatrix(values, data2.size());
		matrixData->setResampleMode(QwtMatrixRasterData::BilinearInterpolation);

		// 设置区间
		QwtInterval xInterval(data1.first(), data1.last());
		QwtInterval yInterval(data2.first(), data2.last());

		// 计算Z轴区间
		double minZ = std::numeric_limits< double >::max();
		double maxZ = std::numeric_limits< double >::lowest();
		for (const auto& row : data3) {
			for (double val : row) {
				minZ = std::min(minZ, val);
				maxZ = std::max(maxZ, val);
			}
		}
		QwtInterval zInterval(minZ, maxZ);

		matrixData->setInterval(Qt::XAxis, xInterval);
		matrixData->setInterval(Qt::YAxis, yInterval);
		matrixData->setInterval(Qt::ZAxis, zInterval);

		// 设置数据
		res.setValue(data1, data2, data3);

		return true;
	} catch (const std::exception& e) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to set data: %1").arg(e.what()));
		return false;
	}
#else
	return false;
#endif
}
}
