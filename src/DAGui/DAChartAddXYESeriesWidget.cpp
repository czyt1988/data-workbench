#include "DAChartAddXYESeriesWidget.h"
#include "ui_DAChartAddXYESeriesWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include "qwt_plot_intervalcurve.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModule.h"
#endif
namespace DA
{

//===================================================
// DAChartAddXYESeriesWidget
//===================================================

DAChartAddXYESeriesWidget::DAChartAddXYESeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddXYESeriesWidget)
{
	ui->setupUi(this);
#if DA_ENABLE_PYTHON
    mModel = new DAPySeriesTableModule(this);
    mModel->setHeaderLabel({ tr("x"), tr("y"), tr("error") });
    ui->tableViewXYE->setModel(mModel);
#endif
	QFontMetrics fm = fontMetrics();
	ui->tableViewXYE->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    connect(this, &DAChartAddXYESeriesWidget::dataManagerChanged, this, &DAChartAddXYESeriesWidget::onDataManagerChanged);
    connect(this, &DAChartAddXYESeriesWidget::currentDataChanged, this, &DAChartAddXYESeriesWidget::onCurrentDataChanged);
	connect(ui->comboBoxX,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddXYESeriesWidget::onComboBoxXCurrentDataframeSeriesChanged);
	connect(ui->comboBoxY,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged);
	connect(ui->comboBoxYE,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddXYESeriesWidget::onComboBoxYECurrentDataframeSeriesChanged);
	connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYESeriesWidget::onGroupBoxXAutoincrementClicked);
	connect(ui->groupBoxYAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYESeriesWidget::onGroupBoxYAutoincrementClicked);
}

