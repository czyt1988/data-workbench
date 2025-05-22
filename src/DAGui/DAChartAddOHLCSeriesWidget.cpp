#include "DAChartAddOHLCSeriesWidget.h"
#include "ui_DAChartAddOHLCSeriesWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include "qwt_samples.h"
#include "qwt_plot_tradingcurve.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModel.h"
#endif
namespace DA
{

class DAChartAddOHLCSeriesWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartAddOHLCSeriesWidget)
public:
	PrivateData(DAChartAddOHLCSeriesWidget* p);

public:
	DADataManager* _dataMgr { nullptr };
#if DA_ENABLE_PYTHON
	DAPySeriesTableModel* _model { nullptr };
#endif
};

DAChartAddOHLCSeriesWidget::PrivateData::PrivateData(DAChartAddOHLCSeriesWidget* p) : q_ptr(p)
{
}
//===================================================
// DAChartAddOHLCSeriesWidget
//===================================================

DAChartAddOHLCSeriesWidget::DAChartAddOHLCSeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddOHLCSeriesWidget)
{
	ui->setupUi(this);
#if DA_ENABLE_PYTHON
	d_ptr->_model = new DAPySeriesTableModel(this);
	d_ptr->_model->setHeaderLabel({ tr("t"), tr("o"), tr("h"), tr("l"), tr("c") });
	ui->tableViewOHLC->setModel(d_ptr->_model);
#endif
	QFontMetrics fm = fontMetrics();
	ui->tableViewOHLC->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
	connect(this, &DAChartAddOHLCSeriesWidget::dataManagerChanged, this, &DAChartAddOHLCSeriesWidget::onDataManagerChanged);
	connect(this, &DAChartAddOHLCSeriesWidget::currentDataChanged, this, &DAChartAddOHLCSeriesWidget::onCurrentDataChanged);
	connect(ui->comboBoxT,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddOHLCSeriesWidget::onComboBoxTCurrentDataframeSeriesChanged);
	connect(ui->comboBoxO,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddOHLCSeriesWidget::onComboBoxOCurrentDataframeSeriesChanged);
	connect(ui->comboBoxH,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddOHLCSeriesWidget::onComboBoxHCurrentDataframeSeriesChanged);
	connect(ui->comboBoxL,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddOHLCSeriesWidget::onComboBoxLCurrentDataframeSeriesChanged);
	connect(ui->comboBoxC,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DAChartAddOHLCSeriesWidget::onComboBoxCCurrentDataframeSeriesChanged);
	connect(ui->groupBoxTAutoincrement, &QGroupBox::clicked, this, &DAChartAddOHLCSeriesWidget::onGroupBoxTAutoincrementClicked);
}

