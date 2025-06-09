#include "DAChartAddtGridRasterDataWidget.h"
#include "ui_DAChartAddtGridRasterDataWidget.h"
#include <QMessageBox>
#include <qwt_interval.h>
#include <qwt_matrix_raster_data.h>
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPyGridDataTableModel.h"
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
	mModel = new DAPyGridDataTableModel(nullptr, this);
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
            &DADataManagerComboBox::currentDataChanged,
            this,
            &DAChartAddtGridRasterDataWidget::onComboBoxMatricsCurrentDataChanged);
}

DAChartAddtGridRasterDataWidget::~DAChartAddtGridRasterDataWidget()
{
    delete ui;
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QwtGridRasterData* DAChartAddtGridRasterDataWidget::makeSeries() const
{
	DAChartAddtGridRasterDataWidget* that = const_cast< DAChartAddtGridRasterDataWidget* >(this);
	return that->makeGridDataFromUI();
}

/**
 * @brief 判断当前维度是否正确
 * @return
 */
bool DAChartAddtGridRasterDataWidget::isCorrectDim() const
{
	const DAPySeries& x        = mModel->xSeries();
	const DAPySeries& y        = mModel->ySeries();
	const DAPyDataFrame& value = mModel->dataFrame();
	if (x.isNone() || y.isNone() || value.isNone()) {
		return false;
	}
	auto shape = value.shape();
	if (shape.first != y.size() || shape.second != x.size()) {
		return false;
	}
	return true;
}

/**
 * @brief 转换为矩阵
 * @param df
 * @return
 */
QVector< QVector< double > > DAChartAddtGridRasterDataWidget::dataframeToMatrix(const DAPyDataFrame& df)
{
	QVector< QVector< double > > res;
	try {
		auto shape = df.shape();
        res.reserve(static_cast< int >(shape.second));
		for (std::size_t i = 0; i < shape.second; ++i) {
			QVector< double > col;
            col.reserve(static_cast< int >(shape.first));
			df[ i ].castTo< double >(std::back_inserter(col));
			res.push_back(col);
		}
	} catch (const std::exception& e) {
		qWarning() << e.what();
	}
	return res;
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
		mModel->setGridX(series);
	}
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
	mModel->setGridY(series);
#endif
}

/**
 * @brief DAChartAddtGridRasterDataWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddtGridRasterDataWidget::onComboBoxMatricsCurrentDataChanged(const DAData& data)
{
#if DA_ENABLE_PYTHON
	DAPySeries series;
	DAPyDataFrame df = data.toDataFrame();
	if (!df.isNone()) {
		mModel->setDataFrame(df);
	}
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
QwtGridRasterData* DAChartAddtGridRasterDataWidget::makeGridDataFromUI()
{
#if DA_ENABLE_PYTHON
	try {
		// 验证数据维度
		if (!isCorrectDim()) {
			QMessageBox::warning(this,
                                 tr("Warning"),
                                 tr("The data dimensions are incorrect. The length of x should be equal to the number "
                                    "of columns in "
                                    "value, and the length of y should be equal to the number of rows in value."));  // cn:数据维度不正确，要求x长度和value的列数相等，y的长度和value的行数相等
			return nullptr;
		}

		const DAPySeries& xSeries    = mModel->xSeries();
		const DAPySeries& ySeries    = mModel->ySeries();
		const DAPyDataFrame& valueDf = mModel->dataFrame();

		// 将二维数据转换为一维数组
		QVector< double > x;
		QVector< double > y;
		x.reserve(xSeries.size());
		y.reserve(ySeries.size());
		xSeries.castTo< double >(std::back_inserter(x));
		ySeries.castTo< double >(std::back_inserter(y));

		QVector< QVector< double > > value = dataframeToMatrix(valueDf);

		// 创建 QwtMatrixRasterData 对象
		std::unique_ptr< QwtGridRasterData > gridData = std::make_unique< QwtGridRasterData >();
		gridData->setValue(x, y, value);

		return gridData.release();
	} catch (const std::exception& e) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to set data: %1").arg(e.what()));
		return nullptr;
	}
#endif
	return nullptr;
}
}
