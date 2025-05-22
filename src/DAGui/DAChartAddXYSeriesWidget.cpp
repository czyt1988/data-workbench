#include "DAChartAddXYSeriesWidget.h"
#include <QMessageBox>
#include "ui_DAChartAddXYSeriesWidget.h"
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include <QHeaderView>
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModel.h"
#endif
namespace DA
{

//===================================================
// DAChartAddXYSeriesWidget
//===================================================

DAChartAddXYSeriesWidget::DAChartAddXYSeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddXYSeriesWidget)
{
	ui->setupUi(this);
#if DA_ENABLE_PYTHON
	mModel = new DAPySeriesTableModel(this);
	mModel->setHeaderLabel({ tr("x"), tr("y") });
	ui->tableViewXY->setModel(mModel);
#endif
	QFontMetrics fm = fontMetrics();
	ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
	connect(this, &DAChartAddXYSeriesWidget::dataManagerChanged, this, &DAChartAddXYSeriesWidget::onDataManagerChanged);
	connect(this, &DAChartAddXYSeriesWidget::currentDataChanged, this, &DAChartAddXYSeriesWidget::onCurrentDataChanged);
	connect(ui->comboBoxX,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged);
	connect(ui->comboBoxY,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged);
	connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked);
	connect(ui->groupBoxYAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked);
}

DAChartAddXYSeriesWidget::~DAChartAddXYSeriesWidget()
{
	delete ui;
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddXYSeriesWidget::isXAutoincrement() const
{
    return ui->groupBoxXAutoincrement->isChecked();
}

/**
 * @brief 判断y是否是自增
 * @return
 */
bool DAChartAddXYSeriesWidget::isYAutoincrement() const
{
    return ui->groupBoxYAutoincrement->isChecked();
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QPointF > DAChartAddXYSeriesWidget::getSeries() const
{
	DAChartAddXYSeriesWidget* that = const_cast< DAChartAddXYSeriesWidget* >(this);
	QVector< QPointF > xy;
	that->getToVectorPointFFromUI(xy);
	return xy;
}

/**
 * @brief DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
    resetTableView();
#endif
}

/**
 * @brief DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
    resetTableView();

#endif
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked(bool on)
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
    resetTableView();
	ui->comboBoxX->setEnabled(!on);
#endif
}

/**
 * @brief y值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked(bool on)
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
    resetTableView();
	ui->comboBoxY->setEnabled(!on);
#endif
}

void DAChartAddXYSeriesWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxX->setDataManager(dmgr);
	ui->comboBoxY->setDataManager(dmgr);
}

void DAChartAddXYSeriesWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxX->setCurrentDAData(d);
	ui->comboBoxY->setCurrentDAData(d);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYSeriesWidget::getXAutoIncFromUI(DAAutoincrementSeries< double >& v)
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
bool DAChartAddXYSeriesWidget::getYAutoIncFromUI(DAAutoincrementSeries< double >& v)
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
bool DAChartAddXYSeriesWidget::getToVectorPointFFromUI(QVector< QPointF >& res)
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
		DAData yd = ui->comboBoxY->getCurrentDAData();
		if (!yd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
			return false;
		}
		DAPySeries y = yd.toSeries();
		if (y.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = y.size();
		try {
			std::vector< double > vy;
			vy.reserve(y.size());
			y.castTo< double >(std::back_inserter(vy));
			res.resize(static_cast< int >(s));
			for (int i = 0; i < s; ++i) {
				res[ i ].setX(xinc[ i ]);
				res[ i ].setY(vy[ i ]);
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
		DAData xd = ui->comboBoxX->getCurrentDAData();
		if (!xd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
			);
			return false;
		}
		DAPySeries x = xd.toSeries();
		if (x.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = x.size();
		try {
			std::vector< double > vx;
			vx.reserve(x.size());
			x.castTo< double >(std::back_inserter(vx));
			res.resize(static_cast< int >(s));
			for (auto i = 0; i < s; ++i) {
				res[ i ].setX(vx[ i ]);
				res[ i ].setY(yinc[ i ]);
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
		DAData xd = ui->comboBoxX->getCurrentDAData();
		DAData yd = ui->comboBoxY->getCurrentDAData();
		if (!xd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
			);
			return false;
		}
		if (!yd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
			return false;
		}
		DAPySeries x = xd.toSeries();
		DAPySeries y = yd.toSeries();
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
			std::vector< qreal > vx, vy;
			vx.reserve(x.size());
			vy.reserve(y.size());
			x.castTo< qreal >(std::back_inserter(vx));
			y.castTo< qreal >(std::back_inserter(vy));
			res.resize(static_cast< int >(s));
			for (auto i = 0; i < s; ++i) {
				res[ i ].setX(vx[ i ]);
				res[ i ].setY(vy[ i ]);
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
bool DAChartAddXYSeriesWidget::tryGetXSelfInc(double& base, double& step)
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
bool DAChartAddXYSeriesWidget::tryGetYSelfInc(double& base, double& step)
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

void DAChartAddXYSeriesWidget::resetTableView()
{
    if (!mModel) {
        return;
    }
    int r = mModel->actualRowCount();
    if (QHeaderView* vh = ui->tableViewXY->verticalHeader()) {
        int w = vh->fontMetrics().horizontalAdvance(QString(" %1 ").arg(r));
        vh->setFixedWidth(w);
    }
    ui->tableViewXY->showActualRow(0);
}
}