DAChartAddOHLCSeriesWidget::~DAChartAddOHLCSeriesWidget()
{
	delete ui;
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddOHLCSeriesWidget::isTAutoincrement() const
{
    return ui->groupBoxTAutoincrement->isChecked();
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QwtOHLCSample > DAChartAddOHLCSeriesWidget::getSeries() const
{
	DAChartAddOHLCSeriesWidget* that = const_cast< DAChartAddOHLCSeriesWidget* >(this);
	QVector< QwtOHLCSample > ohlc;
	that->getToVectorPointFFromUI(ohlc);
	return ohlc;
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onComboBoxTCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
	d_ptr->_model->setSeriesAt(0, series);
    ui->tableViewOHLC->showActualRow(0);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxOCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onComboBoxOCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
	d_ptr->_model->setSeriesAt(1, series);
    ui->tableViewOHLC->showActualRow(0);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxHCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onComboBoxHCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
	d_ptr->_model->setSeriesAt(2, series);
    ui->tableViewOHLC->showActualRow(0);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxLCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onComboBoxLCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
	d_ptr->_model->setSeriesAt(3, series);
    ui->tableViewOHLC->showActualRow(0);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxCCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onComboBoxCCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
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
	d_ptr->_model->setSeriesAt(4, series);
    ui->tableViewOHLC->showActualRow(0);
#endif
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddOHLCSeriesWidget::onGroupBoxTAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
	if (on) {
		double base, step;
		if (tryGetTSelfInc(base, step)) {
			d_ptr->_model->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
		}
	} else {
		// 取消要读取回原来的设置
		DAPySeries series;
		DAData data = ui->comboBoxT->getCurrentDAData();
		if (data) {
			series = data.toSeries();
		}
		d_ptr->_model->setSeriesAt(0, series);
	}
    ui->tableViewOHLC->showActualRow(0);
	ui->comboBoxT->setEnabled(!on);
#endif
}

void DAChartAddOHLCSeriesWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxT->setDataManager(dmgr);
	ui->comboBoxO->setDataManager(dmgr);
	ui->comboBoxH->setDataManager(dmgr);
	ui->comboBoxL->setDataManager(dmgr);
	ui->comboBoxC->setDataManager(dmgr);
}

void DAChartAddOHLCSeriesWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxT->setCurrentDAData(d);
	ui->comboBoxO->setCurrentDAData(d);
	ui->comboBoxH->setCurrentDAData(d);
	ui->comboBoxL->setCurrentDAData(d);
	ui->comboBoxC->setCurrentDAData(d);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddOHLCSeriesWidget::getTAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
	bool isOK   = false;
	double base = ui->lineEditTInitValue->text().toDouble(&isOK);
	if (!isOK) {
        QMessageBox::
            warning(this,
                    tr("Warning"),  // cn:警告
                    tr("The initial value of x auto increment series must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
            );
		return false;
	}
	double step = ui->lineEditTStepValue->text().toDouble(&isOK);
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
 * @brief 获取点序列
 * @param res
 * @return
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddOHLCSeriesWidget::getToVectorPointFFromUI(QVector< QwtOHLCSample >& res)
{
	bool isTAuto = ui->groupBoxTAutoincrement->isChecked();
#if DA_ENABLE_PYTHON
	if (isTAuto) {  // 不存在同时，因此这个就是x自增
		DAAutoincrementSeries< double > tinc;
		if (!getTAutoIncFromUI(tinc)) {
			return false;
		}
		DAData od = ui->comboBoxO->getCurrentDAData();
		DAData hd = ui->comboBoxH->getCurrentDAData();
		DAData ld = ui->comboBoxL->getCurrentDAData();
		DAData cd = ui->comboBoxC->getCurrentDAData();
		if (!od.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("o must be a series"));  // cn:o必须是序列
			return false;
		}
		if (!hd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("h must be a series"));  // cn:h必须是序列
			return false;
		}
		if (!ld.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("l must be a series"));  // cn:l必须是序列
			return false;
		}
		if (!cd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("c must be a series"));  // cn:c必须是序列
			return false;
		}
		DAPySeries o = od.toSeries();
		DAPySeries h = hd.toSeries();
		DAPySeries l = ld.toSeries();
		DAPySeries c = cd.toSeries();
		if (o.isNone() || h.isNone() || l.isNone() || c.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}
		std::size_t s = o.size();
		try {
			std::vector< double > vo;
			std::vector< double > vh;
			std::vector< double > vl;
			std::vector< double > vc;
			vo.reserve(o.size());
			vh.reserve(h.size());
			vl.reserve(l.size());
			vc.reserve(c.size());
			o.castTo< double >(std::back_inserter(vo));
			h.castTo< double >(std::back_inserter(vh));
			l.castTo< double >(std::back_inserter(vl));
			c.castTo< double >(std::back_inserter(vc));
			res.resize(static_cast< int >(s));
			for (int i = 0; i < s; ++i) {
				res[ i ] = QwtOHLCSample(tinc[ i ], vo[ i ], vh[ i ], vl[ i ], vc[ i ]);
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
		DAData td = ui->comboBoxT->getCurrentDAData();
		DAData od = ui->comboBoxO->getCurrentDAData();
		DAData hd = ui->comboBoxH->getCurrentDAData();
		DAData ld = ui->comboBoxL->getCurrentDAData();
		DAData cd = ui->comboBoxC->getCurrentDAData();
		if (!td.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("t must be a series")  // cn:t必须是序列
			);
			return false;
		}
		if (!od.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("o must be a series"));  // cn:o必须是序列
			return false;
		}
		if (!hd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("h must be a series"));  // cn:h必须是序列
			return false;
		}
		if (!ld.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("l must be a series"));  // cn:l必须是序列
			return false;
		}
		if (!cd.isSeries()) {
			QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("c must be a series"));  // cn:c必须是序列
			return false;
		}
		DAPySeries t = td.toSeries();
		DAPySeries o = od.toSeries();
		DAPySeries h = hd.toSeries();
		DAPySeries l = ld.toSeries();
		DAPySeries c = cd.toSeries();
		if (o.isNone() || h.isNone() || l.isNone() || c.isNone()) {
			QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
			return false;
		}

		std::size_t s = std::min(t.size(), o.size());
		if (0 == s) {
			return true;
		}
		try {
			std::vector< double > vt;
			std::vector< double > vo;
			std::vector< double > vh;
			std::vector< double > vl;
			std::vector< double > vc;
			vt.reserve(t.size());
			vo.reserve(o.size());
			vh.reserve(h.size());
			vl.reserve(l.size());
			vc.reserve(c.size());
			t.castTo< double >(std::back_inserter(vt));
			o.castTo< double >(std::back_inserter(vo));
			h.castTo< double >(std::back_inserter(vh));
			l.castTo< double >(std::back_inserter(vl));
			c.castTo< double >(std::back_inserter(vc));
			res.resize(static_cast< int >(s));
			for (int i = 0; i < s; ++i) {
				res[ i ] = QwtOHLCSample(vt[ i ], vo[ i ], vh[ i ], vl[ i ], vc[ i ]);
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
 * @brief 尝试获取t值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddOHLCSeriesWidget::tryGetTSelfInc(double& base, double& step)
{
	bool isOK = false;
	double a  = ui->lineEditTInitValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	double b = ui->lineEditTStepValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	base = a;
	step = b;
	return true;
}
}