DAChartAddXYESeriesWidget::~DAChartAddXYESeriesWidget()
{
	delete ui;
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddXYESeriesWidget::isXAutoincrement() const
{
    return ui->groupBoxXAutoincrement->isChecked();
}

/**
 * @brief 判断y是否是自增
 * @return
 */
bool DAChartAddXYESeriesWidget::isYAutoincrement() const
{
    return ui->groupBoxYAutoincrement->isChecked();
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QwtIntervalSample > DAChartAddXYESeriesWidget::getSeries() const
{
	DAChartAddXYESeriesWidget* that = const_cast< DAChartAddXYESeriesWidget* >(this);
	QVector< QwtIntervalSample > xye;
	that->getToVectorPointFFromUI(xye);
	return xye;
}

/**
 * @brief 此函数创建QwtPlotIntervalCurve
 * @return
 */
QwtPlotItem* DAChartAddXYESeriesWidget::createPlotItem()
{
    QVector< QwtIntervalSample > xye = getSeries();
    if (xye.empty()) {
        return nullptr;
    }
    QwtPlotIntervalCurve* item = new QwtPlotIntervalCurve();
    item->setSamples(xye);
    return item;
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onComboBoxXCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
    mModel->setSeriesAt(0, series);
#endif
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
    mModel->setSeriesAt(1, series);
#endif
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onComboBoxYECurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
    mModel->setSeriesAt(2, series);
#endif
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddXYESeriesWidget::onGroupBoxXAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
	if (on) {
		double base, step;
		if (tryGetXSelfInc(base, step)) {
            mModel->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
		}
	} else {
		// 取消要读取回原来的设置
		DAPySeries series;
		DAData data = ui->comboBoxX->getCurrentDAData();
		if (data) {
			series = data.toSeries();
		}
        mModel->setSeriesAt(0, series);
	}
	ui->comboBoxX->setEnabled(!on);
#endif
}

/**
 * @brief y值是否使用自增序列
 * @param on
 */
void DAChartAddXYESeriesWidget::onGroupBoxYAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
	if (on) {
		double base, step;
		if (tryGetYSelfInc(base, step)) {
            mModel->setSeriesAt(1, DAAutoincrementSeries< double >(base, step));
		}
	} else {
		// 取消要读取回原来的设置
		DAPySeries series;
		DAData data = ui->comboBoxY->getCurrentDAData();
		if (data) {
			series = data.toSeries();
		}
        mModel->setSeriesAt(1, series);
	}
	ui->comboBoxY->setEnabled(!on);
#endif
}

void DAChartAddXYESeriesWidget::onDataManagerChanged(DADataManager* dmgr)
{
    ui->comboBoxX->setDataManager(dmgr);
    ui->comboBoxY->setDataManager(dmgr);
    ui->comboBoxYE->setDataManager(dmgr);
}

void DAChartAddXYESeriesWidget::onCurrentDataChanged(const DAData& d)
{
    ui->comboBoxX->setCurrentDAData(d);
    ui->comboBoxY->setCurrentDAData(d);
    ui->comboBoxYE->setCurrentDAData(d);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getXAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
	bool isOK   = false;
	double base = ui->lineEditXInitValue->text().toDouble(&isOK);
	if (!isOK) {
        QMessageBox::
            warning(this,
                    tr("Warning"),  // cn:警告
                    tr("The initial value of x auto increment series must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
            );
		return false;
	}
	double step = ui->lineEditXStepValue->text().toDouble(&isOK);
	if (!isOK) {
		QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of x auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的步长必须为浮点数
		);
		return false;
	}
	v.setBaseValue(base);
	v.setStepValue(step);
	return true;
}

/**
 * @brief 获取y自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getYAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
	bool isOK   = false;
	double base = ui->lineEditYInitValue->text().toDouble(&isOK);
	if (!isOK) {
		QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The initial value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
		);
		return false;
	}
	double step = ui->lineEditYStepValue->text().toDouble(&isOK);
	if (!isOK) {
		QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的步长必须为浮点数
		);
		return false;
	}
	v.setBaseValue(base);
	v.setStepValue(step);
	return true;
}

/**
 * @brief 获取点序列
 * @param res
 * @return
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getToVectorPointFFromUI(QVector< QwtIntervalSample >& res)
{
	bool isXAuto = ui->groupBoxXAutoincrement->isChecked();
	bool isYAuto = ui->groupBoxYAutoincrement->isChecked();
	if (isXAuto && isYAuto) {
		QMessageBox::warning(this,
                             tr("Warning"),                                                 // cn:警告
                             tr("x and y cannot be set to autoincrement at the same time")  // cn:x和y无法同时设置为自增
		);
		return false;
	}
#if DA_ENABLE_PYTHON
	if (isXAuto) {  // 不存在同时，因此这个就是x自增
		DAAutoincrementSeries< double > xinc;
		if (!getXAutoIncFromUI(xinc)) {
			return false;
		}
		DAData yCenter = ui->comboBoxY->getCurrentDAData();
		DAData yError  = ui->comboBoxYE->getCurrentDAData();
		if (!yCenter.isSeries() || !yError.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
			return false;
		}
		DAPySeries y = yCenter.toSeries();
		DAPySeries e = yError.toSeries();
		if (y.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = y.size();
		try {
			std::vector< double > yCenter;
			std::vector< double > yError;
			yCenter.reserve(y.size());
			yError.reserve(e.size());
			y.castTo< double >(std::back_inserter(yCenter));
			e.castTo< double >(std::back_inserter(yError));
			res.resize(static_cast< int >(s));
			for (int i = 0; i < s; ++i) {
				double min = yCenter[ i ] - yError[ i ];  // 计算区间范围
				double max = yCenter[ i ] + yError[ i ];
				res[ i ]   = QwtIntervalSample(xinc[ i ], min, max);
			}
		} catch (const std::exception& e) {
			qCritical() << tr("Exception occurred during extracting from "
                              "pandas.Series to double vector:%1")
                               .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
			QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从pandas.Series提取为double vector过程中出现异常

			return false;
		}
	} else if (isYAuto) {
		DAAutoincrementSeries< double > yinc;
		if (!getYAutoIncFromUI(yinc)) {
			return false;
		}
		DAData xd     = ui->comboBoxX->getCurrentDAData();
		DAData yError = ui->comboBoxYE->getCurrentDAData();
		if (!xd.isSeries() || !yError.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
			);
			return false;
		}
		DAPySeries x = xd.toSeries();
		DAPySeries e = yError.toSeries();
		if (x.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = x.size();
		try {
			std::vector< double > vx;
			std::vector< double > yError;
			vx.reserve(x.size());
			yError.reserve(e.size());
			x.castTo< double >(std::back_inserter(vx));
			e.castTo< double >(std::back_inserter(yError));
			res.resize(static_cast< int >(s));
			for (auto i = 0; i < s; ++i) {
				double min = yinc[ i ] - yError[ i ];
				double max = yinc[ i ] + yError[ i ];
				res[ i ]   = QwtIntervalSample(vx[ i ], min, max);
			}
		} catch (const std::exception& e) {
			qCritical() << tr("Exception occurred during extracting from "
                              "pandas.Series to double vector:%1")
                               .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
			QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从pandas.Series提取为double vector过程中出现异常

			return false;
		}
	} else {
		DAData xd      = ui->comboBoxX->getCurrentDAData();
		DAData yCenter = ui->comboBoxY->getCurrentDAData();
		DAData yError  = ui->comboBoxYE->getCurrentDAData();
		if (!xd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
			);
			return false;
		}
		if (!yCenter.isSeries() || !yError.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
			return false;
		}
		DAPySeries x = xd.toSeries();
		DAPySeries y = yCenter.toSeries();
		DAPySeries e = yError.toSeries();
		if (x.isNone() || y.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = std::min(x.size(), y.size());
		if (0 == s) {
			return true;
		}
		try {
			std::vector< double > vx;
			std::vector< double > yCenter;
			std::vector< double > yError;
			vx.reserve(x.size());
			yCenter.reserve(y.size());
			yError.reserve(e.size());
			x.castTo< qreal >(std::back_inserter(vx));
			y.castTo< double >(std::back_inserter(yCenter));
			e.castTo< double >(std::back_inserter(yError));
			res.resize(static_cast< int >(s));
			for (auto i = 0; i < s; ++i) {
				double min = yCenter[ i ] - yError[ i ];
				double max = yCenter[ i ] + yError[ i ];
				res[ i ]   = QwtIntervalSample(vx[ i ], min, max);
			}
		} catch (const std::exception& e) {
			qCritical() << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                               .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
			return false;
		}
	}
#endif
	return true;
}

/**
 * @brief 尝试获取x值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYESeriesWidget::tryGetXSelfInc(double& base, double& step)
{
	bool isOK = false;
	double a  = ui->lineEditXInitValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	double b = ui->lineEditXStepValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	base = a;
	step = b;
	return true;
}

/**
 * @brief 尝试获取y值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYESeriesWidget::tryGetYSelfInc(double& base, double& step)
{
	bool isOK = false;
	double a  = ui->lineEditYInitValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	double b = ui->lineEditYStepValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	base = a;
	step = b;
	return true;
}
}
