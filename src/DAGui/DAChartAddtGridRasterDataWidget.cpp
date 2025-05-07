#include "DAChartAddtGridRasterDataWidget.h"
#include "ui_DAChartAddtGridRasterDataWidget.h"
#include <QMessageBox>
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
	QwtGridRasterData* raster = new QwtGridRasterData(); // 创建新实例
	if (!that->getToVectorPointFFromUI(raster)) {
		delete raster; // 如果获取数据失败，释放资源
		return nullptr;
	}
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
bool DAChartAddtGridRasterDataWidget::getToVectorPointFFromUI(QwtGridRasterData* res)
{
#if DA_ENABLE_PYTHON
	// 验证参数
	if (res == nullptr) {
		qCritical() << tr("QwtGridRasterData pointer is nullptr"); // cn:QwtGridRasterData指针为空
		return false;
	}

	try {
		// 硬编码数据作为测试 - 确保格式和维度匹配
		QVector< double > xAxis = { 0, 12.8, 25.6 };
		QVector< double > yAxis = { 0, 0.0390625, 0.078125 };
		QVector< QVector< double > > matrix = { 
			{ 92.3322, 90.3358, 93.1646 },
			{ 95.8303, 97.5136, 108.795 },
			{ 70.7386, 97.5136, 109.925 }  
		};
		
		// 设置数据到栅格对象
		res->setValue(xAxis, yAxis, matrix);
		return true;
	} catch (const std::exception& e) {
		qCritical() << tr("Exception occurred: %1").arg(e.what());
		return false;
	}
#endif
	return true;
}
}
